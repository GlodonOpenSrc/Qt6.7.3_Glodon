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

INCLUDE PERFETTO MODULE common.timestamps;

-- Count Binder transactions per process.
--
-- @column process_name  Name of the process that started the binder transaction.
-- @column pid           PID of the process that started the binder transaction.
-- @column slice_name    Name of the slice with binder transaction.
-- @column event_count   Number of binder transactions in process in slice.
CREATE VIEW android_binder_metrics_by_process AS
SELECT
  process.name AS process_name,
  process.pid AS pid,
  slice.name AS slice_name,
  COUNT(*) AS event_count
FROM slice
JOIN thread_track ON slice.track_id = thread_track.id
JOIN thread ON thread.utid = thread_track.utid
JOIN process ON thread.upid = process.upid
WHERE
  slice.name GLOB 'binder*'
GROUP BY
  process_name,
  slice_name;

CREATE PERFETTO TABLE internal_binder_txn_merged AS
WITH
  -- Fetch the broken binder txns first, i.e, the txns that have children slices
  -- They are definietly broken because synchronous txns are blocked sleeping while
  -- waiting for a response.
  -- These broken txns will be excluded below in the binder_txn CTE
  broken_binder_txn AS (
    SELECT ancestor.id FROM slice
    JOIN slice ancestor ON ancestor.id = slice.parent_id
    WHERE ancestor.name = 'binder transaction'
    GROUP BY ancestor.id
  ),
  -- Adding MATERIALIZED here matters in cases where there are few/no binder
  -- transactions in the trace. Our cost estimation is not good enough to allow
  -- the query planner to see through to this fact. Instead, our cost estimation
  -- causes repeated queries on this table which is slow because it's an O(n)
  -- query.
  --
  -- We should fix this by doing some (ideally all) of the following:
  --  1) Add support for columnar tables in SQL which will allow for
  --     "subsetting" the slice table to only contain binder transactions.
  --  2) Make this query faster by adding improving string filtering.
  --  3) Add caching so that even if these queries happen many times, they are
  --     fast.
  --  4) Improve cost estimation algorithm to allow the joins to happen the
  --     right way around.
  binder_txn AS MATERIALIZED (
    SELECT
      slice.id AS binder_txn_id,
      process.name AS process_name,
      thread.name AS thread_name,
      thread.utid AS utid,
      thread.tid AS tid,
      process.pid AS pid,
      process.upid AS upid,
      slice.ts,
      slice.dur,
      thread.is_main_thread
    FROM slice
    JOIN thread_track ON slice.track_id = thread_track.id
    JOIN thread USING (utid)
    JOIN process USING (upid)
    LEFT JOIN broken_binder_txn ON broken_binder_txn.id = slice.id
    WHERE slice.name = 'binder transaction'
    AND broken_binder_txn.id IS NULL
  ),
  binder_reply AS (
    SELECT
      binder_txn.*,
      binder_reply.ts AS server_ts,
      binder_reply.dur AS server_dur,
      binder_reply.id AS binder_reply_id,
      reply_thread.name AS server_thread,
      reply_process.name AS server_process,
      reply_thread.utid AS server_utid,
      reply_thread.tid AS server_tid,
      reply_process.pid AS server_pid,
      reply_process.upid AS server_upid,
      aidl.name AS aidl_name
    FROM binder_txn
    JOIN flow binder_flow ON binder_txn.binder_txn_id = binder_flow.slice_out
    JOIN slice binder_reply ON binder_flow.slice_in = binder_reply.id
    JOIN thread_track reply_thread_track
      ON binder_reply.track_id = reply_thread_track.id
    JOIN thread reply_thread ON reply_thread.utid = reply_thread_track.utid
    JOIN process reply_process ON reply_process.upid = reply_thread.upid
    LEFT JOIN slice aidl ON aidl.parent_id = binder_reply.id
        AND (aidl.name GLOB 'AIDL::cpp*Server'
             OR aidl.name GLOB 'AIDL::java*server'
             OR aidl.name GLOB 'HIDL::*server')
  )
SELECT
  MIN(aidl_name) AS aidl_name,
  binder_txn_id,
  process_name AS client_process,
  thread_name AS client_thread,
  upid AS client_upid,
  utid AS client_utid,
  tid AS client_tid,
  pid AS client_pid,
  is_main_thread,
  ts AS client_ts,
  dur AS client_dur,
  binder_reply_id,
  server_process,
  server_thread,
  server_upid,
  server_utid,
  server_tid,
  server_pid,
  server_ts,
  server_dur
