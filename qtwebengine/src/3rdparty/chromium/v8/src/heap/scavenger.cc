// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/heap/scavenger.h"

#include "src/common/globals.h"
#include "src/handles/global-handles.h"
#include "src/heap/array-buffer-sweeper.h"
#include "src/heap/concurrent-allocator.h"
#include "src/heap/concurrent-marking.h"
#include "src/heap/ephemeron-remembered-set.h"
#include "src/heap/gc-tracer-inl.h"
#include "src/heap/gc-tracer.h"
#include "src/heap/heap-inl.h"
#include "src/heap/heap.h"
#include "src/heap/mark-compact-inl.h"
#include "src/heap/mark-compact.h"
#include "src/heap/memory-chunk-inl.h"
#include "src/heap/memory-chunk-layout.h"
#include "src/heap/memory-chunk.h"
#include "src/heap/objects-visiting-inl.h"
#include "src/heap/pretenuring-handler.h"
#include "src/heap/remembered-set-inl.h"
#include "src/heap/scavenger-inl.h"
#include "src/heap/slot-set.h"
#include "src/heap/sweeper.h"
#include "src/objects/data-handler-inl.h"
#include "src/objects/embedder-data-array-inl.h"
#include "src/objects/js-array-buffer-inl.h"
#include "src/objects/objects-body-descriptors-inl.h"
#include "src/objects/slots.h"
#include "src/objects/transitions-inl.h"
#include "src/utils/utils-inl.h"

namespace v8 {
namespace internal {

class IterateAndScavengePromotedObjectsVisitor final : public ObjectVisitor {
 public:
  IterateAndScavengePromotedObjectsVisitor(Scavenger* scavenger,
                                           bool record_slots)
      : scavenger_(scavenger), record_slots_(record_slots) {}

  V8_INLINE void VisitMapPointer(Tagged<HeapObject> host) final {
    if (!record_slots_) return;
    MapWord map_word = host->map_word(kRelaxedLoad);
    if (map_word.IsForwardingAddress()) {
      // Surviving new large objects have forwarding pointers in the map word.
      DCHECK(MemoryChunk::FromHeapObject(host)->InNewLargeObjectSpace());
      return;
    }
    HandleSlot(host, HeapObjectSlot(host->map_slot()), map_word.ToMap());
  }

  V8_INLINE void VisitPointers(Tagged<HeapObject> host, ObjectSlot start,
                               ObjectSlot end) final {
    VisitPointersImpl(host, start, end);
  }

  V8_INLINE void VisitPointers(Tagged<HeapObject> host, MaybeObjectSlot start,
                               MaybeObjectSlot end) final {
    VisitPointersImpl(host, start, end);
  }

  inline void VisitEphemeron(Tagged<HeapObject> obj, int entry, ObjectSlot key,
                             ObjectSlot value) override {
    DCHECK(Heap::IsLargeObject(obj) || IsEphemeronHashTable(obj));
    VisitPointer(obj, value);

    if (ObjectInYoungGeneration(*key)) {
      // We cannot check the map here, as it might be a large object.
      scavenger_->RememberPromotedEphemeron(
          EphemeronHashTable::unchecked_cast(obj), entry);
    } else {
      VisitPointer(obj, key);
    }
  }

  // Special cases: Unreachable visitors for objects that are never found in the
  // young generation and thus cannot be found when iterating promoted objects.
  void VisitInstructionStreamPointer(Tagged<Code>,
                                     InstructionStreamSlot) final {
    UNREACHABLE();
  }
  void VisitCodeTarget(Tagged<InstructionStream>, RelocInfo*) final {
    UNREACHABLE();
  }
  void VisitEmbeddedPointer(Tagged<InstructionStream>, RelocInfo*) final {
    UNREACHABLE();
  }

 private:
  template <typename TSlot>
  V8_INLINE void VisitPointersImpl(Tagged<HeapObject> host, TSlot start,
                                   TSlot end) {
    using THeapObjectSlot = typename TSlot::THeapObjectSlot;
    // Treat weak references as strong.
    // TODO(marja): Proper weakness handling in the young generation.
    for (TSlot slot = start; slot < end; ++slot) {
      typename TSlot::TObject object = *slot;
      Tagged<HeapObject> heap_object;
      if (object.GetHeapObject(&heap_object)) {
        HandleSlot(host, THeapObjectSlot(slot), heap_object);
      }
    }
  }

