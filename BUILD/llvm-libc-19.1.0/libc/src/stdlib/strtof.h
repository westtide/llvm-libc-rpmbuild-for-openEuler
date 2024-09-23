//===-- Implementation header for strtof ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_STDLIB_STRTOF_H
#define LLVM_LIBC_SRC_STDLIB_STRTOF_H

#include "src/__support/macros/config.h"

namespace LIBC_NAMESPACE_DECL {

float strtof(const char *__restrict str, char **__restrict str_end);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_STDLIB_STRTOF_H
