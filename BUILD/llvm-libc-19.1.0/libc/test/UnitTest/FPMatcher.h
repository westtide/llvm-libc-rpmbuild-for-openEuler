//===-- FPMatchers.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TEST_UNITTEST_FPMATCHER_H
#define LLVM_LIBC_TEST_UNITTEST_FPMATCHER_H

#include "src/__support/CPP/array.h"
#include "src/__support/CPP/type_traits.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/fpbits_str.h"
#include "src/__support/macros/config.h"
#include "test/UnitTest/RoundingModeUtils.h"
#include "test/UnitTest/StringUtils.h"
#include "test/UnitTest/Test.h"

#include "hdr/math_macros.h"

namespace LIBC_NAMESPACE_DECL {
namespace testing {

template <typename T, TestCond Condition> class FPMatcher : public Matcher<T> {
  static_assert(cpp::is_floating_point_v<T>,
                "FPMatcher can only be used with floating point values.");
  static_assert(Condition == TestCond::EQ || Condition == TestCond::NE,
                "Unsupported FPMatcher test condition.");

  T expected;
  T actual;

public:
  FPMatcher(T expectedValue) : expected(expectedValue) {}

  bool match(T actualValue) {
    actual = actualValue;
    fputil::FPBits<T> actualBits(actual), expectedBits(expected);
    if (Condition == TestCond::EQ)
      return (actualBits.is_nan() && expectedBits.is_nan()) ||
             (actualBits.uintval() == expectedBits.uintval());

    // If condition == TestCond::NE.
    if (actualBits.is_nan())
      return !expectedBits.is_nan();
    return expectedBits.is_nan() ||
           (actualBits.uintval() != expectedBits.uintval());
  }

  void explainError() override {
    tlog << "Expected floating point value: "
         << str(fputil::FPBits<T>(expected)) << '\n';
    tlog << "Actual floating point value: " << str(fputil::FPBits<T>(actual))
         << '\n';
  }
};

template <TestCond C, typename T> FPMatcher<T, C> getMatcher(T expectedValue) {
  return FPMatcher<T, C>(expectedValue);
}

template <typename T> struct FPTest : public Test {
  using FPBits = LIBC_NAMESPACE::fputil::FPBits<T>;
  using StorageType = typename FPBits::StorageType;
  static constexpr StorageType STORAGE_MAX =
      LIBC_NAMESPACE::cpp::numeric_limits<StorageType>::max();
  static constexpr T zero = FPBits::zero(Sign::POS).get_val();
  static constexpr T neg_zero = FPBits::zero(Sign::NEG).get_val();
  static constexpr T aNaN = FPBits::quiet_nan().get_val();
  static constexpr T sNaN = FPBits::signaling_nan().get_val();
  static constexpr T inf = FPBits::inf(Sign::POS).get_val();
  static constexpr T neg_inf = FPBits::inf(Sign::NEG).get_val();
  static constexpr T min_normal = FPBits::min_normal().get_val();
  static constexpr T max_normal = FPBits::max_normal(Sign::POS).get_val();
  static constexpr T neg_max_normal = FPBits::max_normal(Sign::NEG).get_val();
  static constexpr T min_denormal = FPBits::min_subnormal().get_val();
  static constexpr T max_denormal = FPBits::max_subnormal().get_val();

  static constexpr int N_ROUNDING_MODES = 4;
  static constexpr fputil::testing::RoundingMode ROUNDING_MODES[4] = {
      fputil::testing::RoundingMode::Nearest,
      fputil::testing::RoundingMode::Upward,
      fputil::testing::RoundingMode::Downward,
      fputil::testing::RoundingMode::TowardZero,
  };
};

} // namespace testing
} // namespace LIBC_NAMESPACE_DECL