  template <typename THeapObjectSlot>
  V8_INLINE void HandleSlot(Tagged<HeapObject> host, THeapObjectSlot slot,
                            Tagged<HeapObject> target) {
    static_assert(
        std::is_same<THeapObjectSlot, FullHeapObjectSlot>::value ||
            std::is_same<THeapObjectSlot, HeapObjectSlot>::value,
        "Only FullHeapObjectSlot and HeapObjectSlot are expected here");
    scavenger_->PageMemoryFence(MaybeObject::FromObject(target));

    if (Heap::InFromPage(target)) {
      SlotCallbackResult result = scavenger_->ScavengeObject(slot, target);
      bool success = (*slot).GetHeapObject(&target);
      USE(success);
      DCHECK(success);

      if (result == KEEP_SLOT) {
        SLOW_DCHECK(IsHeapObject(target));
        MemoryChunk* chunk = MemoryChunk::FromHeapObject(host);

        // Sweeper is stopped during scavenge, so we can directly
        // insert into its remembered set here.
        RememberedSet<OLD_TO_NEW>::Insert<AccessMode::ATOMIC>(chunk,
                                                              slot.address());
      }
      DCHECK(!MarkCompactCollector::IsOnEvacuationCandidate(target));
    } else if (record_slots_ &&
               MarkCompactCollector::IsOnEvacuationCandidate(target)) {
      // We should never try to record off-heap slots.
      DCHECK((std::is_same<THeapObjectSlot, HeapObjectSlot>::value));
      // InstructionStream slots never appear in new space because
      // Code objects, the only object that can contain code pointers, are
      // always allocated in the old space.
      DCHECK_IMPLIES(V8_EXTERNAL_CODE_SPACE_BOOL,
                     !MemoryChunk::FromHeapObject(target)->IsFlagSet(
                         MemoryChunk::IS_EXECUTABLE));

      // We cannot call MarkCompactCollector::RecordSlot because that checks
      // that the host page is not in young generation, which does not hold
      // for pending large pages.
      RememberedSet<OLD_TO_OLD>::Insert<AccessMode::ATOMIC>(
          MemoryChunk::FromHeapObject(host), slot.address());
    }

    if (target.InWritableSharedSpace()) {
      MemoryChunk* chunk = MemoryChunk::FromHeapObject(host);
      RememberedSet<OLD_TO_SHARED>::Insert<AccessMode::ATOMIC>(chunk,
                                                               slot.address());
    }
  }

  Scavenger* const scavenger_;
  const bool record_slots_;
};

namespace {

V8_INLINE bool IsUnscavengedHeapObject(Heap* heap, Tagged<Object> object) {
  return Heap::InFromPage(object) && !HeapObject::cast(object)
                                          ->map_word(kRelaxedLoad)
                                          .IsForwardingAddress();
}

// Same as IsUnscavengedHeapObject() above but specialized for HeapObjects.
V8_INLINE bool IsUnscavengedHeapObject(Heap* heap,
                                       Tagged<HeapObject> heap_object) {
  return Heap::InFromPage(heap_object) &&
         !heap_object->map_word(kRelaxedLoad).IsForwardingAddress();
}

bool IsUnscavengedHeapObjectSlot(Heap* heap, FullObjectSlot p) {
  return IsUnscavengedHeapObject(heap, *p);
}

}  // namespace

ScavengerCollector::JobTask::JobTask(
    ScavengerCollector* outer,
    std::vector<std::unique_ptr<Scavenger>>* scavengers,
    std::vector<std::pair<ParallelWorkItem, MemoryChunk*>> memory_chunks,
    Scavenger::CopiedList* copied_list,
    Scavenger::PromotionList* promotion_list)
    : outer_(outer),
      scavengers_(scavengers),
      memory_chunks_(std::move(memory_chunks)),
      remaining_memory_chunks_(memory_chunks_.size()),
      generator_(memory_chunks_.size()),
      copied_list_(copied_list),
      promotion_list_(promotion_list),
      trace_id_(
          reinterpret_cast<uint64_t>(this) ^
          outer_->heap_->tracer()->CurrentEpoch(GCTracer::Scope::SCAVENGER)) {}

void ScavengerCollector::JobTask::Run(JobDelegate* delegate) {
  DCHECK_LT(delegate->GetTaskId(), scavengers_->size());
  Scavenger* scavenger = (*scavengers_)[delegate->GetTaskId()].get();
  if (delegate->IsJoiningThread()) {
    TRACE_GC_WITH_FLOW(outer_->heap_->tracer(),
                       GCTracer::Scope::SCAVENGER_SCAVENGE_PARALLEL, trace_id_,
                       TRACE_EVENT_FLAG_FLOW_IN);
    ProcessItems(delegate, scavenger);
  } else {
    TRACE_GC_EPOCH_WITH_FLOW(
        outer_->heap_->tracer(),
        GCTracer::Scope::SCAVENGER_BACKGROUND_SCAVENGE_PARALLEL,
        ThreadKind::kBackground, trace_id_, TRACE_EVENT_FLAG_FLOW_IN);
    ProcessItems(delegate, scavenger);
  }
}

size_t ScavengerCollector::JobTask::GetMaxConcurrency(
    size_t worker_count) const {
  // We need to account for local segments held by worker_count in addition to
  // GlobalPoolSize() of copied_list_ and promotion_list_.
  size_t wanted_num_workers = std::max<size_t>(
      remaining_memory_chunks_.load(std::memory_order_relaxed),
      worker_count + copied_list_->Size() + promotion_list_->Size());
  if (!outer_->heap_->ShouldUseBackgroundThreads()) {
    return std::min<size_t>(wanted_num_workers, 1);
  }
  return std::min<size_t>(scavengers_->size(), wanted_num_workers);
}

void ScavengerCollector::JobTask::ProcessItems(JobDelegate* delegate,
                                               Scavenger* scavenger) {
  double scavenging_time = 0.0;
  {
    TimedScope scope(&scavenging_time);
    ConcurrentScavengePages(scavenger);
    scavenger->Process(delegate);
  }
  if (v8_flags.trace_parallel_scavenge) {
    PrintIsolate(outer_->heap_->isolate(),
                 "scavenge[%p]: time=%.2f copied=%zu promoted=%zu\n",
                 static_cast<void*>(this), scavenging_time,
                 scavenger->bytes_copied(), scavenger->bytes_promoted());
  }
}

void ScavengerCollector::JobTask::ConcurrentScavengePages(
    Scavenger* scavenger) {
  while (remaining_memory_chunks_.load(std::memory_order_relaxed) > 0) {
    base::Optional<size_t> index = generator_.GetNext();
    if (!index) return;
    for (size_t i = *index; i < memory_chunks_.size(); ++i) {
      auto& work_item = memory_chunks_[i];
      if (!work_item.first.TryAcquire()) break;
      scavenger->ScavengePage(work_item.second);
      if (remaining_memory_chunks_.fetch_sub(1, std::memory_order_relaxed) <=
          1) {
        return;
      }
    }
  }
}

ScavengerCollector::ScavengerCollector(Heap* heap)
    : isolate_(heap->isolate()), heap_(heap) {}

namespace {

// Helper class for updating weak global handles. There's no additional scavenge
// processing required here as this phase runs after actual scavenge.
class GlobalHandlesWeakRootsUpdatingVisitor final : public RootVisitor {
 public:
  void VisitRootPointer(Root root, const char* description,
                        FullObjectSlot p) final {
    UpdatePointer(p);
  }
  void VisitRootPointers(Root root, const char* description,
                         FullObjectSlot start, FullObjectSlot end) final {
    for (FullObjectSlot p = start; p < end; ++p) {
      UpdatePointer(p);
    }
  }

