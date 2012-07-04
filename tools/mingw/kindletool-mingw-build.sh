#! /bin/bash -e
#
# KindleTool cross mingw buildscript
#
##

## Install/Setup CrossTool-NG
Build_CT-NG() {
	echo "* Building CrossTool-NG . . ."
	echo ""

	# Get out of our git tree
	cd ../../..

	# Make us a dedicated sysroot
	mkdir -p MinGW
	cd MinGW

	mkdir -p CT-NG
	cd CT-NG
	hg clone http://crosstool-ng.org/hg/crosstool-ng .
	# Bump MinGW API & RT to latest versions
	# FIXME: Broken right now, cf. http://sourceforge.net/tracker/?func=detail&atid=102435&aid=3441135&group_id=2435
	patch -p1 < ../../KindleTool/tools/mingw/ct-ng-mingw-vbump.patch
	# The default versions build okay, but let's use mingw-w64 instead... (http://mingw-w64.sourceforge.net/)

	./bootstrap
	./configure --prefix=/home/niluje/Kindle/KTool_Static/MinGW/CT
	make
	make install
	export PATH="${PATH}:/home/niluje/Kindle/KTool_Static/MinGW/CT/bin"

	cd ..
	mkdir -p Build_TC
	cd Build_TC

	ct-ng distclean
	unset CFLAGS CXXFLAGS LDFLAGS

	cp ../../KindleTool/tools/mingw/ct-ng_mingw_dot.config .config
	ct-ng oldconfig
	#ct-ng menuconfig

	## Config:
	cat << EOF

	CT-NG Config Overview:

	* Paths >
	EXPERIMENTAL: [*]
	Parallel jobs: 3

	* Target >
	Arch: x86
	Bitness: 32-bit
	Arch level: i686
	CPU: i686
	Tune: i686
	CFLAGS: -O2 -fomit-frame-pointer -pipe

	* TC >
	Tuple's vendor: pc

	* OS >
	Target: mingw32
	Win API: 3.17-2

	* Binary >
	Format: ELF			# Don't mind this, we really will end up with Win PE binaries ;)
	Binutils: 2.22
	Linkers to enable: ld

	* C Compiler >
	Type: gcc
	Version: 4.6.3
	Additional Lang: C++
	Link lstdc++ statically
	Enable GRAPHITE
	Enable LTO
	Opt gcc libs for size [ ]
	Use __cxa_atexit
	<M> sjlj
	<M> 128-bit long doubles

	* C library >
	Type: mingw
	MinGW RT: 3.20-2
	Threading: win32

EOF
	##

	nice ct-ng build
}

## I'd use this TC: https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/rubenvb/release/
## but they're actually for an x86_64 Linux host, not x86, so, fallback to the automated builds on my x86 box...

echo "* Setting environment up . . ."
echo ""
ARCH_FLAGS="-march=i686 -mtune=i686"
CROSS_TC="i686-w64-mingw32"
TC_BUILD_DIR="/home/niluje/Kindle/KTool_Static/MinGW/Build_W32"

export PATH="/home/niluje/x-tools/mingw32/bin:${PATH}"

BASE_CFLAGS="-O2 -ffast-math ${ARCH_FLAGS} -pipe -fomit-frame-pointer"
export CFLAGS="${BASE_CFLAGS}"
export CXXFLAGS="${BASE_CFLAGS}"
BASE_CPPFLAGS="-I${TC_BUILD_DIR}/include"
export CPPFLAGS="${BASE_CPPFLAGS}"
BASE_LDFLAGS="-L${TC_BUILD_DIR}/lib -Wl,-O1 -Wl,--as-needed"
export LDFLAGS="${BASE_LDFLAGS}"

## Go :)
## Get to our build dir
mkdir -p "${TC_BUILD_DIR}"
cd "${TC_BUILD_DIR}"

if [[ ! -d "zlib-1.2.7" ]] ; then
	echo "* Building zlib . . ."
	echo ""
	if [[ ! -f "./zlib127.zip" ]] ; then
		wget -O "zlib127.zip" "http://zlib.net/zlib127.zip"
	fi
	unzip ./zlib127.zip
	cd zlib-1.2.7
	patch -p1 < ../../../KindleTool/tools/mingw/zlib-1.2.7-mingw-makefile-fix.patch
	make -f win32/Makefile.gcc
	mkdir -p ../include ../bin ../lib
	#cp -v zlib1.dll ../bin
	cp -v zconf.h zlib.h ../include
	cp -v libz.a ../lib
	#cp -v libz.dll.a ../lib
	cd ..
fi

