#include "aetherq/executor/parallel_for.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace aetherq {

namespace {

std::vector<std::pair<std::size_t, std::size_t>> split_static(std::size_t count,
                                                              std::size_t parts) {
  std::vector<std::pair<std::size_t, std::size_t>> ranges;
  const std::size_t base = count / parts;
  const std::size_t remainder = count % parts;
  std::size_t offset = 0;
  for (std::size_t i = 0; i < parts; ++i) {
    const std::size_t length = base + (i < remainder ? 1 : 0);
    ranges.emplace_back(offset, offset + length);
    offset += length;
  }
  return ranges;
}

std::vector<std::pair<std::size_t, std::size_t>> split_fixed(std::size_t count, std::size_t grain) {
  std::vector<std::pair<std::size_t, std::size_t>> ranges;
  for (std::size_t offset = 0; offset < count; offset += grain) {
    ranges.emplace_back(offset, offset + grain < count ? offset + grain : count);
  }
  return ranges;
}

}

Result<void> parallel_for(Executor& executor, std::size_t count, ParallelForPolicy policy,
                          std::size_t grain, const IndexRangeBody& body) {
  if (count == 0) {
    return ok_result();
  }
  const std::size_t effective_grain = grain == 0 ? 1 : grain;
  const std::size_t workers = executor.worker_count();

  if (policy == ParallelForPolicy::guided) {
    auto cursor = std::make_shared<std::atomic<std::size_t>>(0);
    const std::size_t total_workers = workers == 0 ? 1 : workers;
    for (std::size_t worker = 0; worker < total_workers; ++worker) {
      auto submitted = executor.submit([&body, count, effective_grain, cursor] {
        for (;;) {
          const std::size_t begin = cursor->fetch_add(effective_grain);
          if (begin >= count) {
            return;
          }
          const std::size_t end = begin + effective_grain < count ? begin + effective_grain : count;
          body(begin, end);
        }
      });
      if (!submitted) {
        return submitted.error();
      }
    }
    return executor.wait();
  }

  std::vector<std::pair<std::size_t, std::size_t>> ranges =
      policy == ParallelForPolicy::fixed_grain ? split_fixed(count, effective_grain)
                                               : split_static(count, workers == 0 ? 1 : workers);
  for (const auto& range : ranges) {
    if (range.first >= range.second) {
      continue;
    }
    auto submitted = executor.submit([&body, range] { body(range.first, range.second); });
    if (!submitted) {
      return submitted.error();
    }
  }
  return executor.wait();
}

}