 private:
  void UpdatePointer(FullObjectSlot p) {
    Tagged<Object> object = *p;
    DCHECK(!HasWeakHeapObjectTag(object));
    // The object may be in the old generation as global handles over
    // approximates the list of young nodes. This checks also bails out for
    // Smis.
    if (!Heap::InYoungGeneration(object)) return;

    Tagged<HeapObject> heap_object = HeapObject::cast(object);
    // TODO(chromium:1336158): Turn the following CHECKs into DCHECKs after
    // flushing out potential issues.
    CHECK(Heap::InFromPage(heap_object));
    MapWord first_word = heap_object->map_word(kRelaxedLoad);
    CHECK(first_word.IsForwardingAddress());
    Tagged<HeapObject> dest = first_word.ToForwardingAddress(heap_object);
    HeapObjectReference::Update(FullHeapObjectSlot(p), dest);
    CHECK_IMPLIES(Heap::InYoungGeneration(dest),
                  Heap::InToPage(dest) || Heap::IsLargeObject(dest));
  }
};

}  // namespace

// Remove this crashkey after chromium:1010312 is fixed.
class V8_NODISCARD ScopedFullHeapCrashKey {
 public:
  explicit ScopedFullHeapCrashKey(Isolate* isolate) : isolate_(isolate) {
    isolate_->AddCrashKey(v8::CrashKeyId::kDumpType, "heap");
  }
  ~ScopedFullHeapCrashKey() {
    isolate_->AddCrashKey(v8::CrashKeyId::kDumpType, "");
  }

