#!/usr/bin/env bash
set -e

#
# Simple static build.
# (Only libarchive & nettle will be built/statically linked).
#
##

OSTYPE="$(uname -s)"
ARCH="$(uname -m)"

## Linux!
Build_Linux() {
	echo "* Preparing a static KindleTool build on Linux . . ."
	unset CPPFLAGS	# Let the Makefile take care of it ;).
	export CFLAGS="-march=native -pipe -O2 -fomit-frame-pointer"
	export CXXFLAGS="-march=native -pipe -O2 -fomit-frame-pointer"
	if [[ "${ARCH}" == "x86_64" ]] ; then
		export CFLAGS="${CFLAGS} -frename-registers -fweb"
		export CXXFLAGS="${CXXFLAGS} -frename-registers -fweb"
		GMPABI="64"
	else
		GMPABI="32"
	fi

	GMP_VER="6.2.1"
	GMP_DIR="gmp-${GMP_VER%a}"
	NETTLE_VER="3.6"
	NETTLE_DIR="nettle-${NETTLE_VER}"
	LIBARCHIVE_VER="3.5.0"
	LIBARCHIVE_DIR="libarchive-${LIBARCHIVE_VER}"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

	KT_SYSROOT="${PWD}/kt-sysroot-lin-${ARCH}"
	# NOTE: Use -isystem so that gmp doesn't do crazy stuff...
	export CPPFLAGS="-isystem${KT_SYSROOT}/include"
	export LDFLAGS="-L${KT_SYSROOT}/lib -Wl,-O1 -Wl,--as-needed"

	BASE_PKG_CONFIG_PATH="${KT_SYSROOT}/lib/pkgconfig"
	BASE_PKG_CONFIG_LIBDIR="${KT_SYSROOT}/lib/pkgconfig"
	export PKG_CONFIG_DIR=
	export PKG_CONFIG_PATH="${BASE_PKG_CONFIG_PATH}"
	export PKG_CONFIG_LIBDIR="${BASE_PKG_CONFIG_LIBDIR}"

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
		./configure ABI=${GMPABI} --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-cxx
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
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
			make -j2
			make install
			cd ..
		fi
	else
		# Build from git to benefit from the more x86_64 friendly API changes
		if [[ ! -d "nettle-git" ]] ; then
			echo "* Building nettle . . ."
			echo ""
			git clone https://git.lysator.liu.se/nettle/nettle.git nettle-git
			cd nettle-git
			sed -e '/CFLAGS=/s: -ggdb3::' -e 's/solaris\*)/sunldsolaris*)/' -i configure.ac
			sed -i '/SUBDIRS/s/testsuite examples//' Makefile.in
			sh ./.bootstrap
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
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
			export ac_cv_header_ext2fs_ext2_fs_h=0
			./build/autogen.sh
			./configure --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-openssl --without-expat --without-xml2
			make -j2
			make install
			unset ac_cv_header_ext2fs_ext2_fs_h
			cd ..
		fi
	else
		if [[ ! -d "libarchive-git" ]] ; then
			echo "* Building libarchive . . ."
			echo ""
			git clone https://github.com/libarchive/libarchive.git libarchive-git
			cd libarchive-git
			# Kill -Werror, git master doesn't always build with it...
			sed -e 's/-Werror //' -i ./Makefile.am
			export ac_cv_header_ext2fs_ext2_fs_h=0
			./build/autogen.sh
			./configure --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-openssl --without-expat --without-xml2 -without-lz4
			make -j2
			make install
			unset ac_cv_header_ext2fs_ext2_fs_h
			cd ..
		fi
	fi

	# Build KT package credits
	cat > CREDITS << EOF
* kindletool:

KindleTool, Copyright (C) 2011-2012 Yifan Lu & Copyright (C) 2012-2023 NiLuJe, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |
  |->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
  |   (http://libarchive.github.com/)
  |
  |->   GMP, GNU MP Library, Copyright 1991-2018 Free Software Foundation, Inc.,
  |   licensed under the GNU Lesser General Public License version 3+ (http://www.gnu.org/licenses/lgpl.html).
  |   (http://gmplib.org/)
  |
  \`->   nettle, Copyright (C) 2001-2018 Niels Möller,
      licensed under the GNU Lesser General Public License version 2.1+ (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
      (http://www.lysator.liu.se/~nisse/nettle)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	cd KindleTool/KindleTool
	rm -rf lib includes
	make clean
	make strip

	# Package it
	git log --stat --graph > ../../ChangeLog
	./version.sh PMS STATIC
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	# Strip the git commit
	REV="${VER_CURRENT%%-*}"
	#REV="${VER_CURRENT}"
	cd ../..
	cp -v KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/<b>//g;s/<\/b>//g;s/<i>//g;s/<\/i>//g;s/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
	cp -v KindleTool/KindleTool/kindletool.1 ./kindletool.1
	mv -v KindleTool/KindleTool/VERSION ./VERSION
	tar -cvzf "kindletool-${REV}-linux-${ARCH}.tar.gz" kindletool CREDITS README kindletool.1 ChangeLog VERSION
	rm -f kindletool CREDITS README kindletool.1 ChangeLog VERSION
}

# Main
case "${OSTYPE}" in
	"Linux" )
		Build_Linux
	;;
	* )
		echo "Unknown OS: ${OSTYPE}"
		exit 1
	;;
esac
