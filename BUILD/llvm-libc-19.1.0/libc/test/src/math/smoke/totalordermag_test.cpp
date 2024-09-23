//===-- Unittests for totalordermag ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TotalOrderMagTest.h"

#include "src/math/totalordermag.h"

LIST_TOTALORDERMAG_TESTS(double, LIBC_NAMESPACE::totalordermag)