 private:
  Isolate* isolate_ = nullptr;
};

void ScavengerCollector::CollectGarbage() {
  ScopedFullHeapCrashKey collect_full_heap_dump_if_crash(isolate_);

  DCHECK(surviving_new_large_objects_.empty());
  std::vector<std::unique_ptr<Scavenger>> scavengers;
  Scavenger::EmptyChunksList empty_chunks;
  const int num_scavenge_tasks = NumberOfScavengeTasks();
  Scavenger::CopiedList copied_list;
  Scavenger::PromotionList promotion_list;
  EphemeronRememberedSet::TableList ephemeron_table_list;

  {
    const bool is_logging = isolate_->log_object_relocation();
    for (int i = 0; i < num_scavenge_tasks; ++i) {
      scavengers.emplace_back(
          new Scavenger(this, heap_, is_logging, &empty_chunks, &copied_list,
                        &promotion_list, &ephemeron_table_list, i));
    }

    std::vector<std::pair<ParallelWorkItem, MemoryChunk*>> memory_chunks;
    OldGenerationMemoryChunkIterator::ForAll(
        heap_, [&memory_chunks](MemoryChunk* chunk) {
          if (chunk->slot_set<OLD_TO_NEW>() ||
              chunk->typed_slot_set<OLD_TO_NEW>() ||
              chunk->slot_set<OLD_TO_NEW_BACKGROUND>()) {
            memory_chunks.emplace_back(ParallelWorkItem{}, chunk);
          }
        });

    RootScavengeVisitor root_scavenge_visitor(scavengers[kMainThreadId].get());

    {
      // Identify weak unmodified handles. Requires an unmodified graph.
      TRACE_GC(
          heap_->tracer(),
          GCTracer::Scope::SCAVENGER_SCAVENGE_WEAK_GLOBAL_HANDLES_IDENTIFY);
      isolate_->traced_handles()->ComputeWeaknessForYoungObjects(
          &JSObject::IsUnmodifiedApiObject);
    }
    {
      // Copy roots.
      TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_SCAVENGE_ROOTS);
      // Scavenger treats all weak roots except for global handles as strong.
      // That is why we don't set skip_weak = true here and instead visit
      // global handles separately.
      base::EnumSet<SkipRoot> options(
          {SkipRoot::kExternalStringTable, SkipRoot::kGlobalHandles,
           SkipRoot::kTracedHandles, SkipRoot::kOldGeneration,
           SkipRoot::kConservativeStack, SkipRoot::kReadOnlyBuiltins});
      if (V8_UNLIKELY(v8_flags.scavenge_separate_stack_scanning)) {
        options.Add(SkipRoot::kStack);
      }
      heap_->IterateRoots(&root_scavenge_visitor, options);
      isolate_->global_handles()->IterateYoungStrongAndDependentRoots(
          &root_scavenge_visitor);
      isolate_->traced_handles()->IterateYoungRoots(&root_scavenge_visitor);
      scavengers[kMainThreadId]->Publish();
    }
    {
      // Parallel phase scavenging all copied and promoted objects.
      TRACE_GC_ARG1(
          heap_->tracer(), GCTracer::Scope::SCAVENGER_SCAVENGE_PARALLEL_PHASE,
          "UseBackgroundThreads", heap_->ShouldUseBackgroundThreads());
      auto job =
          std::make_unique<JobTask>(this, &scavengers, std::move(memory_chunks),
                                    &copied_list, &promotion_list);
      TRACE_GC_NOTE_WITH_FLOW("Parallel scavenge started", job->trace_id(),
                              TRACE_EVENT_FLAG_FLOW_OUT);
      V8::GetCurrentPlatform()
          ->CreateJob(v8::TaskPriority::kUserBlocking, std::move(job))
          ->Join();
      DCHECK(copied_list.IsEmpty());
      DCHECK(promotion_list.IsEmpty());
    }

    if (V8_UNLIKELY(v8_flags.scavenge_separate_stack_scanning)) {
      IterateStackAndScavenge(&root_scavenge_visitor, &scavengers,
                              kMainThreadId);
      DCHECK(copied_list.IsEmpty());
      DCHECK(promotion_list.IsEmpty());
    }

    {
      // Scavenge weak global handles.
      TRACE_GC(heap_->tracer(),
               GCTracer::Scope::SCAVENGER_SCAVENGE_WEAK_GLOBAL_HANDLES_PROCESS);
      GlobalHandlesWeakRootsUpdatingVisitor visitor;
      isolate_->global_handles()->ProcessWeakYoungObjects(
          &visitor, &IsUnscavengedHeapObjectSlot);
      isolate_->traced_handles()->ProcessYoungObjects(
          &visitor, &IsUnscavengedHeapObjectSlot);
    }

    {
      // Finalize parallel scavenging.
      TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_SCAVENGE_FINALIZE);

      DCHECK(surviving_new_large_objects_.empty());

      for (auto& scavenger : scavengers) {
        scavenger->Finalize();
      }
      scavengers.clear();

      HandleSurvivingNewLargeObjects();
    }
  }

  {
    // Update references into new space
    TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_SCAVENGE_UPDATE_REFS);
    heap_->UpdateYoungReferencesInExternalStringTable(
        &Heap::UpdateYoungReferenceInExternalStringTableEntry);

    heap_->incremental_marking()->UpdateMarkingWorklistAfterScavenge();

    if (V8_UNLIKELY(v8_flags.track_retaining_path)) {
      heap_->UpdateRetainersAfterScavenge();
    }

