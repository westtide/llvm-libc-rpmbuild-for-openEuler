//===-- Self contained C++ type_traits --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC___SUPPORT_CPP_TYPE_TRAITS_H
#define LLVM_LIBC_SRC___SUPPORT_CPP_TYPE_TRAITS_H

#include "src/__support/CPP/type_traits/add_lvalue_reference.h"
#include "src/__support/CPP/type_traits/add_pointer.h"
#include "src/__support/CPP/type_traits/add_rvalue_reference.h"
#include "src/__support/CPP/type_traits/aligned_storage.h"
#include "src/__support/CPP/type_traits/bool_constant.h"
#include "src/__support/CPP/type_traits/conditional.h"
#include "src/__support/CPP/type_traits/decay.h"
#include "src/__support/CPP/type_traits/enable_if.h"
#include "src/__support/CPP/type_traits/false_type.h"
#include "src/__support/CPP/type_traits/integral_constant.h"
#include "src/__support/CPP/type_traits/invoke.h"
#include "src/__support/CPP/type_traits/invoke_result.h"
#include "src/__support/CPP/type_traits/is_arithmetic.h"
#include "src/__support/CPP/type_traits/is_array.h"
#include "src/__support/CPP/type_traits/is_base_of.h"
#include "src/__support/CPP/type_traits/is_class.h"
#include "src/__support/CPP/type_traits/is_const.h"
#include "src/__support/CPP/type_traits/is_constant_evaluated.h"
#include "src/__support/CPP/type_traits/is_convertible.h"
#include "src/__support/CPP/type_traits/is_destructible.h"
#include "src/__support/CPP/type_traits/is_enum.h"
#include "src/__support/CPP/type_traits/is_fixed_point.h"
#include "src/__support/CPP/type_traits/is_floating_point.h"
#include "src/__support/CPP/type_traits/is_function.h"
#include "src/__support/CPP/type_traits/is_integral.h"
#include "src/__support/CPP/type_traits/is_lvalue_reference.h"
#include "src/__support/CPP/type_traits/is_member_pointer.h"
#include "src/__support/CPP/type_traits/is_null_pointer.h"
#include "src/__support/CPP/type_traits/is_object.h"
#include "src/__support/CPP/type_traits/is_pointer.h"
#include "src/__support/CPP/type_traits/is_reference.h"
#include "src/__support/CPP/type_traits/is_rvalue_reference.h"
#include "src/__support/CPP/type_traits/is_same.h"
#include "src/__support/CPP/type_traits/is_scalar.h"
#include "src/__support/CPP/type_traits/is_signed.h"
#include "src/__support/CPP/type_traits/is_trivially_constructible.h"
#include "src/__support/CPP/type_traits/is_trivially_copyable.h"
#include "src/__support/CPP/type_traits/is_trivially_destructible.h"
#include "src/__support/CPP/type_traits/is_union.h"
#include "src/__support/CPP/type_traits/is_unsigned.h"
#include "src/__support/CPP/type_traits/is_void.h"
#include "src/__support/CPP/type_traits/make_signed.h"
#include "src/__support/CPP/type_traits/make_unsigned.h"
#include "src/__support/CPP/type_traits/remove_all_extents.h"
#include "src/__support/CPP/type_traits/remove_cv.h"
#include "src/__support/CPP/type_traits/remove_cvref.h"
#include "src/__support/CPP/type_traits/remove_extent.h"
#include "src/__support/CPP/type_traits/remove_reference.h"
#include "src/__support/CPP/type_traits/true_type.h"
#include "src/__support/CPP/type_traits/type_identity.h"
#include "src/__support/CPP/type_traits/void_t.h"

#endif // LLVM_LIBC_SRC___SUPPORT_CPP_TYPE_TRAITS_H
