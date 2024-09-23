//===-- Differential test for fmodf ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "BinaryOpSingleOutputPerf.h"

#include "src/math/fmodf.h"

#include <math.h>

BINARY_OP_SINGLE_OUTPUT_PERF(float, float, LIBC_NAMESPACE::fmodf, ::fmodf,
                             "fmodf_perf.log")