    if (V8_UNLIKELY(v8_flags.always_use_string_forwarding_table)) {
      isolate_->string_forwarding_table()->UpdateAfterYoungEvacuation();
    }
  }

  SemiSpaceNewSpace* semi_space_new_space =
      SemiSpaceNewSpace::From(heap_->new_space());

  if (v8_flags.concurrent_marking) {
    // Ensure that concurrent marker does not track pages that are
    // going to be unmapped.
    for (Page* p :
         PageRange(semi_space_new_space->from_space().first_page(), nullptr)) {
      heap_->concurrent_marking()->ClearMemoryChunkData(p);
    }
  }

  ProcessWeakReferences(&ephemeron_table_list);

  // Set age mark.
  semi_space_new_space->set_age_mark(semi_space_new_space->top());

  // Since we promote all surviving large objects immediately, all remaining
  // large objects must be dead.
  // TODO(hpayer): Don't free all as soon as we have an intermediate generation.
  heap_->new_lo_space()->FreeDeadObjects(
      [](Tagged<HeapObject>) { return true; });

  {
    TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_FREE_REMEMBERED_SET);
    Scavenger::EmptyChunksList::Local empty_chunks_local(empty_chunks);
    MemoryChunk* chunk;
    while (empty_chunks_local.Pop(&chunk)) {
      // Since sweeping was already restarted only check chunks that already got
      // swept.
      if (chunk->SweepingDone()) {
        RememberedSet<OLD_TO_NEW>::CheckPossiblyEmptyBuckets(chunk);
        RememberedSet<OLD_TO_NEW_BACKGROUND>::CheckPossiblyEmptyBuckets(chunk);
      } else {
        chunk->possibly_empty_buckets()->Release();
      }
    }

#ifdef DEBUG
    OldGenerationMemoryChunkIterator::ForAll(
        heap_, [](MemoryChunk* chunk) {
          if (chunk->slot_set<OLD_TO_NEW>() ||
              chunk->typed_slot_set<OLD_TO_NEW>() ||
              chunk->slot_set<OLD_TO_NEW_BACKGROUND>()) {
            DCHECK(chunk->possibly_empty_buckets()->IsEmpty());
          }
        });
#endif
  }

  {
    TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_SWEEP_ARRAY_BUFFERS);
    SweepArrayBufferExtensions();
  }

  isolate_->global_handles()->UpdateListOfYoungNodes();
  isolate_->traced_handles()->UpdateListOfYoungNodes();

  // Update how much has survived scavenge.
  heap_->IncrementYoungSurvivorsCounter(heap_->SurvivedYoungObjectSize());
}

void ScavengerCollector::IterateStackAndScavenge(
    RootScavengeVisitor* root_scavenge_visitor,
    std::vector<std::unique_ptr<Scavenger>>* scavengers, int main_thread_id) {
  // Scan the stack, scavenge the newly discovered objects, and report
  // the survival statistics before and after the stack scanning.
  // This code is not intended for production.
  TRACE_GC(heap_->tracer(), GCTracer::Scope::SCAVENGER_SCAVENGE_STACK_ROOTS);
  size_t survived_bytes_before = 0;
  for (auto& scavenger : *scavengers) {
    survived_bytes_before +=
        scavenger->bytes_copied() + scavenger->bytes_promoted();
  }
  heap_->IterateStackRoots(root_scavenge_visitor);
  (*scavengers)[main_thread_id]->Process();
  size_t survived_bytes_after = 0;
  for (auto& scavenger : *scavengers) {
    survived_bytes_after +=
        scavenger->bytes_copied() + scavenger->bytes_promoted();
  }
  TRACE_EVENT2(TRACE_DISABLED_BY_DEFAULT("v8.gc"),
               "V8.GCScavengerStackScanning", "survived_bytes_before",
               survived_bytes_before, "survived_bytes_after",
               survived_bytes_after);
  if (v8_flags.trace_gc_verbose && !v8_flags.trace_gc_ignore_scavenger) {
    isolate_->PrintWithTimestamp(
        "Scavenge stack scanning: survived_before=%4zuKB, "
        "survived_after=%4zuKB delta=%.1f%%\n",
        survived_bytes_before / KB, survived_bytes_after / KB,
        (survived_bytes_after - survived_bytes_before) * 100.0 /
            survived_bytes_after);
  }
}

void ScavengerCollector::SweepArrayBufferExtensions() {
  DCHECK_EQ(0, heap_->new_lo_space()->Size());
  heap_->array_buffer_sweeper()->RequestSweep(
      ArrayBufferSweeper::SweepingType::kYoung,
      (heap_->new_space()->Size() == 0)
          ? ArrayBufferSweeper::TreatAllYoungAsPromoted::kYes
          : ArrayBufferSweeper::TreatAllYoungAsPromoted::kNo);
}

void ScavengerCollector::HandleSurvivingNewLargeObjects() {
  const bool is_compacting = heap_->incremental_marking()->IsCompacting();
  MarkingState* marking_state = heap_->marking_state();

  for (SurvivingNewLargeObjectMapEntry update_info :
       surviving_new_large_objects_) {
    Tagged<HeapObject> object = update_info.first;
    Tagged<Map> map = update_info.second;
    // Order is important here. We have to re-install the map to have access
    // to meta-data like size during page promotion.
    object->set_map_word(map, kRelaxedStore);

    if (is_compacting && marking_state->IsMarked(object) &&
        MarkCompactCollector::IsOnEvacuationCandidate(map)) {
      RememberedSet<OLD_TO_OLD>::Insert<AccessMode::ATOMIC>(
          MemoryChunk::FromHeapObject(object), object->map_slot().address());
    }
    LargePage* page = LargePage::FromHeapObject(object);
    heap_->lo_space()->PromoteNewLargeObject(page);
  }
  surviving_new_large_objects_.clear();
  heap_->new_lo_space()->set_objects_size(0);
}

