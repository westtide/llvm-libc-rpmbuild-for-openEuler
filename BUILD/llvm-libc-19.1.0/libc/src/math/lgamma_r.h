//===-- Implementation header for lgamma_r-----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_MATH_LGAMMA_R_H
#define LLVM_LIBC_SRC_MATH_LGAMMA_R_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

double lgamma_r(double x, int *signp);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_MATH_LGAMMA_R_H
