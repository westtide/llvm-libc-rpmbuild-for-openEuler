//===-- Implementation header for stdc_trailing_ones_ull --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_STDBIT_STDC_TRAILING_ONES_ULL_H
#define LLVM_LIBC_SRC_STDBIT_STDC_TRAILING_ONES_ULL_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

unsigned stdc_trailing_ones_ull(unsigned long long value);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_STDBIT_STDC_TRAILING_ONES_ULL_H