void ScavengerCollector::MergeSurvivingNewLargeObjects(
    const SurvivingNewLargeObjectsMap& objects) {
  for (SurvivingNewLargeObjectMapEntry object : objects) {
    bool success = surviving_new_large_objects_.insert(object).second;
    USE(success);
    DCHECK(success);
  }
}

int ScavengerCollector::NumberOfScavengeTasks() {
  if (!v8_flags.parallel_scavenge) return 1;
  const int num_scavenge_tasks =
      static_cast<int>(
          SemiSpaceNewSpace::From(heap_->new_space())->TotalCapacity()) /
          MB +
      1;
  static int num_cores = V8::GetCurrentPlatform()->NumberOfWorkerThreads() + 1;
  int tasks = std::max(
      1, std::min({num_scavenge_tasks, kMaxScavengerTasks, num_cores}));
  if (!heap_->CanPromoteYoungAndExpandOldGeneration(
          static_cast<size_t>(tasks * Page::kPageSize))) {
    // Optimize for memory usage near the heap limit.
    tasks = 1;
  }
  return tasks;
}

Scavenger::PromotionList::Local::Local(Scavenger::PromotionList* promotion_list)
    : regular_object_promotion_list_local_(
          promotion_list->regular_object_promotion_list_),
      large_object_promotion_list_local_(
          promotion_list->large_object_promotion_list_) {}

namespace {
ConcurrentAllocator* CreateSharedOldAllocator2(Heap* heap) {
  if (v8_flags.shared_string_table && heap->isolate()->has_shared_space()) {
    return new ConcurrentAllocator(nullptr, heap->shared_allocation_space(),
                                   ConcurrentAllocator::Context::kGC);
  }
  return nullptr;
}

}  // namespace

Scavenger::Scavenger(ScavengerCollector* collector, Heap* heap, bool is_logging,
                     EmptyChunksList* empty_chunks, CopiedList* copied_list,
                     PromotionList* promotion_list,
                     EphemeronRememberedSet::TableList* ephemeron_table_list,
                     int task_id)
    : collector_(collector),
      heap_(heap),
      empty_chunks_local_(*empty_chunks),
      promotion_list_local_(promotion_list),
      copied_list_local_(*copied_list),
      ephemeron_table_list_local_(*ephemeron_table_list),
      pretenuring_handler_(heap_->pretenuring_handler()),
      local_pretenuring_feedback_(PretenuringHandler::kInitialFeedbackCapacity),
      copied_size_(0),
      promoted_size_(0),
      allocator_(heap, CompactionSpaceKind::kCompactionSpaceForScavenge),
      shared_old_allocator_(CreateSharedOldAllocator2(heap_)),
      is_logging_(is_logging),
      is_incremental_marking_(heap->incremental_marking()->IsMarking()),
      is_compacting_(heap->incremental_marking()->IsCompacting()),
      shared_string_table_(shared_old_allocator_ != nullptr),
      mark_shared_heap_(heap->isolate()->is_shared_space_isolate()),
      shortcut_strings_(
          heap->CanShortcutStringsDuringGC(GarbageCollector::SCAVENGER)) {
  DCHECK_IMPLIES(is_incremental_marking_,
                 heap->incremental_marking()->IsMajorMarking());
}

void Scavenger::IterateAndScavengePromotedObject(Tagged<HeapObject> target,
                                                 Tagged<Map> map, int size) {
  // We are not collecting slots on new space objects during mutation thus we
  // have to scan for pointers to evacuation candidates when we promote
  // objects. But we should not record any slots in non-black objects. Grey
  // object's slots would be rescanned. White object might not survive until
  // the end of collection it would be a violation of the invariant to record
  // its slots.
  const bool record_slots =
      is_compacting_ && heap()->marking_state()->IsMarked(target);

  IterateAndScavengePromotedObjectsVisitor visitor(this, record_slots);

  // Iterate all outgoing pointers including map word.
  target->IterateFast(map, size, &visitor);

  if (IsJSArrayBufferMap(map)) {
    DCHECK(!BasicMemoryChunk::FromHeapObject(target)->IsLargePage());
    JSArrayBuffer::cast(target)->YoungMarkExtensionPromoted();
  }
}

void Scavenger::RememberPromotedEphemeron(Tagged<EphemeronHashTable> table,
                                          int index) {
  auto indices =
      ephemeron_remembered_set_.insert({table, std::unordered_set<int>()});
  indices.first->second.insert(index);
}

