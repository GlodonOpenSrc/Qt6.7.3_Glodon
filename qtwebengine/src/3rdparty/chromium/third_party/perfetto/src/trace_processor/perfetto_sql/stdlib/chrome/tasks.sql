--
-- Copyright 2023 The Android Open Source Project
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

INCLUDE PERFETTO MODULE common.slices;

-- Returns the mojo ipc hash for a given task, looking it up from the
-- argument of descendant ScopedSetIpcHash slice.
-- This is relevant only for the older Chrome traces, where mojo IPC
-- hash was reported in a separate ScopedSetIpcHash slice.
CREATE PERFETTO FUNCTION internal_extract_mojo_ipc_hash(slice_id INT)
RETURNS INT AS
SELECT EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.ipc_hash")
FROM descendant_slice($slice_id)
WHERE name="ScopedSetIpcHash"
ORDER BY id
LIMIT 1;

-- Returns the frame type (main frame vs subframe) for key navigation tasks
-- which capture the associated RenderFrameHost in an argument.
CREATE PERFETTO FUNCTION internal_extract_frame_type(slice_id INT)
RETURNS INT AS
SELECT EXTRACT_ARG(arg_set_id, "render_frame_host.frame_type")
FROM descendant_slice($slice_id)
WHERE name IN (
  "RenderFrameHostImpl::BeginNavigation",
  "RenderFrameHostImpl::DidCommitProvisionalLoad",
  "RenderFrameHostImpl::DidCommitSameDocumentNavigation",
  "RenderFrameHostImpl::DidStopLoading"
)
LIMIT 1;

-- Human-readable aliases for a few key navigation tasks.
CREATE PERFETTO FUNCTION internal_human_readable_navigation_task_name(task_name STRING)
RETURNS STRING AS
SELECT
  CASE
    WHEN $task_name = "content.mojom.FrameHost message (hash=2168461044)"
      THEN "FrameHost::BeginNavigation"
    WHEN $task_name = "content.mojom.FrameHost message (hash=3561497419)"
      THEN "FrameHost::DidCommitProvisionalLoad"
    WHEN $task_name = "content.mojom.FrameHost message (hash=1421450774)"
      THEN "FrameHost::DidCommitSameDocumentNavigation"
    WHEN $task_name = "content.mojom.FrameHost message (hash=368650583)"
      THEN "FrameHost::DidStopLoading"
  END;

-- Takes a task name and formats it correctly for scheduler tasks.
CREATE PERFETTO FUNCTION internal_format_scheduler_task_name(task_name STRING)
RETURNS STRING AS
SELECT printf("RunTask(posted_from=%s)", $task_name);

-- Takes the category and determines whether it is "Java" only, as opposed to
-- "toplevel,Java".
CREATE PERFETTO FUNCTION internal_java_not_top_level_category(category STRING)
RETURNS BOOL AS
SELECT $category GLOB "*Java*" AND $category not GLOB "*toplevel*";

-- Takes the category and determines whether is any valid
-- toplevel category or combination of categories.
CREATE PERFETTO FUNCTION internal_any_top_level_category(category STRING)
RETURNS BOOL AS
SELECT $category IN ("toplevel", "toplevel,viz", "toplevel,Java");

-- TODO(altimin): the situations with kinds in this file is a bit of a mess.
-- The idea is that it should work as `type` in the `slice` table, pointing to
-- a "child" table with more information about the task (e.g. posted_from for
-- scheduler tasks). Currently this is not the case and needs a cleanup.
-- Also we should align this with how table inheritance should work for
-- `CREATE PERFETTO TABLE`.

-- Get task type for a given task kind.
CREATE PERFETTO FUNCTION internal_get_java_views_task_type(kind STRING)
RETURNS STRING AS
SELECT
  CASE $kind
    WHEN "Choreographer" THEN "choreographer"
    WHEN "SingleThreadProxy::BeginMainFrame" THEN "ui_thread_begin_main_frame"
  END;