FROM binder_reply
WHERE client_dur != -1 AND server_dur != -1 AND client_dur >= server_dur
GROUP BY
  process_name,
  thread_name,
  binder_txn_id,
  binder_reply_id;

CREATE TABLE internal_oom_score AS
  SELECT
    process.upid,
    CAST(c.value AS INT) AS value,
    c.ts,
    IFNULL(LEAD(ts) OVER (PARTITION BY upid ORDER BY ts), trace_bounds.end_ts) AS end_ts
    FROM counter c, trace_bounds
         JOIN process_counter_track t ON c.track_id = t.id
         JOIN process USING (upid)
   WHERE t.name = 'oom_score_adj';

CREATE INDEX internal_oom_score_idx ON internal_oom_score(upid, ts);

-- Breakdown synchronous binder transactions per txn.
-- It returns data about the client and server ends of every binder transaction.
--
-- @column aidl_name name of the binder interface if existing.
-- @column binder_txn_id slice id of the binder txn.
-- @column client_process name of the client process.
-- @column client_thread name of the client thread.
-- @column client_upid Upid of the client process.
-- @column client_utid Utid of the client thread.
-- @column client_tid Tid of the client thread.
-- @column client_ts timestamp of the client txn.
-- @column client_dur dur of the client txn.
-- @column client_oom_score oom score of the client process at the start of the txn.
-- @column is_main_thread Whether the txn was initiated from the main thread of the client process.
-- @column binder_reply_id slice id of the binder reply.
-- @column server_process name of the server process.
-- @column server_thread  name of the server thread.
-- @column server_upid Upid of the server process.
-- @column server_utid Utid of the server thread.
-- @column server_tid Tid of the server thread.
-- @column server_ts timestamp of the server txn.
-- @column server_dur dur of the server txn.
-- @column server_oom_score oom score of the server process at the start of the reply.
CREATE VIEW android_sync_binder_metrics_by_txn AS
SELECT binder.*, client_oom.value AS client_oom_score, server_oom.value AS server_oom_score
FROM internal_binder_txn_merged binder
LEFT JOIN internal_oom_score client_oom
  ON
    binder.client_upid = client_oom.upid
    AND binder.client_ts BETWEEN client_oom.ts AND client_oom.end_ts
LEFT JOIN internal_oom_score server_oom
  ON
    binder.server_upid = server_oom.upid
    AND binder.server_ts BETWEEN server_oom.ts AND server_oom.end_ts;

CREATE VIEW internal_binder_txn
AS
SELECT client_ts AS ts, client_dur AS dur, client_utid AS utid, *
FROM android_sync_binder_metrics_by_txn;

CREATE VIEW internal_binder_reply
AS
SELECT server_ts AS ts, server_dur AS dur, server_utid AS utid, *
FROM android_sync_binder_metrics_by_txn;

CREATE VIRTUAL TABLE internal_sp_binder_txn_thread_state
USING
  SPAN_JOIN(internal_binder_txn PARTITIONED utid, thread_state PARTITIONED utid);

CREATE VIRTUAL TABLE internal_sp_binder_reply_thread_state
USING
  SPAN_JOIN(internal_binder_reply PARTITIONED utid, thread_state PARTITIONED utid);

-- Aggregated thread_states on the client and server side per binder txn
-- This builds on the data from |android_sync_binder_metrics_by_txn| and
-- for each end (client and server) of the transaction, it returns
-- the aggregated sum of all the thread state durations.
-- The |thread_state_type| column represents whether a given 'aggregated thread_state'
-- row is on the client or server side. 'binder_txn' is client side and 'binder_reply'
-- is server side.
--
-- @column binder_txn_id slice id of the binder txn
-- @column binder_reply_id slice id of the binder reply
-- @column thread_state_type whether thread state is on the txn or reply side
-- @column thread_state a thread_state that occurred in the txn
-- @column thread_state_dur aggregated dur of the |thread_state| in the txn
-- @column thread_state_count aggregated count of the |thread_state| in the txn
CREATE VIEW android_sync_binder_thread_state_by_txn
AS
SELECT
  binder_txn_id,
  client_ts,
  client_tid,
  binder_reply_id,
  server_ts,
  server_tid,
  'binder_txn' AS thread_state_type,
  state AS thread_state,
  SUM(dur) AS thread_state_dur,
  COUNT(dur) AS thread_state_count