void Scavenger::ScavengePage(MemoryChunk* page) {
  CodePageMemoryModificationScope memory_modification_scope(page);
  const bool record_old_to_shared_slots = heap_->isolate()->has_shared_space();

  if (page->slot_set<OLD_TO_NEW, AccessMode::ATOMIC>() != nullptr) {
    RememberedSet<OLD_TO_NEW>::IterateAndTrackEmptyBuckets(
        page,
        [this, page, record_old_to_shared_slots](MaybeObjectSlot slot) {
          SlotCallbackResult result = CheckAndScavengeObject(heap_, slot);
          // A new space string might have been promoted into the shared heap
          // during GC.
          if (result == REMOVE_SLOT && record_old_to_shared_slots) {
            CheckOldToNewSlotForSharedUntyped(page, slot);
          }
          return result;
        },
        &empty_chunks_local_);
  }

  RememberedSet<OLD_TO_NEW>::IterateTyped(
      page, [this, page, record_old_to_shared_slots](SlotType slot_type,
                                                     Address slot_address) {
        return UpdateTypedSlotHelper::UpdateTypedSlot(
            heap_, slot_type, slot_address,
            [this, page, slot_type, slot_address,
             record_old_to_shared_slots](FullMaybeObjectSlot slot) {
              SlotCallbackResult result = CheckAndScavengeObject(heap(), slot);
              // A new space string might have been promoted into the shared
              // heap during GC.
              if (result == REMOVE_SLOT && record_old_to_shared_slots) {
                CheckOldToNewSlotForSharedTyped(page, slot_type, slot_address,
                                                *slot);
              }
              return result;
            });
      });

  if (page->slot_set<OLD_TO_NEW_BACKGROUND, AccessMode::ATOMIC>() != nullptr) {
    RememberedSet<OLD_TO_NEW_BACKGROUND>::IterateAndTrackEmptyBuckets(
        page,
        [this, page, record_old_to_shared_slots](MaybeObjectSlot slot) {
          SlotCallbackResult result = CheckAndScavengeObject(heap_, slot);
          // A new space string might have been promoted into the shared heap
          // during GC.
          if (result == REMOVE_SLOT && record_old_to_shared_slots) {
            CheckOldToNewSlotForSharedUntyped(page, slot);
          }
          return result;
        },
        &empty_chunks_local_);
  }
}

void Scavenger::Process(JobDelegate* delegate) {
  ScavengeVisitor scavenge_visitor(this);

  bool done;
  size_t objects = 0;
  do {
    done = true;
    ObjectAndSize object_and_size;
    while (promotion_list_local_.ShouldEagerlyProcessPromotionList() &&
           copied_list_local_.Pop(&object_and_size)) {
      scavenge_visitor.Visit(object_and_size.first);
      done = false;
      if (delegate && ((++objects % kInterruptThreshold) == 0)) {
        if (!copied_list_local_.IsLocalEmpty()) {
          delegate->NotifyConcurrencyIncrease();
        }
      }
    }

    struct PromotionListEntry entry;
    while (promotion_list_local_.Pop(&entry)) {
      Tagged<HeapObject> target = entry.heap_object;
      IterateAndScavengePromotedObject(target, entry.map, entry.size);
      done = false;
      if (delegate && ((++objects % kInterruptThreshold) == 0)) {
        if (!promotion_list_local_.IsGlobalPoolEmpty()) {
          delegate->NotifyConcurrencyIncrease();
        }
      }
    }
  } while (!done);
}

void ScavengerCollector::ProcessWeakReferences(
    EphemeronRememberedSet::TableList* ephemeron_table_list) {
  ClearYoungEphemerons(ephemeron_table_list);
  ClearOldEphemerons();
}

// Clear ephemeron entries from EphemeronHashTables in new-space whenever the
// entry has a dead new-space key.
void ScavengerCollector::ClearYoungEphemerons(
    EphemeronRememberedSet::TableList* ephemeron_table_list) {
  ephemeron_table_list->Iterate([this](Tagged<EphemeronHashTable> table) {
    for (InternalIndex i : table->IterateEntries()) {
      // Keys in EphemeronHashTables must be heap objects.
      HeapObjectSlot key_slot(
          table->RawFieldOfElementAt(EphemeronHashTable::EntryToIndex(i)));
      Tagged<HeapObject> key = key_slot.ToHeapObject();
      if (IsUnscavengedHeapObject(heap_, key)) {
        table->RemoveEntry(i);
      } else {
        Tagged<HeapObject> forwarded = ForwardingAddress(key);
        key_slot.StoreHeapObject(forwarded);
      }
    }
  });
  ephemeron_table_list->Clear();
}

