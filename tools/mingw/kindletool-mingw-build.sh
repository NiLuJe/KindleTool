#!/usr/bin/env bash
set -e

#
# KindleTool cross mingw-w64 buildscript
#
##

## NOTE: Getting a decent cross-toolchain is a bit of a chore...
##       Official releases are found @ https://sourceforge.net/projects/mingw-w64/files/
##       But currently, they only provide *native* binaries.
##       Historically, rubenvb provided binaries for cross-toolchains, but they're now horribly outdated.
## NOTE: Thankfully, there's http://sourceforge.net/projects/mingw-w64-dgn/ which does provide up to date binaries.
## NOTE: Alternatively, you could use MXE (http://mxe.cc) to build one yourself,
##       although it's currently only using GCC 5.5.0 & binutils 2.28 (but with the latest mingw-w64 release),
##       and has a bit too much dependencies for a headless box...
## NOTE: Or some other build script, like https://github.com/shinchiro/mpv-winbuild-cmake
## FIXME: Might need to symlink bcrypt.h to Bcrypt.h & windows.h to Windows.h to make libarchive happy...

# Remember where we are...
SCRIPT_NAME="${BASH_SOURCE[0]-${(%):-%x}}"
SCRIPT_BASE_DIR="$(readlink -f "${SCRIPT_NAME%/*}")"

# Make sure we're up to date
git pull

echo "* Setting environment up . . ."
echo ""
ARCH_FLAGS="-march=x86-64 -mtune=generic"
CROSS_TC="x86_64-w64-mingw32"
TC_BUILD_DIR="/home/niluje/Kindle/KTool_Static/MinGW/Build_W64"

export PATH="/home/niluje/x-tools/mingw64/install/bin:${PATH}"

BASE_CFLAGS="${ARCH_FLAGS} -O2 -pipe -fomit-frame-pointer"
export CFLAGS="${BASE_CFLAGS}"
export CXXFLAGS="${BASE_CFLAGS}"
BASE_CPPFLAGS="-isystem${TC_BUILD_DIR}/include"
export CPPFLAGS="${BASE_CPPFLAGS}"
BASE_LDFLAGS="-L${TC_BUILD_DIR}/lib -Wl,-O1 -Wl,--as-needed"
export LDFLAGS="${BASE_LDFLAGS}"

BASE_PKG_CONFIG_PATH="${TC_BUILD_DIR}/lib/pkgconfig"
BASE_PKG_CONFIG_LIBDIR="${TC_BUILD_DIR}/lib/pkgconfig"
export PKG_CONFIG_DIR=
export PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}"
export PKG_CONFIG_LIBDIR="${BASE_PKG_CONFIG_LIBDIR}"

## Go :)
## Get to our build dir
mkdir -p "${TC_BUILD_DIR}"
cd "${TC_BUILD_DIR}"

ZLIB_VER="1.2.11"
ZLIB_DIR="zlib-${ZLIB_VER}"
ZLIB_FILE="zlib${ZLIB_VER//.}.zip"
GMP_VER="6.2.1"
GMP_DIR="gmp-${GMP_VER%a}"
NETTLE_VER="3.6"
NETTLE_DIR="nettle-${NETTLE_VER}"
LIBARCHIVE_VER="3.5.0"
LIBARCHIVE_DIR="libarchive-${LIBARCHIVE_VER}"

if [[ ! -d "${ZLIB_DIR}" ]] ; then
	echo "* Building zlib . . ."
	echo ""
	if [[ ! -f "./${ZLIB_FILE}" ]] ; then
		wget -O "${ZLIB_FILE}" "http://zlib.net/${ZLIB_FILE}"
	fi
	unzip ./${ZLIB_FILE}
	cd ${ZLIB_DIR}
	patch -p1 < ../../../KindleTool/tools/mingw/zlib-1.2.7-mingw-makefile-fix.patch
	make -f win32/Makefile.gcc
	mkdir -p ${TC_BUILD_DIR}/include ${TC_BUILD_DIR}/bin ${TC_BUILD_DIR}/lib
	#cp -v zlib1.dll ${TC_BUILD_DIR}/bin
	cp -v zconf.h zlib.h ${TC_BUILD_DIR}/include
	cp -v libz.a ${TC_BUILD_DIR}/lib
	#cp -v libz.dll.a ${TC_BUILD_DIR}/lib
	cd ..
fi

# GMP
if [[ ! -d "${GMP_DIR}" ]] ; then
	echo "* Building ${GMP_DIR} . . ."
	echo ""
	if [[ ! -f "./${GMP_DIR}.tar.xz" ]] ; then
		wget -O "./${GMP_DIR}.tar.xz" "https://gmplib.org/download/gmp/${GMP_DIR}.tar.xz"
	fi
	tar -xvJf ./${GMP_DIR}.tar.xz
	cd ${GMP_DIR}
	autoreconf -fi
	libtoolize
	./configure --prefix="${TC_BUILD_DIR}" --host="${CROSS_TC}" --enable-static --disable-shared --disable-cxx
	make -j2
	make install
	cd ..
fi

# nettle
if [[ "${USE_STABLE_NETTLE}" == "true" ]] ; then
	if [[ ! -d "${NETTLE_DIR}" ]] ; then
		echo "* Building ${NETTLE_DIR} . . ."
		echo ""
		if [[ ! -f "./${NETTLE_DIR}.tar.gz" ]] ; then
			wget -O "./${NETTLE_DIR}.tar.gz" "http://www.lysator.liu.se/~nisse/archive/${NETTLE_DIR}.tar.gz"
		fi
		tar -xvzf ./${NETTLE_DIR}.tar.gz
		cd ${NETTLE_DIR}
		sed -e '/CFLAGS=/s: -ggdb3::' -e 's/solaris\*)/sunldsolaris*)/' -i configure.ac
		sed -i '/SUBDIRS/s/testsuite examples//' Makefile.in
		autoreconf -fi
		./configure --prefix="${TC_BUILD_DIR}" --libdir="${TC_BUILD_DIR}/lib" --host="${CROSS_TC}" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
		make -j2
		make install
		cd ..
	fi
else
	if [[ ! -d "nettle-git" ]] ; then
		echo "* Building nettle . . ."
		echo ""
		git clone https://git.lysator.liu.se/nettle/nettle.git nettle-git
		cd nettle-git
		sed -e '/CFLAGS=/s: -ggdb3::' -e 's/solaris\*)/sunldsolaris*)/' -i configure.ac
		sed -i '/SUBDIRS/s/testsuite examples//' Makefile.in
		# Fix MinGW builds...
		# shellcheck disable=SC2016
		sed -e 's#desdata$(EXEEXT)#desdata$(EXEEXT_FOR_BUILD)#g' -i Makefile.in
		sh ./.bootstrap
		./configure --prefix="${TC_BUILD_DIR}" --libdir="${TC_BUILD_DIR}/lib" --host="${CROSS_TC}" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
		make -j2
		make install
		cd ..
	fi
fi

# libarchive
if [[ "${USE_STABLE_LIBARCHIVE}" == "true" ]] ; then
	if [[ ! -d "${LIBARCHIVE_DIR}" ]] ; then
		echo "* Building ${LIBARCHIVE_DIR} . . ."
		echo ""
		if [[ ! -f "./${LIBARCHIVE_DIR}.tar.gz" ]] ; then
			wget -O "./${LIBARCHIVE_DIR}.tar.gz" "http://github.com/libarchive/libarchive/archive/v${LIBARCHIVE_VER}.tar.gz"
		fi
		tar -xvzf ./${LIBARCHIVE_DIR}.tar.gz
		cd ${LIBARCHIVE_DIR}
		./build/autogen.sh
		./configure --prefix="${TC_BUILD_DIR}" --host="${CROSS_TC}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --with-nettle --without-openssl --without-expat --without-xml2 --without-lz4 --without-zstd --disable-bsdcat --disable-bsdtar --disable-bsdcpio
		make -j2
		make install
		cd ..
	fi
else
	if [[ ! -d "libarchive-git" ]] ; then
		echo "* Building libarchive . . ."
		echo ""
		git clone https://github.com/libarchive/libarchive.git libarchive-git
		cd libarchive-git
		# Remove -Werror, there might be some warnings depending on the TC used...
		sed -e 's/-Werror //' -i ./Makefile.am
		./build/autogen.sh
		./configure --prefix="${TC_BUILD_DIR}" --host="${CROSS_TC}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --with-nettle --without-openssl --without-expat --without-xml2 --without-lz4 --without-zstd --disable-bsdcat --disable-bsdtar --disable-bsdcpio
		make -j2
		make install
		cd ..
	fi
fi

# Build KT package credits
cat > ../../CREDITS << EOF
* kindletool.exe: KindleTool, Copyright (C) 2011-2012 Yifan Lu & Copyright (C) 2012-2023 NiLuJe, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |->   zlib, Copyright (C) 1995-2018 Jean-loup Gailly and Mark Adler,
  |   Licensed under the zlib license (http://zlib.net/zlib_license.html)
  |   (http://zlib.net/)
  |
  |->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
  |   (http://libarchive.github.com/)
  |
  |->   GMP, GNU MP Library, Copyright 1991-2018 Free Software Foundation, Inc.,
  |   licensed under the GNU Lesser General Public License version 3+ (http://www.gnu.org/licenses/lgpl.html).
  |   (http://gmplib.org/)
  |
  |->   nettle, Copyright (C) 2001-2018 Niels Möller,
  |   licensed under the GNU Lesser General Public License version 2.1+ (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
  |   (http://www.lysator.liu.se/~nisse/nettle)
  |
  \`->   Built using MinGW-w64 and statically linked against the MinGW-w64 runtime, Copyright (C) 2009-2019 by the mingw-w64 project,
      Licensed mostly under the Zope Public License (ZPL) Version 2.1. (http://sourceforge.net/p/mingw-w64/code/HEAD/tree/stable/v3.x/COPYING.MinGW-w64-runtime/COPYING.MinGW-w64-runtime.txt)
      (http://mingw-w64.sourceforge.net/)
EOF

# KindleTool
echo "* Building KindleTool . . ."
echo ""
cd ../..
cd KindleTool/KindleTool
rm -rf lib includes
make clean
make mingw MINGW=true

# Package it
git log --stat --graph > ../../ChangeLog
./version.sh PMS STATIC
VER_FILE="VERSION"
VER_CURRENT="$(<${VER_FILE})"
# Strip the git commit
REV="${VER_CURRENT%%-*}"
#REV="${VER_CURRENT}"
cd ../..
cp -v KindleTool/KindleTool/MinGW/kindletool.exe ./kindletool.exe
cp -v KindleTool/README.md ./README
# Quick! Markdown => plaintext
sed -si 's/<b>//g;s/<\/b>//g;s/<i>//g;s/<\/i>//g;s/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
mv -v KindleTool/KindleTool/VERSION ./VERSION
# LF => CRLF...
unix2dos CREDITS README ChangeLog
7z a -tzip "kindletool-${REV}-mingw.zip" kindletool.exe CREDITS README ChangeLog VERSION
rm -f kindletool.exe CREDITS README ChangeLog VERSION
