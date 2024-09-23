//===-- Implementation of fseeko ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fseeko.h"
#include "src/__support/File/file.h"

#include "src/__support/macros/config.h"
#include "src/errno/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, fseeko, (::FILE * stream, off_t offset, int whence)) {
  auto result =
      reinterpret_cast<LIBC_NAMESPACE::File *>(stream)->seek(offset, whence);
  if (!result.has_value()) {
    libc_errno = result.error();
    return -1;
  }
  return 0;
}

} // namespace LIBC_NAMESPACE_DECL
