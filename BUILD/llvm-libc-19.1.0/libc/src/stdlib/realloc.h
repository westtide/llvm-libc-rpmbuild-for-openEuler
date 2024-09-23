//===-- Implementation header for realloc -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/macros/config.h"
#include <stddef.h>

#ifndef LLVM_LIBC_SRC_STDLIB_REALLOC_H
#define LLVM_LIBC_SRC_STDLIB_REALLOC_H

namespace LIBC_NAMESPACE_DECL {

void *realloc(void *ptr, size_t size);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_STDLIB_REALLOC_H