#define DECLARE_SPECIAL_CONSTANTS(T)                                           \
  using FPBits = LIBC_NAMESPACE::fputil::FPBits<T>;                            \
  using StorageType = typename FPBits::StorageType;                            \
                                                                               \
  static constexpr StorageType STORAGE_MAX =                                   \
      LIBC_NAMESPACE::cpp::numeric_limits<StorageType>::max();                 \
  const T zero = FPBits::zero(Sign::POS).get_val();                            \
  const T neg_zero = FPBits::zero(Sign::NEG).get_val();                        \
  const T aNaN = FPBits::quiet_nan(Sign::POS).get_val();                       \
  const T neg_aNaN = FPBits::quiet_nan(Sign::NEG).get_val();                   \
  const T sNaN = FPBits::signaling_nan(Sign::POS).get_val();                   \
  const T neg_sNaN = FPBits::signaling_nan(Sign::NEG).get_val();               \
  const T inf = FPBits::inf(Sign::POS).get_val();                              \
  const T neg_inf = FPBits::inf(Sign::NEG).get_val();                          \
  const T min_normal = FPBits::min_normal().get_val();                         \
  const T max_normal = FPBits::max_normal(Sign::POS).get_val();                \
  const T neg_max_normal = FPBits::max_normal(Sign::NEG).get_val();            \
  const T min_denormal = FPBits::min_subnormal(Sign::POS).get_val();           \
  const T neg_min_denormal = FPBits::min_subnormal(Sign::NEG).get_val();       \
  const T max_denormal = FPBits::max_subnormal().get_val();                    \
  static constexpr int UNKNOWN_MATH_ROUNDING_DIRECTION = 99;                   \
  static constexpr LIBC_NAMESPACE::cpp::array<int, 6>                          \
      MATH_ROUNDING_DIRECTIONS_INCLUDING_UNKNOWN = {                           \
          FP_INT_UPWARD,     FP_INT_DOWNWARD,                                  \
          FP_INT_TOWARDZERO, FP_INT_TONEARESTFROMZERO,                         \
          FP_INT_TONEAREST,  UNKNOWN_MATH_ROUNDING_DIRECTION,                  \
  };

#define EXPECT_FP_EQ(expected, actual)                                         \
  EXPECT_THAT(actual, LIBC_NAMESPACE::testing::getMatcher<                     \
                          LIBC_NAMESPACE::testing::TestCond::EQ>(expected))

#define TEST_FP_EQ(expected, actual)                                           \
  LIBC_NAMESPACE::testing::getMatcher<LIBC_NAMESPACE::testing::TestCond::EQ>(  \
      expected)                                                                \
      .match(actual)

#define EXPECT_FP_IS_NAN(actual) EXPECT_TRUE((actual) != (actual))

#define ASSERT_FP_EQ(expected, actual)                                         \
  ASSERT_THAT(actual, LIBC_NAMESPACE::testing::getMatcher<                     \
                          LIBC_NAMESPACE::testing::TestCond::EQ>(expected))

#define EXPECT_FP_NE(expected, actual)                                         \
  EXPECT_THAT(actual, LIBC_NAMESPACE::testing::getMatcher<                     \
                          LIBC_NAMESPACE::testing::TestCond::NE>(expected))

#define ASSERT_FP_NE(expected, actual)                                         \
  ASSERT_THAT(actual, LIBC_NAMESPACE::testing::getMatcher<                     \
                          LIBC_NAMESPACE::testing::TestCond::NE>(expected))

#define EXPECT_MATH_ERRNO(expected)                                            \
  do {                                                                         \
    if (math_errhandling & MATH_ERRNO) {                                       \
      int actual = LIBC_NAMESPACE::libc_errno;                                 \
      LIBC_NAMESPACE::libc_errno = 0;                                          \
      EXPECT_EQ(actual, expected);                                             \
    }                                                                          \
  } while (0)

#define ASSERT_MATH_ERRNO(expected)                                            \
  do {                                                                         \
    if (math_errhandling & MATH_ERRNO) {                                       \
      int actual = LIBC_NAMESPACE::libc_errno;                                 \
      LIBC_NAMESPACE::libc_errno = 0;                                          \
      ASSERT_EQ(actual, expected);                                             \
    }                                                                          \
  } while (0)

#define EXPECT_FP_EXCEPTION(expected)                                          \
  do {                                                                         \
    if (math_errhandling & MATH_ERREXCEPT) {                                   \
      EXPECT_EQ(LIBC_NAMESPACE::fputil::test_except(FE_ALL_EXCEPT) &           \
                    ((expected) ? (expected) : FE_ALL_EXCEPT),                 \
                (expected));                                                   \
    }                                                                          \
  } while (0)

#define ASSERT_FP_EXCEPTION(expected)                                          \
  do {                                                                         \
    if (math_errhandling & MATH_ERREXCEPT) {                                   \
      ASSERT_EQ(LIBC_NAMESPACE::fputil::test_except(FE_ALL_EXCEPT) &           \
                    ((expected) ? (expected) : FE_ALL_EXCEPT),                 \
                (expected));                                                   \
    }                                                                          \
  } while (0)

