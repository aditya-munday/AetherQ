#pragma once

#include <cstddef>

#include "aetherq/executor/executor.hpp"
#include "aetherq/runtime/config.hpp"
#include "aetherq/runtime/result.hpp"

namespace aetherq {

using IndexRangeBody = std::function<void(std::size_t begin, std::size_t end)>;

Result<void> parallel_for(Executor& executor, std::size_t count, ParallelForPolicy policy,
                          std::size_t grain, const IndexRangeBody& body);

}
