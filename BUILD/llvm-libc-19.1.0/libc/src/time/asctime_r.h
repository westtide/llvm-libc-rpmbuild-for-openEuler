//===-- Implementation header of asctime_r ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_TIME_ASCTIME_R_H
#define LLVM_LIBC_SRC_TIME_ASCTIME_R_H

#include "src/__support/macros/config.h"
#include <time.h>

namespace LIBC_NAMESPACE_DECL {

char *asctime_r(const struct tm *timeptr, char *buffer);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_TIME_ASCTIME_R_H