FROM internal_sp_binder_txn_thread_state
GROUP BY binder_txn_id, binder_reply_id, thread_state_type, thread_state
UNION ALL
SELECT
  binder_txn_id,
  client_ts,
  client_tid,
  binder_reply_id,
  server_ts,
  server_tid,
  'binder_reply' AS thread_state_type,
  state AS thread_state,
  SUM(dur) AS thread_state_dur,
  COUNT(dur) AS thread_state_count
FROM internal_sp_binder_reply_thread_state
GROUP BY binder_txn_id, binder_reply_id, thread_state_type, thread_state;

-- Aggregated blocked_functions on the client and server side per binder txn
-- This builds on the data from |android_sync_binder_metrics_by_txn| and
-- for each end (client and server) of the transaction, it returns
-- the aggregated sum of all the kernel blocked function durations.
-- The |thread_state_type| column represents whether a given 'aggregated blocked_function'
-- row is on the client or server side. 'binder_txn' is client side and 'binder_reply'
-- is server side.
--
-- @column binder_txn_id slice id of the binder txn
-- @column binder_reply_id slice id of the binder reply
-- @column thread_state_type whether thread state is on the txn or reply side
-- @column blocked_function blocked kernel function in a thread state
-- @column blocked_function_dur aggregated dur of the |blocked_function| in the txn
-- @column blocked_function_count aggregated count of the |blocked_function| in the txn
CREATE VIEW android_sync_binder_blocked_functions_by_txn
AS
SELECT
  binder_txn_id,
  client_ts,
  client_tid,
  binder_reply_id,
  server_ts,
  server_tid,
  'binder_txn' AS thread_state_type,
  blocked_function,
  SUM(dur) AS blocked_function_dur,
  COUNT(dur) AS blocked_function_count
FROM internal_sp_binder_txn_thread_state
WHERE blocked_function IS NOT NULL
GROUP BY binder_txn_id, binder_reply_id, blocked_function
UNION ALL
SELECT
  binder_txn_id,
  client_ts,
  client_tid,
  binder_reply_id,
  server_ts,
  server_tid,
  'binder_reply' AS thread_state_type,
  blocked_function,
  SUM(dur) AS blocked_function_dur,
  COUNT(dur) AS blocked_function_count
FROM internal_sp_binder_reply_thread_state
WHERE blocked_function IS NOT NULL
GROUP BY binder_txn_id, binder_reply_id, blocked_function;

CREATE TABLE internal_async_binder_reply AS
WITH async_reply AS MATERIALIZED (
  SELECT id, ts, dur, track_id, name
  FROM slice
  WHERE
    name GLOB 'AIDL::cpp*Server'
    OR name GLOB 'AIDL::java*server'
    OR name GLOB 'HIDL::*server'
    OR name = 'binder async rcv'
) SELECT *, LEAD(name) OVER (PARTITION BY track_id ORDER BY ts) AS next_name FROM async_reply;

CREATE TABLE internal_binder_async_txn_raw AS
SELECT
  slice.id AS binder_txn_id,
  process.name AS client_process,
  thread.name AS client_thread,
  process.upid AS client_upid,
  thread.utid AS client_utid,
  thread.tid AS client_tid,
  process.pid AS client_pid,
  thread.is_main_thread,
  slice.ts AS client_ts,
  slice.dur AS client_dur
FROM slice
JOIN thread_track
  ON slice.track_id = thread_track.id
JOIN thread
  USING (utid)
JOIN process
  USING (upid)
WHERE slice.name = 'binder transaction async';

CREATE PERFETTO TABLE internal_binder_async_txn AS
SELECT
  IIF(binder_reply.next_name = 'binder async rcv', NULL, binder_reply.next_name) AS aidl_name,
  binder_txn.*,
  binder_reply.id AS binder_reply_id,
  reply_process.name AS server_process,
  reply_thread.name AS server_thread,
  reply_process.upid AS server_upid,
  reply_thread.utid AS server_utid,
  reply_thread.tid AS server_tid,
  reply_process.pid AS server_pid,
  binder_reply.ts AS server_ts,
  binder_reply.dur AS server_dur
