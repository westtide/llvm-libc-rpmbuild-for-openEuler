//===-- Implementation header for tss_create --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_THREADS_TSS_CREATE_H
#define LLVM_LIBC_SRC_THREADS_TSS_CREATE_H

#include "src/__support/macros/config.h"
#include <threads.h>

namespace LIBC_NAMESPACE_DECL {

int tss_create(tss_t *key, tss_dtor_t dtor);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_THREADS_TSS_CREATE_H