#define EXPECT_FP_EQ_WITH_EXCEPTION(expected_val, actual_val, expected_except) \
  do {                                                                         \
    LIBC_NAMESPACE::fputil::clear_except(FE_ALL_EXCEPT);                       \
    EXPECT_FP_EQ(expected_val, actual_val);                                    \
    EXPECT_FP_EXCEPTION(expected_except);                                      \
  } while (0)

#define EXPECT_FP_IS_NAN_WITH_EXCEPTION(actual_val, expected_except)           \
  do {                                                                         \
    LIBC_NAMESPACE::fputil::clear_except(FE_ALL_EXCEPT);                       \
    EXPECT_FP_IS_NAN(actual_val);                                              \
    EXPECT_FP_EXCEPTION(expected_except);                                      \
  } while (0)

#define EXPECT_FP_EQ_ALL_ROUNDING(expected, actual)                            \
  do {                                                                         \
    using namespace LIBC_NAMESPACE::fputil::testing;                           \
    ForceRoundingMode __r1(RoundingMode::Nearest);                             \
    if (__r1.success) {                                                        \
      EXPECT_FP_EQ((expected), (actual));                                      \
    }                                                                          \
    ForceRoundingMode __r2(RoundingMode::Upward);                              \
    if (__r2.success) {                                                        \
      EXPECT_FP_EQ((expected), (actual));                                      \
    }                                                                          \
    ForceRoundingMode __r3(RoundingMode::Downward);                            \
    if (__r3.success) {                                                        \
      EXPECT_FP_EQ((expected), (actual));                                      \
    }                                                                          \
    ForceRoundingMode __r4(RoundingMode::TowardZero);                          \
    if (__r4.success) {                                                        \
      EXPECT_FP_EQ((expected), (actual));                                      \
    }                                                                          \
  } while (0)

#define EXPECT_FP_EQ_ROUNDING_MODE(expected, actual, rounding_mode)            \
  do {                                                                         \
    using namespace LIBC_NAMESPACE::fputil::testing;                           \
    ForceRoundingMode __r((rounding_mode));                                    \
    if (__r.success) {                                                         \
      EXPECT_FP_EQ((expected), (actual));                                      \
    }                                                                          \
  } while (0)

#define EXPECT_FP_EQ_ROUNDING_NEAREST(expected, actual)                        \
  EXPECT_FP_EQ_ROUNDING_MODE((expected), (actual), RoundingMode::Nearest)

#define EXPECT_FP_EQ_ROUNDING_UPWARD(expected, actual)                         \
  EXPECT_FP_EQ_ROUNDING_MODE((expected), (actual), RoundingMode::Upward)

#define EXPECT_FP_EQ_ROUNDING_DOWNWARD(expected, actual)                       \
  EXPECT_FP_EQ_ROUNDING_MODE((expected), (actual), RoundingMode::Downward)

#define EXPECT_FP_EQ_ROUNDING_TOWARD_ZERO(expected, actual)                    \
  EXPECT_FP_EQ_ROUNDING_MODE((expected), (actual), RoundingMode::TowardZero)

#define EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_MODE(                             \
    expected, actual, expected_except, rounding_mode)                          \
  do {                                                                         \
    using namespace LIBC_NAMESPACE::fputil::testing;                           \
    ForceRoundingMode __r((rounding_mode));                                    \
    if (__r.success) {                                                         \
      LIBC_NAMESPACE::fputil::clear_except(FE_ALL_EXCEPT);                     \
      EXPECT_FP_EQ((expected), (actual));                                      \
      EXPECT_FP_EXCEPTION(expected_except);                                    \
    }                                                                          \
  } while (0)

#define EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_NEAREST(expected, actual,         \
                                                     expected_except)          \
  EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_MODE(                                   \
      (expected), (actual), (expected_except), RoundingMode::Nearest)

#define EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_UPWARD(expected, actual,          \
                                                    expected_except)           \
  EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_MODE(                                   \
      (expected), (actual), (expected_except), RoundingMode::Upward)

#define EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_DOWNWARD(expected, actual,        \
                                                      expected_except)         \
  EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_MODE(                                   \
      (expected), (actual), (expected_except), RoundingMode::Downward)

#define EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_TOWARD_ZERO(expected, actual,     \
                                                         expected_except)      \
  EXPECT_FP_EQ_WITH_EXCEPTION_ROUNDING_MODE(                                   \
      (expected), (actual), (expected_except), RoundingMode::TowardZero)

#endif // LLVM_LIBC_TEST_UNITTEST_FPMATCHER_H
