//===-- Implementation header for getpayload --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_MATH_GETPAYLOAD_H
#define LLVM_LIBC_SRC_MATH_GETPAYLOAD_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

double getpayload(const double *x);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_MATH_GETPAYLOAD_H