FROM internal_binder_async_txn_raw binder_txn
JOIN flow binder_flow
  ON binder_txn.binder_txn_id = binder_flow.slice_out
JOIN internal_async_binder_reply binder_reply
  ON binder_flow.slice_in = binder_reply.id
JOIN thread_track reply_thread_track
  ON binder_reply.track_id = reply_thread_track.id
JOIN thread reply_thread
  ON reply_thread.utid = reply_thread_track.utid
JOIN process reply_process
  ON reply_process.upid = reply_thread.upid
WHERE binder_reply.name = 'binder async rcv';

-- Breakdown asynchronous binder transactions per txn.
-- It returns data about the client and server ends of every binder transaction async.
--
-- @column aidl_name name of the binder interface if existing.
-- @column binder_txn_id slice id of the binder txn.
-- @column client_process name of the client process.
-- @column client_thread name of the client thread.
-- @column client_upid Upid of the client process.
-- @column client_utid Utid of the client thread.
-- @column client_tid Tid of the client thread.
-- @column client_ts timestamp of the client txn.
-- @column client_dur dur of the client txn.
-- @column client_oom_score oom score of the client process at the start of the txn.
-- @column is_main_thread Whether the txn was initiated from the main thread of the client process.
-- @column binder_reply_id slice id of the binder reply.
-- @column server_process name of the server process.
-- @column server_thread  name of the server thread.
-- @column server_upid Upid of the server process.
-- @column server_utid Utid of the server thread.
-- @column server_tid Tid of the server thread.
-- @column server_ts timestamp of the server txn.
-- @column server_dur dur of the server txn.
-- @column server_oom_score oom score of the server process at the start of the reply.
CREATE VIEW android_async_binder_metrics_by_txn
AS
SELECT binder.*, client_oom.value AS client_oom_score, server_oom.value AS server_oom_score
FROM internal_binder_async_txn binder
LEFT JOIN internal_oom_score client_oom
  ON
    binder.client_upid = client_oom.upid
    AND binder.client_ts BETWEEN client_oom.ts AND client_oom.end_ts
LEFT JOIN internal_oom_score server_oom
  ON
    binder.server_upid = server_oom.upid
    AND binder.server_ts BETWEEN server_oom.ts AND server_oom.end_ts;

-- Breakdown asynchronous binder transactions per txn.
-- It returns data about the client and server ends of every binder transaction async.
--
-- @column aidl_name name of the binder interface if existing.
-- @column binder_txn_id slice id of the binder txn.
-- @column client_process name of the client process.
-- @column client_thread name of the client thread.
-- @column client_upid Upid of the client process.
-- @column client_utid Utid of the client thread.
-- @column client_tid Tid of the client thread.
-- @column client_ts timestamp of the client txn.
-- @column client_dur dur of the client txn.
-- @column client_oom_score oom score of the client process at the start of the txn.
-- @column is_main_thread Whether the txn was initiated from the main thread of the client process.
-- @column binder_reply_id slice id of the binder reply.
-- @column server_process name of the server process.
-- @column server_thread  name of the server thread.
-- @column server_upid Upid of the server process.
-- @column server_utid Utid of the server thread.
-- @column server_tid Tid of the server thread.
-- @column server_ts timestamp of the server txn.
-- @column server_dur dur of the server txn.
-- @column server_oom_score oom score of the server process at the start of the reply.
-- @column is_sync whether the txn is synchronous or async (oneway).
CREATE VIEW android_binder_txns
AS
SELECT *, 1 AS is_sync FROM android_sync_binder_metrics_by_txn
UNION ALL
SELECT *, 0 AS is_sync FROM android_async_binder_metrics_by_txn;