// Clear ephemeron entries from EphemeronHashTables in old-space whenever the
// entry has a dead new-space key.
void ScavengerCollector::ClearOldEphemerons() {
  auto* table_map = heap_->ephemeron_remembered_set_->tables();
  for (auto it = table_map->begin(); it != table_map->end();) {
    Tagged<EphemeronHashTable> table = it->first;
    auto& indices = it->second;
    for (auto iti = indices.begin(); iti != indices.end();) {
      // Keys in EphemeronHashTables must be heap objects.
      HeapObjectSlot key_slot(table->RawFieldOfElementAt(
          EphemeronHashTable::EntryToIndex(InternalIndex(*iti))));
      Tagged<HeapObject> key = key_slot.ToHeapObject();
      if (IsUnscavengedHeapObject(heap_, key)) {
        table->RemoveEntry(InternalIndex(*iti));
        iti = indices.erase(iti);
      } else {
        Tagged<HeapObject> forwarded = ForwardingAddress(key);
        key_slot.StoreHeapObject(forwarded);
        if (!Heap::InYoungGeneration(forwarded)) {
          iti = indices.erase(iti);
        } else {
          ++iti;
        }
      }
    }

    if (indices.empty()) {
      it = table_map->erase(it);
    } else {
      ++it;
    }
  }
}

void Scavenger::Finalize() {
  pretenuring_handler_->MergeAllocationSitePretenuringFeedback(
      local_pretenuring_feedback_);
  heap()->IncrementNewSpaceSurvivingObjectSize(copied_size_);
  heap()->IncrementPromotedObjectsSize(promoted_size_);
  collector_->MergeSurvivingNewLargeObjects(surviving_new_large_objects_);
  allocator_.Finalize();
  if (shared_old_allocator_) shared_old_allocator_->FreeLinearAllocationArea();
  empty_chunks_local_.Publish();
  ephemeron_table_list_local_.Publish();
  for (auto it = ephemeron_remembered_set_.begin();
       it != ephemeron_remembered_set_.end(); ++it) {
    DCHECK_IMPLIES(!MemoryChunk::FromHeapObject(it->first)->IsLargePage(),
                   !Heap::InYoungGeneration(it->first));
    heap()->ephemeron_remembered_set()->RecordEphemeronKeyWrites(
        it->first, std::move(it->second));
  }
}

void Scavenger::Publish() {
  copied_list_local_.Publish();
  promotion_list_local_.Publish();
}

void Scavenger::AddEphemeronHashTable(Tagged<EphemeronHashTable> table) {
  ephemeron_table_list_local_.Push(table);
}

template <typename TSlot>
void Scavenger::CheckOldToNewSlotForSharedUntyped(MemoryChunk* chunk,
                                                  TSlot slot) {
  MaybeObject object = *slot;
  Tagged<HeapObject> heap_object;

  if (object.GetHeapObject(&heap_object) &&
      heap_object.InWritableSharedSpace()) {
    RememberedSet<OLD_TO_SHARED>::Insert<AccessMode::ATOMIC>(chunk,
                                                             slot.address());
  }
}

void Scavenger::CheckOldToNewSlotForSharedTyped(MemoryChunk* chunk,
                                                SlotType slot_type,
                                                Address slot_address,
                                                MaybeObject new_target) {
  Tagged<HeapObject> heap_object;

  if (new_target.GetHeapObject(&heap_object) &&
      heap_object.InWritableSharedSpace()) {
    const uintptr_t offset = slot_address - chunk->address();
    DCHECK_LT(offset, static_cast<uintptr_t>(TypedSlotSet::kMaxOffset));

    base::MutexGuard guard(chunk->mutex());
    RememberedSet<OLD_TO_SHARED>::InsertTyped(chunk, slot_type,
                                              static_cast<uint32_t>(offset));
  }
}

void RootScavengeVisitor::VisitRootPointer(Root root, const char* description,
                                           FullObjectSlot p) {
  DCHECK(!HasWeakHeapObjectTag(*p));
  DCHECK(!MapWord::IsPacked((*p).ptr()));
  ScavengePointer(p);
}

void RootScavengeVisitor::VisitRootPointers(Root root, const char* description,
                                            FullObjectSlot start,
                                            FullObjectSlot end) {
  // Copy all HeapObject pointers in [start, end)
  for (FullObjectSlot p = start; p < end; ++p) {
    ScavengePointer(p);
  }
}

void RootScavengeVisitor::ScavengePointer(FullObjectSlot p) {
  Tagged<Object> object = *p;
  DCHECK(!HasWeakHeapObjectTag(object));
  DCHECK(!MapWord::IsPacked(object.ptr()));
  if (Heap::InYoungGeneration(object)) {
    scavenger_->ScavengeObject(FullHeapObjectSlot(p), HeapObject::cast(object));
  }
}

RootScavengeVisitor::RootScavengeVisitor(Scavenger* scavenger)
    : scavenger_(scavenger) {}

ScavengeVisitor::ScavengeVisitor(Scavenger* scavenger)
    : NewSpaceVisitor<ScavengeVisitor>(scavenger->heap()->isolate()),
      scavenger_(scavenger) {}

}  // namespace internal
}  // namespace v8
