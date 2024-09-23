//===-- Definition of type union sigval -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_TYPES_UNION_SIGVAL_H
#define LLVM_LIBC_TYPES_UNION_SIGVAL_H

union sigval {
  int sival_int;
  void *sival_ptr;
};

#endif // LLVM_LIBC_TYPES_UNION_SIGVAL_H