-- Returns a DAG of all outgoing binder txns from a process.
-- The roots of the graph are the threads making the txns and the graph flows from:
-- thread -> server_process -> AIDL interface -> AIDL method.
-- The weights of each node represent the wall execution time in the server_process.
--
-- @arg upid STRING   Upid of process to generate an outgoing graph for.
-- @ret pprof BYTES   Pprof of outgoing binder txns.
CREATE PERFETTO FUNCTION ANDROID_BINDER_OUTGOING_GRAPH(upid INT)
RETURNS TABLE(pprof BYTES) AS
WITH threads AS (
  SELECT binder_txn_id, CAT_STACKS(client_thread) AS stack
  FROM android_binder_txns
  WHERE ($upid IS NOT NULL AND client_upid = $upid) OR ($upid IS NULL)
), server_process AS (
  SELECT binder_txn_id, CAT_STACKS(stack, server_process) AS stack
  FROM android_binder_txns
  JOIN threads USING(binder_txn_id)
), end_points AS (
  SELECT binder_txn_id,
         CAT_STACKS(stack, STR_SPLIT(aidl_name, '::', IIF(aidl_name GLOB 'AIDL*', 2, 1))) AS stack
  FROM android_binder_txns
  JOIN server_process USING(binder_txn_id)
), aidl_names AS (
  SELECT binder_txn_id, server_dur,
         CAT_STACKS(stack, STR_SPLIT(aidl_name, '::', IIF(aidl_name GLOB 'AIDL*', 3, 2))) AS stack
  FROM android_binder_txns
  JOIN end_points USING(binder_txn_id)
) SELECT EXPERIMENTAL_PROFILE(stack, 'duration', 'ns', server_dur) AS pprof
  FROM aidl_names;

-- Returns a DAG of all incoming binder txns from a process.
-- The roots of the graph are the clients making the txns and the graph flows from:
-- client_process -> AIDL interface -> AIDL method.
-- The weights of each node represent the wall execution time in the server_process.
--
-- @arg upid STRING   Upid of process to generate an outgoing graph for.
-- @ret pprof BYTES   Pprof of outgoing binder txns.
CREATE PERFETTO FUNCTION ANDROID_BINDER_INCOMING_GRAPH(upid INT)
RETURNS TABLE(pprof BYTES) AS
WITH client_process AS (
  SELECT binder_txn_id, CAT_STACKS(client_process) AS stack
  FROM android_binder_txns
  WHERE ($upid IS NOT NULL AND server_upid = $upid) OR ($upid IS NULL)
), end_points AS (
  SELECT binder_txn_id,
         CAT_STACKS(stack, STR_SPLIT(aidl_name, '::', IIF(aidl_name GLOB 'AIDL*', 2, 1))) AS stack
  FROM android_binder_txns
  JOIN client_process USING(binder_txn_id)
), aidl_names AS (
  SELECT binder_txn_id, server_dur,
         CAT_STACKS(stack, STR_SPLIT(aidl_name, '::', IIF(aidl_name GLOB 'AIDL*', 3, 2))) AS stack
  FROM android_binder_txns
  JOIN end_points USING(binder_txn_id)
) SELECT EXPERIMENTAL_PROFILE(stack, 'duration', 'ns', server_dur) AS pprof
  FROM aidl_names;

-- Returns a graph of all binder txns in a trace.
-- The nodes are client_process and server_process.
-- The weights of each node represent the wall execution time in the server_process.
--
-- @arg min_client_oom_score INT   Matches txns from client_processes greater than or equal to the OOM score.
-- @arg max_client_oom_score INT   Matches txns from client_processes less than or equal to the OOM score.
-- @arg min_server_oom_score INT   Matches txns to server_processes greater than or equal to the OOM score.
-- @arg max_server_oom_score INT   Matches txns to server_processes less than or equal to the OOM score.
-- @ret pprof BYTES                Pprof of binder txns.
CREATE PERFETTO FUNCTION ANDROID_BINDER_GRAPH(min_client_oom_score INT, max_client_oom_score INT, min_server_oom_score INT, max_server_oom_score INT)
RETURNS TABLE(pprof BYTES) AS
WITH clients AS (
  SELECT binder_txn_id, CAT_STACKS(client_process) AS stack
   FROM android_binder_txns
   WHERE client_oom_score BETWEEN $min_client_oom_score AND $max_client_oom_score
), servers AS (
  SELECT binder_txn_id, server_dur, CAT_STACKS(stack, server_process) AS stack
  FROM android_binder_txns
  JOIN clients USING(binder_txn_id)
  WHERE server_oom_score BETWEEN $min_server_oom_score AND $max_server_oom_score
) SELECT EXPERIMENTAL_PROFILE(stack, 'duration', 'ns', server_dur) AS pprof
  FROM servers;
