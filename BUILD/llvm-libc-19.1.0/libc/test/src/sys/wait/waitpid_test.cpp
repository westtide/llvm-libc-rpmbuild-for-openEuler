//===-- Unittests for waitpid ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/sys/wait/waitpid.h"
#include "test/UnitTest/ErrnoSetterMatcher.h"
#include "test/UnitTest/Test.h"

#include <errno.h>
#include <sys/wait.h>

// The test here is a simpl test for WNOHANG functionality. For a more
// involved test, look at fork_test.

TEST(LlvmLibcWaitPidTest, NoHangTest) {
  using LIBC_NAMESPACE::testing::ErrnoSetterMatcher::Fails;
  int status;
  ASSERT_THAT(LIBC_NAMESPACE::waitpid(-1, &status, WNOHANG), Fails(ECHILD));
}
