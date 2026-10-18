#pragma once

#include <cstddef>
#include <span>

namespace aetherq {

double scalar_dot(std::span<const double> lhs, std::span<const double> rhs);
double optimized_dot(std::span<const double> lhs, std::span<const double> rhs);

double scalar_sum(std::span<const double> values);
double optimized_sum(std::span<const double> values);

double scalar_norm(std::span<const double> values);
double optimized_norm(std::span<const double> values);

void scalar_scale(double factor, std::span<const double> input, std::span<double> output);
void optimized_scale(double factor, std::span<const double> input, std::span<double> output);

void scalar_axpy(double factor, std::span<const double> x, std::span<double> y);
void optimized_axpy(double factor, std::span<const double> x, std::span<double> y);

void scalar_matrix_vector(std::span<const double> matrix, std::size_t rows, std::size_t cols,
                          std::span<const double> vector, std::span<double> output);
void optimized_matrix_vector(std::span<const double> matrix, std::size_t rows, std::size_t cols,
                             std::span<const double> vector, std::span<double> output);

void scalar_matrix_matrix(std::span<const double> lhs, std::size_t lhs_rows, std::size_t shared,
                          std::span<const double> rhs, std::size_t rhs_cols,
                          std::span<double> output);
void optimized_matrix_matrix(std::span<const double> lhs, std::size_t lhs_rows, std::size_t shared,
                             std::span<const double> rhs, std::size_t rhs_cols,
                             std::span<double> output);

void scalar_covariance(std::span<const double> returns, std::size_t observations,
                       std::size_t assets, std::span<double> covariance);
void optimized_covariance(std::span<const double> returns, std::size_t observations,
                          std::size_t assets, std::span<double> covariance);

void scalar_weighted_sum(std::span<const double> values, std::span<const double> weights,
                         std::span<double> output);
void optimized_weighted_sum(std::span<const double> values, std::span<const double> weights,
                            std::span<double> output);

void scalar_exp(std::span<const double> input, std::span<double> output);
void optimized_exp(std::span<const double> input, std::span<double> output);

void scalar_log(std::span<const double> input, std::span<double> output);
void optimized_log(std::span<const double> input, std::span<double> output);

void scalar_normal_cdf(std::span<const double> input, std::span<double> output);
void optimized_normal_cdf(std::span<const double> input, std::span<double> output);

}
