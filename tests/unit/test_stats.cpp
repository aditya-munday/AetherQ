#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "aetherq/measurement/stats.hpp"

using namespace aetherq;

TEST(Stats, EmptyInputProducesEmptySummary) {
  const SummaryStats stats = summarize({});
  EXPECT_EQ(stats.sample_count, 0U);
}

TEST(Stats, ComputesBasicStatistics) {
  const std::vector<double> samples{1.0, 2.0, 3.0, 4.0, 5.0};
  const SummaryStats stats = summarize(samples);
  EXPECT_EQ(stats.sample_count, 5U);
  EXPECT_DOUBLE_EQ(stats.mean, 3.0);
  EXPECT_DOUBLE_EQ(stats.median, 3.0);
  EXPECT_DOUBLE_EQ(stats.min, 1.0);
  EXPECT_DOUBLE_EQ(stats.max, 5.0);
  EXPECT_DOUBLE_EQ(stats.stddev, std::sqrt(2.5));
}

TEST(Stats, MadIsRobustToOutliers) {
  const std::vector<double> samples{1.0, 1.0, 1.0, 1.0, 100.0};
  const SummaryStats stats = summarize(samples);
  EXPECT_DOUBLE_EQ(stats.median, 1.0);
  EXPECT_DOUBLE_EQ(stats.mad, 0.0);
}

TEST(Stats, PercentilesOnlyWhenSampleCountSupports) {
  const std::vector<double> small(10, 1.0);
  const SummaryStats small_stats = summarize(small);
  EXPECT_FALSE(small_stats.p50.has_value());
  EXPECT_FALSE(small_stats.p95.has_value());

  std::vector<double> medium(50);
  for (std::size_t i = 0; i < medium.size(); ++i) {
    medium[i] = static_cast<double>(i);
  }
  const SummaryStats medium_stats = summarize(medium);
  ASSERT_TRUE(medium_stats.p50.has_value());
  ASSERT_TRUE(medium_stats.p95.has_value());
  EXPECT_FALSE(medium_stats.p99.has_value());

  std::vector<double> large(2000);
  for (std::size_t i = 0; i < large.size(); ++i) {
    large[i] = static_cast<double>(i);
  }
  const SummaryStats large_stats = summarize(large);
  EXPECT_TRUE(large_stats.p99.has_value());
  EXPECT_TRUE(large_stats.p999.has_value());
}

TEST(Stats, PercentileInterpolates) {
  std::vector<double> sorted{0.0, 10.0, 20.0, 30.0, 40.0};
  const auto p50 = percentile(sorted, 0.5);
  ASSERT_TRUE(p50.has_value());
  EXPECT_DOUBLE_EQ(*p50, 20.0);
  const auto p25 = percentile(sorted, 0.25);
  ASSERT_TRUE(p25.has_value());
  EXPECT_DOUBLE_EQ(*p25, 10.0);
}

TEST(Stats, RejectsInvalidQuantile) {
  std::vector<double> sorted{1.0, 2.0};
  EXPECT_FALSE(percentile(sorted, -0.1).has_value());
  EXPECT_FALSE(percentile(sorted, 1.1).has_value());
}

TEST(Stats, CoefficientOfVariationIsScaleFree) {
  const std::vector<double> samples{10.0, 12.0, 14.0};
  const SummaryStats stats = summarize(samples);
  EXPECT_NEAR(coefficient_of_variation(stats), 2.0 / 12.0, 1e-12);
}

TEST(Stats, DetectsOverlappingDistributions) {
  const std::vector<double> a{1.0, 2.0, 3.0, 4.0, 5.0};
  const std::vector<double> b{2.0, 3.0, 4.0, 5.0, 6.0};
  EXPECT_TRUE(distributions_overlap(summarize(a), summarize(b)));

  const std::vector<double> c{100.0, 101.0, 102.0, 103.0, 104.0};
  EXPECT_FALSE(distributions_overlap(summarize(a), summarize(c)));
}
