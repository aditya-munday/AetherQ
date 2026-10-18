#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace aetherq::cli {

struct BenchAllocationOptions {
  std::size_t items{100000};
  std::string allocator{};
  bool json_output{false};
  std::size_t repetitions{5};
  std::size_t warmup{1};
  std::size_t seed{20260101};
};

struct BenchScalingOptions {
  std::string workload{"monte_carlo"};
  std::vector<std::size_t> thread_counts{1, 2, 4};
  std::size_t paths{100000};
  std::size_t options{100000};
  std::size_t assets{32};
  std::size_t observations{1024};
  std::string backend{"scalar"};
  bool json_output{false};
  std::size_t repetitions{5};
  std::size_t warmup{1};
  std::size_t seed{20260101};
};

struct BenchKernelsOptions {
  std::string filter;
  bool json_output{false};
};

int command_bench_allocation(const BenchAllocationOptions& options);
int command_bench_scaling(const BenchScalingOptions& options);
int command_bench_kernels(const BenchKernelsOptions& options);

}
