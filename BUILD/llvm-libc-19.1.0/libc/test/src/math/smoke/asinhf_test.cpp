//===-- Unittests for asinhf ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hdr/math_macros.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/errno/libc_errno.h"
#include "src/math/asinhf.h"
#include "test/UnitTest/FPMatcher.h"
#include "test/UnitTest/Test.h"

#include <errno.h>
#include <stdint.h>

using LlvmLibcAsinhfTest = LIBC_NAMESPACE::testing::FPTest<float>;

TEST_F(LlvmLibcAsinhfTest, SpecialNumbers) {
  LIBC_NAMESPACE::libc_errno = 0;

  EXPECT_FP_EQ_ALL_ROUNDING(aNaN, LIBC_NAMESPACE::asinhf(aNaN));
  EXPECT_MATH_ERRNO(0);

  EXPECT_FP_EQ_ALL_ROUNDING(0.0f, LIBC_NAMESPACE::asinhf(0.0f));
  EXPECT_MATH_ERRNO(0);

  EXPECT_FP_EQ_ALL_ROUNDING(-0.0f, LIBC_NAMESPACE::asinhf(-0.0f));
  EXPECT_MATH_ERRNO(0);

  EXPECT_FP_EQ_ALL_ROUNDING(inf, LIBC_NAMESPACE::asinhf(inf));
  EXPECT_MATH_ERRNO(0);

  EXPECT_FP_EQ_ALL_ROUNDING(neg_inf, LIBC_NAMESPACE::asinhf(neg_inf));
  EXPECT_MATH_ERRNO(0);
}
