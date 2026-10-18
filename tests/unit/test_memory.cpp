#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/memory/aligned_alloc.hpp"
#include "aetherq/memory/allocation_metrics.hpp"
#include "aetherq/memory/buffer.hpp"
#include "aetherq/memory/fixed_pool.hpp"
#include "aetherq/memory/monotonic_arena.hpp"
#include "aetherq/memory/pmr_resources.hpp"

using namespace aetherq;

TEST(AlignedAlloc, RoundUpRespectsAlignment) {
  EXPECT_EQ(round_up(0, 64), 0U);
  EXPECT_EQ(round_up(1, 64), 64U);
  EXPECT_EQ(round_up(64, 64), 64U);
  EXPECT_EQ(round_up(65, 64), 128U);
}

TEST(AlignedAlloc, ReturnsAlignedPointers) {
  for (const std::size_t alignment : {8U, 16U, 32U, 64U, 128U}) {
    void* pointer = aligned_allocate(1000, alignment);
    EXPECT_TRUE(is_aligned(pointer, alignment));
    aligned_release(pointer, alignment);
  }
}

TEST(Buffer, AllocatesAlignedStorage) {
  Buffer<double> buffer(128);
  EXPECT_EQ(buffer.size(), 128U);
  EXPECT_TRUE(is_aligned(buffer.data(), simd_alignment));
  buffer[0] = 1.5;
  EXPECT_DOUBLE_EQ(buffer[0], 1.5);
  buffer.fill(2.0);
  EXPECT_DOUBLE_EQ(buffer[127], 2.0);
}

TEST(Buffer, MoveTransfersOwnership) {
  Buffer<double> buffer(32);
  double* raw = buffer.data();
  Buffer<double> moved(std::move(buffer));
  EXPECT_EQ(moved.data(), raw);
  EXPECT_EQ(moved.size(), 32U);
  EXPECT_EQ(buffer.size(), 0U);
  EXPECT_EQ(buffer.data(), nullptr);
}

TEST(Buffer, ViewDoesNotOwnStorage) {
  std::vector<double> storage(16, 3.0);
  {
    Buffer<double> view = Buffer<double>::view(storage.data(), storage.size());
    EXPECT_EQ(view.size(), 16U);
    EXPECT_DOUBLE_EQ(view[0], 3.0);
  }
  EXPECT_DOUBLE_EQ(storage[0], 3.0);
}

TEST(MonotonicArena, AllocatesAndResets) {
  MonotonicArena arena(4096);
  EXPECT_EQ(arena.capacity(), 4096U);
  void* first = arena.allocate(100, 64);
  EXPECT_TRUE(is_aligned(first, 64));
  EXPECT_GT(arena.used(), 0U);
  arena.reset();
  EXPECT_EQ(arena.used(), 0U);
  EXPECT_EQ(arena.peak_used(), 100U);
}

TEST(MonotonicArena, ReportsFailureWhenExhausted) {
  MonotonicArena arena(64);
  EXPECT_NE(arena.allocate(64, 8), nullptr);
  EXPECT_EQ(arena.allocate(1024, 8), nullptr);
}

TEST(MonotonicArena, OwnsOnlyItsStorage) {
  MonotonicArena arena(256);
  void* inside = arena.allocate(32, 8);
  EXPECT_TRUE(arena.owns(inside));
  int outside = 0;
  EXPECT_FALSE(arena.owns(&outside));
}

TEST(FixedPool, ReusesSlots) {
  FixedPool pool(128, 4);
  EXPECT_EQ(pool.object_count(), 4U);
  EXPECT_EQ(pool.available(), 4U);
  void* first = pool.allocate();
  EXPECT_NE(first, nullptr);
  EXPECT_EQ(pool.in_use(), 1U);
  pool.release(first);
  EXPECT_EQ(pool.in_use(), 0U);
  void* reused = pool.allocate();
  EXPECT_EQ(reused, first);
}

TEST(FixedPool, ExhaustsAndReports) {
  FixedPool pool(64, 2);
  void* a = pool.allocate();
  void* b = pool.allocate();
  EXPECT_NE(a, nullptr);
  EXPECT_NE(b, nullptr);
  EXPECT_EQ(pool.allocate(), nullptr);
  pool.release(a);
  EXPECT_NE(pool.allocate(), nullptr);
}

TEST(FixedPool, IgnoresForeignPointers) {
  FixedPool pool(64, 2);
  int foreign = 0;
  pool.release(&foreign);
  EXPECT_EQ(pool.in_use(), 0U);
}

TEST(AllocationRecorder, TracksMetrics) {
  AllocationRecorder recorder(true);
  recorder.record_allocation(100, 10);
  recorder.record_allocation(200, 20);
  recorder.record_deallocation(100);
  recorder.record_reuse();
  const AllocationMetrics metrics = recorder.snapshot();
  EXPECT_EQ(metrics.allocation_count, 2U);
  EXPECT_EQ(metrics.deallocation_count, 1U);
  EXPECT_EQ(metrics.reuse_count, 1U);
  EXPECT_EQ(metrics.requested_bytes, 300U);
  EXPECT_EQ(metrics.peak_live_bytes, 300U);
  EXPECT_EQ(metrics.live_bytes, 200U);
  const LatencySummary summary = summarize_latencies(metrics.latency_ns);
  EXPECT_DOUBLE_EQ(summary.mean_ns, 15.0);
}

TEST(SystemMemoryResource, AllocatesThroughPmr) {
  AllocationRecorder recorder(false);
  SystemMemoryResource resource(&recorder);
  std::pmr::polymorphic_allocator<double> allocator(&resource);
  double* values = allocator.allocate(16);
  EXPECT_TRUE(is_aligned(values, alignof(double)));
  allocator.deallocate(values, 16);
  EXPECT_EQ(recorder.snapshot().allocation_count, 1U);
}

TEST(ArenaMemoryResource, ServesAllocations) {
  MonotonicArena arena(1024);
  ArenaMemoryResource resource(&arena);
  std::pmr::polymorphic_allocator<double> allocator(&resource);
  double* values = allocator.allocate(8);
  EXPECT_TRUE(arena.owns(values));
  resource.reset();
  EXPECT_EQ(arena.used(), 0U);
}

TEST(PoolMemoryResource, ServesFixedSize) {
  FixedPool pool(128, 4);
  PoolMemoryResource resource(&pool);
  std::pmr::polymorphic_allocator<double> allocator(&resource);
  double* values = allocator.allocate(16);
  EXPECT_TRUE(pool.owns(values));
  allocator.deallocate(values, 16);
  EXPECT_EQ(pool.in_use(), 0U);
}
