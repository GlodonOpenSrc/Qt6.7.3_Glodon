--
-- Copyright 2022 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
-- Those are helper functions used in computing jank metrics

-- This function takes timestamps of two consecutive frames and determines if
-- its janky by a delay of more than 0.5 of a frame  in order to make sure that
-- the comparison does not filter out ratios that are precisely 0.5, which can
-- fall a little above or below exact value due to inherent inaccuracy of operations with
-- floating-point numbers. Value 1e-9 have been chosen as follows: the ratio has
-- nanoseconds in numerator and VSync interval in denominator. Assuming refresh
-- rate more than 1 FPS (and therefore VSync interval less than a second), this
-- ratio should increase with increments more than minimal value in numerator
-- (1ns) divided by maximum value in denominator, giving 1e-9.

-- Function : function takes scroll ids of frames to verify it's from
-- the same scroll, and makes sure the frame ts occured within the scroll
-- timestamp of the neighbour and computes whether the frame was janky or not.
CREATE PERFETTO FUNCTION is_janky_frame(cur_gesture_id LONG,
                                      neighbour_gesture_id LONG,
                                      neighbour_ts LONG,
                                      cur_gesture_begin_ts LONG,
                                      cur_gesture_end_ts LONG,
                                      cur_frame_exact FLOAT,
                                      neighbour_frame_exact FLOAT)
-- Returns true if the frame was janky, false otherwise
RETURNS BOOL AS
SELECT
    CASE WHEN
      $cur_gesture_id != $neighbour_gesture_id OR
      $neighbour_ts IS NULL OR
      $neighbour_ts < $cur_gesture_begin_ts OR
      $neighbour_ts > $cur_gesture_end_ts THEN
        FALSE ELSE
        $cur_frame_exact > $neighbour_frame_exact + 0.5 + 1e-9
    END;

-- Function : function takes the cur_frame_exact, prev_frame_exact and
-- next_frame_exact and returns the value of the jank budget of the current
-- frame.
--
-- jank_budget is the minimum amount of frames/time we need to reduce the frame
-- duration by for it to be no longer considered janky.
--
-- Returns the jank budget in percentage (i.e. 0.75) of vsync interval
-- percentage.
CREATE PERFETTO FUNCTION jank_budget(
  cur_frame_exact FLOAT,
  prev_frame_exact FLOAT,
  next_frame_exact FLOAT
)
RETURNS FLOAT AS
-- We determine the difference between the frame count of the current frame
-- and its consecutive frames by subtracting with the frame_exact values. We
-- null check for cases when the neighbor frame count can be null for the
-- first and last frames.
--
-- Since a frame is considered janky, if the difference in the frame count
-- with its adjacent frame is greater than 0.5 (half a vsync) which means we
-- need to reduce the frame count by a value less than 0.5 of maximum
-- difference in frame count for it to be no longer janky. We subtract 1e-9 as
-- we want to output minimum amount required.
SELECT
  COALESCE(
    -- Could be null if next or previous is null.
    MAX(
      ($cur_frame_exact - $prev_frame_exact),
      ($cur_frame_exact - $next_frame_exact)
    ),
    -- If one of them is null output the first non-null.
    ($cur_frame_exact - $prev_frame_exact),
    ($cur_frame_exact - $next_frame_exact)
    -- Otherwise return null
  ) - 0.5 - 1e-9;

-- Extract mojo information for the long-task-tracking scenario for specific
-- names. For example, LongTaskTracker slices may have associated IPC
-- metadata, or InterestingTask slices for input may have associated IPC to
-- determine whether the task is fling/etc.
--
-- @arg name STRING            The name of slice.
-- @column interface_name      Name of the interface of the IPC call.
-- @column ipc_hash            Hash of the IPC call.
-- @column message_type        Message type (e.g. reply).
-- @column id                  The slice ID.
SELECT CREATE_VIEW_FUNCTION(
  'CHROME_SELECT_LONG_TASK_SLICES(name STRING)',
  'interface_name STRING, ipc_hash INT, message_type STRING, id INT',
  'SELECT
      EXTRACT_ARG(s.arg_set_id, "chrome_mojo_event_info.mojo_interface_tag") AS interface_name,
      EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.ipc_hash") AS ipc_hash,
      CASE
        WHEN EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.is_reply") THEN "reply"
        ELSE "message"
      END AS message_type,
      s.id
    FROM slice s
    WHERE
      category GLOB "*scheduler.long_tasks*"
      AND name = $name
  '
);
