//===-- Unittests for dsubl -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SubTest.h"

#include "src/math/dsubl.h"

LIST_SUB_TESTS(double, long double, LIBC_NAMESPACE::dsubl)
