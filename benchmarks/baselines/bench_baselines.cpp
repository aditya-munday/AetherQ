#include <cstddef>
#include <vector>

#include <Eigen/Dense>
#include <benchmark/benchmark.h>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/kernels/kernels.hpp"
#include "aetherq/kernels/rng.hpp"

using namespace aetherq;

namespace {

std::vector<double> make_values(std::size_t count) {
  std::vector<double> values(count, 0.0);
  Philox4x32::fill_uniform(4321, 1, 0, values);
  return values;
}

void aetherq_dot(benchmark::State& state) {
  const auto size = static_cast<std::size_t>(state.range(0));
  const std::vector<double> lhs = make_values(size);
  const std::vector<double> rhs = make_values(size + 1);
  double sink = 0.0;
  for (auto _ : state) {
    sink += optimized_dot(lhs, rhs);
  }
  benchmark::DoNotOptimize(sink);
}

void eigen_dot(benchmark::State& state) {
  const auto size = static_cast<Eigen::Index>(state.range(0));
  const std::vector<double> lhs_values = make_values(static_cast<std::size_t>(size));
  const std::vector<double> rhs_values = make_values(static_cast<std::size_t>(size));
  const Eigen::Map<const Eigen::VectorXd> lhs(lhs_values.data(), size);
  const Eigen::Map<const Eigen::VectorXd> rhs(rhs_values.data(), size);
  double sink = 0.0;
  for (auto _ : state) {
    sink += lhs.dot(rhs);
  }
  benchmark::DoNotOptimize(sink);
}

void aetherq_matrix_matrix(benchmark::State& state) {
  const auto dimension = static_cast<std::size_t>(state.range(0));
  const std::vector<double> lhs = make_values(dimension * dimension);
  const std::vector<double> rhs = make_values(dimension * dimension);
  std::vector<double> output(dimension * dimension, 0.0);
  for (auto _ : state) {
    optimized_matrix_matrix(lhs, dimension, dimension, rhs, dimension, output);
  }
  benchmark::DoNotOptimize(output);
}

void eigen_matrix_matrix(benchmark::State& state) {
  const auto dimension = static_cast<Eigen::Index>(state.range(0));
  const std::vector<double> lhs_values =
      make_values(static_cast<std::size_t>(dimension * dimension));
  const std::vector<double> rhs_values =
      make_values(static_cast<std::size_t>(dimension * dimension));
  const Eigen::Map<const Eigen::MatrixXd> lhs(lhs_values.data(), dimension, dimension);
  const Eigen::Map<const Eigen::MatrixXd> rhs(rhs_values.data(), dimension, dimension);
  Eigen::MatrixXd output(dimension, dimension);
  for (auto _ : state) {
    output.noalias() = lhs * rhs;
  }
  benchmark::DoNotOptimize(output);
}

}

BENCHMARK(aetherq_dot)->Range(64, 1 << 20);
BENCHMARK(eigen_dot)->Range(64, 1 << 20);
BENCHMARK(aetherq_matrix_matrix)->Range(16, 512);
BENCHMARK(eigen_matrix_matrix)->Range(16, 512);

BENCHMARK_MAIN();
