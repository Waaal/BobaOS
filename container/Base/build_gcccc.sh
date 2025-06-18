#!/bin/bash

set -e

export CC_PREFIX="/usr/local"
export CC_TARGET=x86_64-elf
export PATH="$CC_PREFIX/bin:$PATH"

mkdir -p builds builds/binutils-2.44 builds/gcc-15.1.0

#curl -o builds/gcc-15.1.0.tar.xz https://ftp.gnu.org/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.xz
#curl -o builds/binutils-2.44.tar.xz https://ftp.gnu.org/gnu/binutils/binutils-2.44.tar.xz
#using a mirror because the above URLs are slow
curl -o builds/gcc-15.1.0.tar.xz https://mirror.easyname.at/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.xz
curl -o builds/binutils-2.44.tar.xz https://mirror.easyname.at/gnu/binutils/binutils-2.44.tar.xz

tar -xvJf builds/gcc-15.1.0.tar.xz
tar -xvJf builds/binutils-2.44.tar.xz

rm builds/gcc-15.1.0.tar.xz
rm builds/binutils-2.44.tar.xz

mkdir build-binutils
cd build-binutils
../binutils-2.44/configure --target=$CC_TARGET --prefix="$CC_PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install


cd ..

# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
which -- $CC_TARGET-as || echo $CC_TARGET-as is not in the PATH

mkdir build-gcc
cd build-gcc
../gcc-15.1.0/configure --target=$CC_TARGET --prefix="$CC_PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3