-- All slices corresponding to receiving mojo messages.
-- On the newer Chrome versions, it's just "Receive mojo message" and
-- "Receive mojo reply" slices (or "Receive {mojo_message_name}" if
-- built with `extended_tracing_enabled`. On legacy Chrome versions,
-- other appropriate messages (like "Connector::DispatchMessage") are used.
--
-- @column STRING interface_name    Name of the IPC interface.
-- @column INT ipc_hash             Hash of a message name.
-- @column STRING message_type      Either 'message' or 'reply'.
-- @column INT id                   Slice id.
--
-- Note: this might include messages received within a sync mojo call.
-- TODO(altimin): This should use EXTEND_TABLE when it becomes available.
CREATE TABLE internal_chrome_mojo_slices AS
WITH
-- Select all new-style (post crrev.com/c/3270337) mojo slices and
-- generate |task_name| for them.
-- If extended tracing is enabled, the slice name will have the full method
-- name (i.e. "Receive content::mojom::FrameHost::DidStopLoading") and we
-- should use it as a full name.
-- If extended tracing is not enabled, we should include the interface name
-- and method hash into the full name.
new_mojo_slices AS (
  SELECT
    EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.mojo_interface_tag") AS interface_name,
    EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.ipc_hash") AS ipc_hash,
    CASE name
      WHEN "Receive mojo message" THEN "message"
      WHEN "Receive mojo reply" THEN "reply"
    END AS message_type,
    id
  FROM slice
  WHERE
    category GLOB '*toplevel*'
    AND name GLOB 'Receive *'
),
-- Select old-style slices for channel-associated mojo events.
old_associated_mojo_slices AS (
  SELECT
    name AS interface_name,
    internal_extract_mojo_ipc_hash(id) AS ipc_hash,
    "message" AS message_type,
    id
  FROM slice
  WHERE
    category GLOB "*mojom*"
    AND name GLOB '*.mojom.*'
),
-- Select old-style slices for non-(channel-associated) mojo events.
old_non_associated_mojo_slices AS (
  SELECT
    COALESCE(
      EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.watcher_notify_interface_tag"),
      EXTRACT_ARG(arg_set_id, "chrome_mojo_event_info.mojo_interface_tag")
    ) AS interface_name,
    internal_extract_mojo_ipc_hash(id) AS ipc_hash,
    "message" AS message_type,
    id
  FROM slice
  WHERE
    category GLOB "*toplevel*" AND name = "Connector::DispatchMessage"
)
-- Merge all mojo slices.
SELECT * FROM new_mojo_slices
UNION ALL
SELECT * FROM old_associated_mojo_slices
UNION ALL
SELECT * FROM old_non_associated_mojo_slices;

-- As we lookup by ID on |internal_chrome_mojo_slices| table, add an index on
-- id to make lookups fast.
CREATE INDEX internal_chrome_mojo_slices_idx ON internal_chrome_mojo_slices(id);

-- This table contains a list of slices corresponding to the _representative_
-- Chrome Java view operations.
-- These are the outermost Java view slices after filtering out generic framework views
-- (like FitWindowsLinearLayout) and selecting the outermost slices from the remaining ones.
--
-- @column id INT                       Slice id.
-- @column ts INT                       Timestamp.
-- @column dur INT                      Duration.
-- @column name STRING                  Name of the view.
-- @column is_software_screenshot BOOL  Whether this slice is a part of non-accelerated
--                                      capture toolbar screenshot.
-- @column is_hardware_screenshot BOOL  Whether this slice is a part of accelerated
--                                      capture toolbar screenshot.
CREATE TABLE internal_chrome_java_views AS
WITH
-- .draw, .onLayout and .onMeasure parts of the java view names don't add much, strip them.
java_slices_with_trimmed_names AS (
  SELECT
    id,
    REPLACE(
      REPLACE(
        REPLACE(
          REPLACE(
            REPLACE(
              s1.name,
              ".draw", ""),
            ".onLayout", ""),
          ".onMeasure", ""),
        ".Layout", ""),
      ".Measure", "") AS name,
      ts,
      dur
    FROM
      slice s1
    -- Ensure that toplevel Java slices are not included, as they may be logged
    -- with either category = "toplevel" or category = "toplevel,Java".
    -- Also filter out the zero duration slices as an attempt to reduce noise as
    -- "Java" category contains misc events (as it's hard to add new categories).
    WHERE internal_java_not_top_level_category(category) AND dur > 0
  ),
  -- We filter out generic slices from various UI frameworks which don't tell us much about
  -- what exactly this view is doing.
  interesting_java_slices AS (
    SELECT
      id, name, ts, dur
    FROM java_slices_with_trimmed_names
    WHERE NOT name IN (
      -- AndroidX.
      "FitWindowsFrameLayout",
      "FitWindowsLinearLayout",
      "ContentFrameLayout",
      "CoordinatorLayout",
      -- Other non-Chrome UI libraries.
      "ComponentHost",
      -- Generic Chrome frameworks.
      "CompositorView:finalizeLayers",
      "CompositorViewHolder",
      "CompositorViewHolder:layout",
      "CompositorViewHolder:updateContentViewChildrenDimension",
      "CoordinatorLayoutForPointer",
      "OptimizedFrameLayout",
      "ViewResourceAdapter:getBitmap",
      "ViewResourceFrameLayout",
      -- Non-specific Chrome slices.
      "AppCompatImageButton",
      "ScrollingBottomViewResourceFrameLayout",
      -- Screenshots get their custom annotations below.
      "ViewResourceAdapter:captureWithHardwareDraw",
      "ViewResourceAdapter:captureWithSoftwareDraw",
      -- Non-bytecode generated slices.
      "LayoutDriver:onUpdate"
    )
)
SELECT
  s1.*,
  -- While the parent slices are too generic to be used by themselves,
  -- they can provide some useful metadata.
  has_parent_slice_with_name(
    s1.id,
    "ViewResourceAdapter:captureWithSoftwareDraw"
  ) AS is_software_screenshot,
  has_parent_slice_with_name(
    s1.id,
    "ViewResourceAdapter:captureWithHardwareDraw"
  ) AS is_hardware_screenshot
FROM interesting_java_slices s1
-- We select "outermost" interesting slices: interesting slices which
-- do not another interesting slice in their parent chain.
WHERE (SELECT count()
  FROM ancestor_slice(s1.id) s2
  JOIN interesting_java_slices s3 ON s2.id = s3.id) = 0;

-- A list of slices corresponding to operations on interesting (non-generic)
-- Chrome Java views. The view is considered interested if it's not a system
-- (ContentFrameLayout) or generic library (CompositorViewHolder) views.
--
-- @column filtered_name                Name of the view.
-- @column is_software_screenshot BOOL  Whether this slice is a part of non-accelerated
--                                      capture toolbar screenshot.
-- @column is_hardware_screenshot BOOL  Whether this slice is a part of accelerated
--                                      capture toolbar screenshot.
-- TODO(altimin): Add "columns_from slice" annotation.
-- TODO(altimin): convert this to EXTEND_TABLE when it becomes available.
CREATE VIEW chrome_java_views AS
SELECT
  java_view.name AS filtered_name,
  java_view.is_software_screenshot,
  java_view.is_hardware_screenshot,
  slice.*
FROM internal_chrome_java_views java_view
JOIN slice USING (id);

-- A list of Choreographer tasks (Android frame generation) in Chrome.
CREATE VIEW internal_chrome_choreographer_tasks
AS
SELECT
  id,
  "Choreographer" AS kind,
  ts,
  dur,
  name
FROM slice
WHERE name GLOB "Looper.dispatch: android.view.Choreographer$FrameHandler*";

-- Extract task's posted_from information from task's arguments.
CREATE PERFETTO FUNCTION internal_get_posted_from(arg_set_id INT)
RETURNS STRING AS
WITH posted_from as (
  SELECT
    EXTRACT_ARG($arg_set_id, "task.posted_from.file_name") AS file_name,
    EXTRACT_ARG($arg_set_id, "task.posted_from.function_name") AS function_name
)
SELECT file_name || ":" || function_name as posted_from
FROM posted_from;

-- Selects the BeginMainFrame slices (which as posted from ScheduledActionSendBeginMainFrame),
-- used for root-level processing. In top-level/Java based slices, these will correspond to the
-- ancestor of descendant slices; in long-task tracking, these tasks will be
-- on a custom track and will need to be associated with children by timestamp
-- and duration. Corresponds with the Choreographer root slices in
-- chrome_choreographer_tasks below.
--
-- Schema:
-- @column is            The slice id.
-- @column kind          The type of Java slice.
-- @column ts            The timestamp of the slice.
-- @column name          The name of the slice.
SELECT CREATE_VIEW_FUNCTION(
  'INTERNAL_SELECT_BEGIN_MAIN_FRAME_JAVA_SLICES(name STRING)',
  'id INT, kind STRING, ts LONG, dur LONG, name STRING',
  'SELECT
      id,
      "SingleThreadProxy::BeginMainFrame" AS kind,
      ts,
      dur,
      name
    FROM slice
    WHERE
      (name = $name
        AND internal_get_posted_from(arg_set_id) =
            "cc/trees/single_thread_proxy.cc:ScheduledActionSendBeginMainFrame")
  '
);

-- A list of Chrome tasks which were performing operations with Java views,
-- together with the names of the these views.
-- @column id INT            Slice id.
-- @column kind STRING       Type of the task.
-- @column java_views STRING Concatenated names of Java views used by the task.
CREATE VIEW internal_chrome_slices_with_java_views AS
WITH
  -- Select UI thread BeginMainFrames (which are Chrome scheduler tasks) and
  -- Choreographer frames (which are looper tasks).
  root_slices AS (
    SELECT id, kind
    FROM INTERNAL_SELECT_BEGIN_MAIN_FRAME_JAVA_SLICES('ThreadControllerImpl::RunTask')
    UNION ALL
    SELECT id, kind FROM internal_chrome_choreographer_tasks
  ),
  -- Intermediate step to allow us to sort java view names.
  root_slice_and_java_view_not_grouped AS (
    SELECT
      root.id, root.kind, java_view.name AS java_view_name
    FROM root_slices root
    JOIN descendant_slice(root.id) child
    JOIN internal_chrome_java_views java_view ON java_view.id = child.id
  )
SELECT
  root.id,
  root.kind,
  GROUP_CONCAT(DISTINCT java_view.java_view_name) AS java_views
FROM root_slices root
LEFT JOIN root_slice_and_java_view_not_grouped java_view USING (id)
GROUP BY root.id;

-- A list of tasks executed by Chrome scheduler.
CREATE TABLE internal_chrome_scheduler_tasks AS
SELECT
  id
FROM slice
WHERE
  category GLOB "*toplevel*"
  AND (name = "ThreadControllerImpl::RunTask" OR name = "ThreadPool_RunTask")
ORDER BY id;

-- A list of tasks executed by Chrome scheduler.
--
-- @column id                    Slice id.
-- @column name                  Name of the task.
-- @column ts                    Timestamp.
-- @column dur                   Duration.
-- @column utid                  Utid of the thread this task run on.
-- @column thread_name           Name of the thread this task run on.
-- @column upid                  Upid of the process of this task.
-- @column process_name          Name of the process of this task.
-- @column track_id              Same as slice.track_id.
-- @column depth                 Same as slice.depth.
-- @column parent_id             Same as slice.parent_id.
-- @column arg_set_id            Same as slice.arg_set_id.
-- @column thread_ts             Same as slice.thread_ts.
-- @column thread_dur            Same as slice.thread_dur.
-- @column posted_from           Source location where the PostTask was called.
CREATE VIEW chrome_scheduler_tasks AS
SELECT
  task.id,
  "chrome_scheduler_tasks" as type,
  internal_format_scheduler_task_name(
    internal_get_posted_from(slice.arg_set_id)) as name,
  slice.ts,
  slice.dur,
  thread.utid,
  thread.name as thread_name,
  process.upid,
  process.name as process_name,
  slice.track_id,
  slice.category,
  slice.depth,
  slice.parent_id,
  slice.arg_set_id,
  slice.thread_ts,
  slice.thread_dur,
  internal_get_posted_from(slice.arg_set_id) as posted_from
FROM internal_chrome_scheduler_tasks task
JOIN slice using (id)
JOIN thread_track ON slice.track_id = thread_track.id
JOIN thread using (utid)
JOIN process using (upid)
ORDER BY task.id;

-- Select the slice that might be the descendant mojo slice for the given task
-- slice if it exists.
CREATE PERFETTO FUNCTION internal_get_descendant_mojo_slice_candidate(
  slice_id INT
)
RETURNS INT AS
SELECT
  id
FROM descendant_slice($slice_id)
WHERE
  -- The tricky case here is dealing with sync mojo IPCs: we do not want to
  -- pick up sync IPCs when we are in a non-IPC task.
  -- So we look at all toplevel events and pick up the first one:
  -- for sync mojo messages, it will be "Send mojo message", which then
  -- will fail.
  -- Some events are excluded as they can legimately appear under "RunTask"
  -- before "Receive mojo message".
  category GLOB "*toplevel*" AND
  name NOT IN (
    "SimpleWatcher::OnHandleReady",
    "MessagePipe peer closed")
ORDER by depth, ts
LIMIT 1;

SELECT CREATE_VIEW_FUNCTION('INTERNAL_DESCENDANT_MOJO_SLICE(slice_id INT)',
  'task_name STRING',
  '
  SELECT
    printf("%s %s (hash=%d)",
      mojo.interface_name, mojo.message_type, mojo.ipc_hash) AS task_name
  FROM slice task
  JOIN internal_chrome_mojo_slices mojo
    ON mojo.id = internal_get_descendant_mojo_slice_candidate($slice_id)
  WHERE task.id = $slice_id
  ');

-- A list of "Chrome tasks": top-level execution units (e.g. scheduler tasks /
-- IPCs / system callbacks) run by Chrome. For a given thread, the tasks
-- will not intersect.
--
-- @column task_name STRING  Name for the given task.
-- @column task_type STRING  Type of the task (e.g. "scheduler").
-- @column scheduling_delay INT
CREATE TABLE internal_chrome_tasks AS
WITH
-- Select slices from "toplevel" category which do not have another
-- "toplevel" slice as ancestor. The possible cases include sync mojo messages
-- and tasks in nested runloops. Toplevel events may also be logged as with
-- the Java category.
non_embedded_toplevel_slices AS (
  SELECT * FROM slice
  WHERE
    internal_any_top_level_category(category)
    AND (SELECT count() FROM ancestor_slice(slice.id) anc
      WHERE anc.category GLOB "*toplevel*" or anc.category GLOB "*toplevel.viz*") = 0
),
-- Select slices from "Java" category which do not have another "Java" or
-- "toplevel" slice as parent. In the longer term they should probably belong
-- to "toplevel" category as well, but for now this will have to do. Ensure
-- that "Java" slices do not include "toplevel" slices as those would be
-- handled elsewhere.
non_embedded_java_slices AS (
  SELECT
    id,
    name AS task_name,
    "java" as task_type
  FROM slice s
  WHERE
    internal_java_not_top_level_category(category)
    AND (SELECT count()
      FROM ancestor_slice(s.id) s2
      WHERE s2.category GLOB "*toplevel*" OR s2.category GLOB "*Java*") = 0
),
-- Generate full names for tasks with java views.
java_views_tasks AS (
  SELECT
    id,
    printf('%s(java_views=%s)', kind, java_views) AS task_name,
    internal_get_java_views_task_type(kind) AS task_type
  FROM internal_chrome_slices_with_java_views
),
scheduler_tasks AS (
  SELECT
    id,
    name as task_name,
    "scheduler" as task_type
  FROM chrome_scheduler_tasks
),
-- Select scheduler tasks which are used to run mojo messages and use the mojo names
-- as full names for these slices.
-- We restrict this to specific scheduler tasks which are expected to run mojo
-- tasks due to sync mojo events, which also emit similar events.
scheduler_tasks_with_mojo AS (
  SELECT
    -- We use the "RunTask" as the task, and pick up the name from its child
    -- "Receive mojo message" event.
    task.id,
    receive_message.task_name,
    "mojo" AS task_type
  FROM
    chrome_scheduler_tasks task
  JOIN INTERNAL_DESCENDANT_MOJO_SLICE(task.id) receive_message
  WHERE
    task.posted_from IN (
      "mojo/public/cpp/system/simple_watcher.cc:Notify",
      "mojo/public/cpp/system/simple_watcher.cc:ArmOrNotify",
      "mojo/public/cpp/bindings/lib/connector.cc:PostDispatchNextMessageFromPipe",
      "ipc/ipc_mojo_bootstrap.cc:Accept")
),
navigation_tasks AS (
  WITH tasks_with_readable_names AS (
    SELECT
      id,
      internal_human_readable_navigation_task_name(task_name) as readable_name,
      IFNULL(internal_extract_frame_type(id), 'unknown frame type') as frame_type
    FROM
      scheduler_tasks_with_mojo
  )
  SELECT
    id,
    printf("%s (%s)", readable_name, frame_type) as task_name,
    'navigation_task' AS task_type
  FROM tasks_with_readable_names
  WHERE readable_name IS NOT NULL
),
-- Add scheduler and mojo full names to non-embedded slices from
-- the "toplevel" category, with mojo ones taking precedence.
non_embedded_toplevel_slices_with_task_name AS (
  SELECT
    task.id AS id,
    COALESCE(
        navigation.task_name,
        java_views.task_name,
        mojo.task_name,
        scheduler.task_name,
        task.name
    ) AS name,
    COALESCE(
        navigation.task_type,
        java_views.task_type,
        mojo.task_type,
        scheduler.task_type,
        "other"
    ) AS task_type
  FROM non_embedded_toplevel_slices task
  LEFT JOIN scheduler_tasks_with_mojo mojo ON mojo.id = task.id
  LEFT JOIN scheduler_tasks scheduler ON scheduler.id = task.id
  LEFT JOIN java_views_tasks java_views ON java_views.id = task.id
  LEFT JOIN navigation_tasks navigation ON navigation.id = task.id
)
-- Merge slices from toplevel and Java categories.
SELECT * FROM non_embedded_toplevel_slices_with_task_name
UNION ALL
SELECT * FROM non_embedded_java_slices
ORDER BY id;

-- A list of "Chrome tasks": top-level execution units (e.g. scheduler tasks /
-- IPCs / system callbacks) run by Chrome. For a given thread, the slices
-- corresponding to these tasks will not intersect.
--
-- @column id INT              Id for the given task, also the id of the slice this task corresponds to.
-- @column name STRING         Name for the given task.
-- @column task_type STRING    Type of the task (e.g. "scheduler").
-- @column thread_name STRING  Thread name.
-- @column utid INT            Utid.
-- @column process_name STRING Process name.
-- @column upid INT            Upid.
-- @column full_name STRING    Legacy alias for |task_name|.
-- @column ts INT              Alias of |slice.ts|.
-- @column dur INT             Alias of |slice.dur|.
-- @column track_id INT        Alias of |slice.track_id|.
-- @column category STRING     Alias of |slice.category|.
-- @column arg_set_id INT      Alias of |slice.arg_set_id|.
-- @column thread_ts INT       Alias of |slice.thread_ts|.
-- @column thread_dur INT      Alias of |slice.thread_dur|.
-- @column full_name STRING    Legacy alias for |name|.
CREATE VIEW chrome_tasks AS
SELECT
  cti.id,
  cti.name,
  task_type,
  thread.name AS thread_name,
  thread.utid,
  process.name AS process_name,
  thread.upid,
  s.ts,
  s.dur,
  s.track_id,
  s.category,
  s.arg_set_id,
  s.thread_ts,
  s.thread_dur,
  cti.name as full_name
FROM internal_chrome_tasks cti
JOIN slice s ON cti.id = s.id
JOIN thread_track tt ON s.track_id = tt.id
JOIN thread USING (utid)
JOIN process USING (upid);