if [[ ! -d "openssl-1.0.1c" ]] ; then
	echo "* Building OpenSSL 1 . . ."
	echo ""
	if [[ ! -f "./openssl-1.0.1c.tar.gz" ]] ; then
		wget -O "./openssl-1.0.1c.tar.gz" "http://www.openssl.org/source/openssl-1.0.1c.tar.gz"
	fi
	tar -xvzf ./openssl-1.0.1c.tar.gz
	cd openssl-1.0.1c
	export CROSS_COMPILE="${CROSS_TC}-"
	export CFLAGS="${CPPFLAGS} ${BASE_CFLAGS} -fno-strict-aliasing"
	export CXXFLAGS="${BASE_CFLAGS} -fno-strict-aliasing"
	rm -f Makefile
	patch -p0 < /usr/portage/dev-libs/openssl/files/openssl-1.0.0a-ldflags.patch
	patch -p0 < /usr/portage/dev-libs/openssl/files/openssl-1.0.0d-fbsd-amd64.patch
	patch -p0 < /usr/portage/dev-libs/openssl/files/openssl-1.0.0d-windres.patch
	patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-1.0.0h-pkg-config.patch
	patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-1.0.1-parallel-build.patch
	patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-1.0.1-x32.patch
	#patch -p0 < /usr/portage/dev-libs/openssl/files/openssl-1.0.1-ipv6.patch	# Not MingW friendly
	sed -i -e '/DIRS/s: fips : :g' -e '/^MANSUFFIX/s:=.*:=ssl:' -e "/^MAKEDEPPROG/s:=.*:=${CROSS_TC}-gcc:" -e '/^install:/s:install_docs::' Makefile.org
	sed -i '/^SET_X/s:=.*:=set -x:' Makefile.shared
	cp /usr/portage/dev-libs/openssl/files/gentoo.config-1.0.0 gentoo.config
	chmod a+rx gentoo.config
	sed -i '1s,^:$,#!/usr/bin/perl,' Configure
	#unset CROSS_COMPILE
	./Configure mingw -DL_ENDIAN ${BASE_CFLAGS} -fno-strict-aliasing enable-camellia enable-mdc2 enable-tlsext enable-zlib --prefix=${TC_BUILD_DIR} --openssldir=${TC_BUILD_DIR}/etc/ssl no-shared threads
	grep '^CFLAG=' Makefile | LC_ALL=C sed -e 's:^CFLAG=::' -e 's:-ffast-math ::g' -e 's:-fomit-frame-pointer ::g' -e 's:-O[0-9] ::g' -e 's:-march=[-a-z0-9]* ::g' -e 's:-mcpu=[-a-z0-9]* ::g' -e 's:-m[a-z0-9]* ::g' > x-compile-tmp
	CFLAG="$(< x-compile-tmp)"
	sed -i -e "/^CFLAG/s:=.*:=${CFLAG} ${CFLAGS}:" -e "/^SHARED_LDFLAGS=/s:$: ${LDFLAGS}:" Makefile
	make -j1 depend
	make -j2 all
	make rehash
	make install
	cd ..
fi

if [[ ! -d "libarchive-3.0.4" ]] ; then
	echo "* Building libarchive-3.0.4 . . ."
	echo ""
	export CFLAGS="${BASE_CFLAGS}"
	export CXXFLAGS="${BASE_CFLAGS}"
	if [[ ! -f "./libarchive-3.0.4.tar.gz" ]] ; then
		wget -O "./libarchive-3.0.4.tar.gz" "https://github.com/downloads/libarchive/libarchive/libarchive-3.0.4.tar.gz"
	fi
	tar -xvzf ./libarchive-3.0.4.tar.gz
	cd libarchive-3.0.4
	./configure --prefix=${TC_BUILD_DIR} --host=${CROSS_TC} --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-expat --without-xml2
	make -j2
	make install
	cd ..
fi

# Build KT packages credits
cat > ../../CREDITS << EOF
* kindletool.exe: KindleTool, Copyright (C) 2011-2012  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |->   zlib, Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler,
  |   Licensed under the zlib license (http://zlib.net/zlib_license.html)
  |   (http://zlib.net/)
  |
  |->   OpenSSL, Copyright (C) 1998-2011 The OpenSSL Project. SSLeay, Copyright (C) 1995-1998 Eric Young,
  |   Licensed under an Apache-style license (http://openssl.org/source/license.html).
  |   (http://openssl.org/)
  |
  |->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
  |   (http://libarchive.github.com/)
  |
  \`->   Built using MinGW-w64 and statically linked against the MinGW-w64 runtime, Copyright (C) 2009, 2010 by the mingw-w64 project,
      Licensed mostly under the Zope Public License (ZPL) Version 2.1. (http://sourceforge.net/apps/trac/mingw-w64/browser/tags/v2.0.4/COPYING.MinGW-w64-runtime/COPYING.MinGW-w64-runtime.txt)
      (http://mingw-w64.sourceforge.net/)
EOF

# KindleTool
echo "* Building KindleTool . . ."
echo ""
cd ../..
cd KindleTool/KindleTool
rm -rf lib includes
make clean
make mingw

# Package it
git log --stat --graph > ../../ChangeLog
./version.sh PMS
VER_FILE="VERSION"
VER_CURRENT="$(<${VER_FILE})"
REV="${VER_CURRENT%%-*}"
cd ../..
cp -v KindleTool/KindleTool/MinGW/kindletool.exe ./kindletool.exe
cp -v KindleTool/README.md ./README
# Quick! Markdown => plaintext
sed -si 's/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
mv -v KindleTool/KindleTool/VERSION ./VERSION
# LF => CRLF...
unix2dos CREDITS README ChangeLog
7z a -tzip kindletool-${REV}-mingw.zip kindletool.exe CREDITS README ChangeLog VERSION
rm -f kindletool.exe CREDITS README ChangeLog VERSION

