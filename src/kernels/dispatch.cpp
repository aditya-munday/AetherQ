#include "aetherq/kernels/dispatch.hpp"

namespace aetherq {

KernelSet select_kernels(Backend backend) {
  KernelSet set{};
  const bool optimized = backend == Backend::optimized;
  set.dot = optimized ? &optimized_dot : &scalar_dot;
  set.sum = optimized ? &optimized_sum : &scalar_sum;
  set.norm = optimized ? &optimized_norm : &scalar_norm;
  set.scale = optimized ? &optimized_scale : &scalar_scale;
  set.axpy = optimized ? &optimized_axpy : &scalar_axpy;
  set.matrix_vector = optimized ? &optimized_matrix_vector : &scalar_matrix_vector;
  set.matrix_matrix = optimized ? &optimized_matrix_matrix : &scalar_matrix_matrix;
  set.covariance = optimized ? &optimized_covariance : &scalar_covariance;
  set.weighted_sum = optimized ? &optimized_weighted_sum : &scalar_weighted_sum;
  set.exp_vector = optimized ? &optimized_exp : &scalar_exp;
  set.log_vector = optimized ? &optimized_log : &scalar_log;
  set.normal_cdf_vector = optimized ? &optimized_normal_cdf : &scalar_normal_cdf;
  return set;
}

}
