Name:           llvm-libc
Version:        19.1.0
Release:        1%{?dist}
Summary:        LLVM C standard library implementation

License:        Apache License v2.0 with LLVM Exceptions
URL:            https://libc.llvm.org/
Source0:        llvm-libc-%{version}.tar.gz

BuildRequires:  llvm, clang, cmake, python3-ninja 

%description
LLVM libc is an implementation of the C standard library optimized for use with LLVM and Clang. It aims to provide a fully compliant C17 library with better performance and more opportunities for whole-program optimization when used with LLVM-based compilers.

%prep
%setup -q
%global debug_package %{nil}
%build
# 创建独立的构建目录build,需要在build内配置ninja
mkdir -p build
cd build

%ifarch x86_64
%define target_triple x86_64-openEuler-linux-gnu
%endif
%ifarch aarch64
%define target_triple aarch64-openEuler-linux-gnu
%endif
%ifarch riscv64
%define target_triple riscv64-openEuler-linux-gnu
%endif

cmake ../runtimes -G Ninja \
      -DLLVM_ENABLE_RUNTIMES="libc"  \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Release  \
      -DLIBC_TARGET_TRIPLE=%{target_triple} \
      -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr/local/lib


ninja libc

%check
cd %{_builddir}/llvm-libc-%{version}/build
ninja check-libc


%install
mkdir -p %{buildroot}/usr/local/lib
cp %{_builddir}/llvm-libc-%{version}/build/libc/lib/libllvmlibc.a %{buildroot}/usr/local/lib/


# 输出文件路径告知用户
echo "Static library installed at /usr/local/lib/libllvmlibc.a and %{buildroot}/usr/local/lib/libllvmlibc.a"

%files
%defattr(-,root,root,-)
/usr/local/lib/libllvmlibc.a


%changelog
* Wed Sep 18 2024 westtide <tocokeo@outlook.com> - 19.1.0-1
- Initial package of llvm-libc for openEuler
