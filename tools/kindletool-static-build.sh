#!/bin/bash

OSTYPE="$(uname -s)"
ARCH="$(uname -m)"

## Linux!
Build_Linux() {
	echo "* Preparing a static KindleTool build on Linux . . ."
	if [[ "${ARCH}" == "x86_64" ]] ; then
		export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
		export CXXFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
	else
		export CFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
		export CXXFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
	fi
	export LDFLAGS="-Wl,-O1 -Wl,--as-needed"

	OPENSSL_DIR="openssl-0.9.8x"
	LIBARCHIVE_DIR="libarchive-3.0.4"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

	# OpenSSL: pretty much the same way as in my arm builds
	if [[ ! -d "${OPENSSL_DIR}" ]] ; then
		echo "* Building ${OPENSSL_DIR} . . ."
		echo ""
		export LDFLAGS="-Wa,--noexecstack"
		if [[ ! -f "./${OPENSSL_DIR}.tar.gz" ]] ; then
			wget -O "./${OPENSSL_DIR}.tar.gz" "http://www.openssl.org/source/${OPENSSL_DIR}.tar.gz"
		fi
		tar -xvzf ./${OPENSSL_DIR}.tar.gz
		cd ${OPENSSL_DIR}
		patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-0.9.8e-bsd-sparc64.patch
		patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-0.9.8h-ldflags.patch
		patch -p1 < /usr/portage/dev-libs/openssl/files/openssl-0.9.8m-binutils.patch
		sed -i -e '/DIRS/s: fips : :g' -e '/^MANSUFFIX/s:=.*:=ssl:' -e '/^MAKEDEPPROG/s:=.*:=gcc:' -e '/^install:/s:install_docs::' Makefile{,.org}
		sed -i '/^SET_X/s:=.*:=set -x:' Makefile.shared
		cp /usr/portage/dev-libs/openssl/files/gentoo.config-0.9.8 gentoo.config
		chmod a+rx gentoo.config
		sed -i '1s,^:$,#!/usr/bin/perl,' Configure
		sed -i '/^"debug-steve/d' Configure
		if [[ "${ARCH}" == "x86_64" ]] ; then
			./Configure linux-generic64 -DL_ENDIAN -O2 -march=core2 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE -fno-strict-aliasing enable-camellia enable-mdc2 enable-tlsext enable-zlib shared threads
		else
			./Configure linux-generic32 -DL_ENDIAN -O2 -march=i686 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE -fno-strict-aliasing enable-camellia enable-mdc2 enable-tlsext enable-zlib shared threads
		fi
		grep '^CFLAG=' Makefile | LC_ALL=C sed -e 's:^CFLAG=::' -e 's:-ffast-math ::g' -e 's:-fomit-frame-pointer ::g' -e 's:-O[0-9] ::g' -e 's:-march=[-a-z0-9]* ::g' -e 's:-mcpu=[-a-z0-9]* ::g' -e 's:-m[a-z0-9]* ::g' >| x-compile-tmp
		CFLAG="$(< x-compile-tmp)"
		sed -i -e "/^CFLAG/s:=.*:=${CFLAG} ${CFLAGS}:" -e "/^SHARED_LDFLAGS=/s:$: ${LDFLAGS}:" Makefile
		make -j1 depend
		make -j1 build_libs
		cd ..
	fi

	# libarchive
	if [[ ! -d "${LIBARCHIVE_DIR}" ]] ; then
		echo "* Building ${LIBARCHIVE_DIR} . . ."
		echo ""
		export ac_cv_header_ext2fs_ext2_fs_h=0
		export LDFLAGS="-Wl,-O1 -Wl,--as-needed"
		if [[ ! -f "./${LIBARCHIVE_DIR}.tar.gz" ]] ; then
			wget -O "./${LIBARCHIVE_DIR}.tar.gz" "https://github.com/downloads/libarchive/libarchive/${LIBARCHIVE_DIR}.tar.gz"
		fi
		tar -xvzf ./${LIBARCHIVE_DIR}.tar.gz
		cd ${LIBARCHIVE_DIR}
		./configure --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-expat --without-xml2
		make
		unset ac_cv_header_ext2fs_ext2_fs_h
		cd ..
	fi

	# Build KT packages credits
	cat > CREDITS << EOF
* kindletool:

KindleTool, Copyright (C) 2011-2012  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
(http://libarchive.github.com/)
EOF

	# KindleTool (OpenSSL-0.9.8)
	echo "* Building KindleTool (OpenSSL-0.9.8) . . ."
	echo ""
	# Fake user@host tag
	if [[ "$(whoami)" == "niluje" ]] ; then
		export KT_NO_USERATHOST_TAG="true"
		if [[ "${ARCH}" == "x86_64" ]] ; then
			export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE -DKT_USERATHOST='\"niluje@ajulutsikael\"'"
		else
			export CFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE -DKT_USERATHOST='\"niluje@ajulutsikael\"'"
		fi
	fi
	export LDFLAGS="-Llib -Wl,-O1 -Wl,--as-needed"
	cd KindleTool/KindleTool
	rm -rf lib includes
	mkdir -p lib includes
	cp -v ../../${LIBARCHIVE_DIR}/.libs/libarchive.a lib
	#cp -v ../../${OPENSSL_DIR}/libcrypto.a lib
	cp -v ../../${OPENSSL_DIR}/libcrypto.so.0.9.8 lib
	cd lib
	ln -sfv libcrypto.so.0.9.8 libcrypto.so
	cd ..
	cp -vrL ../../${OPENSSL_DIR}/include/openssl includes
	if [[ "${ARCH}" == "x86_64" ]] ; then
		cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive.h includes
		cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive_entry.h includes
	fi
	make clean
	make strip
	rm -rf lib includes

	# Package it
	git log --stat --graph > ../../ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	REV="${VER_CURRENT%%-*}"
	cd ../..
	cp -v KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
	cp -v KindleTool/KindleTool/kindletool.1 ./kindletool.1
	mv -v KindleTool/KindleTool/VERSION ./VERSION
	tar -cvzf kindletool-${REV}-linux-${ARCH}-openssl-0.9.8.tar.gz kindletool CREDITS README kindletool.1 ChangeLog VERSION
	rm -f kindletool README kindletool.1 ChangeLog VERSION

	# KindleTool (OpenSSL-1)
	echo "* Building KindleTool (OpenSSL-1) . . ."
	echo ""
	cd KindleTool/KindleTool
	rm -rf lib includes
	mkdir -p lib includes
	cp -v ../../${LIBARCHIVE_DIR}/.libs/libarchive.a lib
	if [[ "${ARCH}" == "x86_64" ]] ; then
		cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive.h includes
		cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive_entry.h includes
	fi
	make clean
	make strip
	rm -rf lib includes

	# Package it
	git log --stat --graph > ../../ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	REV="${VER_CURRENT%%-*}"
	cd ../..
	cp -v KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
	cp -v KindleTool/KindleTool/kindletool.1 ./kindletool.1
	mv -v KindleTool/KindleTool/VERSION ./VERSION
	tar -cvzf kindletool-${REV}-linux-${ARCH}.tar.gz kindletool CREDITS README kindletool.1 ChangeLog VERSION
	rm -f kindletool CREDITS README kindletool.1 ChangeLog VERSION
}

# Win32 !
Build_Cygwin() {
	echo "* Preparing a static KindleTool build on Cygwin . . ."
	export CFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer"
	export CXXFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer"
	export LDFLAGS="-Wl,-O1 -Wl,--as-needed"

	LIBARCHIVE_DIR="libarchive-3.0.4"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

	# libarchive
	if [[ ! -d "${LIBARCHIVE_DIR}" ]] ; then
		echo "* Building ${LIBARCHIVE_DIR} . . ."
		echo ""
		if [[ ! -f "./${LIBARCHIVE_DIR}.tar.gz" ]] ; then
			wget -O "./${LIBARCHIVE_DIR}.tar.gz" "https://github.com/downloads/libarchive/libarchive/${LIBARCHIVE_DIR}.tar.gz"
		fi
		tar -xvzf ./${LIBARCHIVE_DIR}.tar.gz
		cd ${LIBARCHIVE_DIR}
		./configure --prefix=/usr --enable-static --enable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-expat --without-xml2
		make
		make install
		cd ..
	fi

	# Build KT packages credits
	cat > CREDITS << EOF
* kindletool.exe:

KindleTool, Copyright (C) 2011-2012  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
(http://libarchive.github.com/)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	# Fake user@host tag
	if [[ "$(whoami)" == "NiLuJe" ]] ; then
		export KT_NO_USERATHOST_TAG="true"
		export CFLAGS="-march=i686 -pipe -O2 -fomit-frame-pointer -DKT_USERATHOST='\"NiLuJe@Ajulutsikael\"'"
	fi
	cd KindleTool/KindleTool
	# Disable dynamic libraries...
	mv -v /usr/lib/libarchive.la{,.disabled}
	mv -v /usr/lib/libarchive.dll.a{,.disabled}
	mv -v /usr/bin/cygarchive-12.dll{,.disabled}
	make clean
	make strip
	## Restore dynamic libraries...
	mv -v /usr/lib/libarchive.la{.disabled,}
	mv -v /usr/lib/libarchive.dll.a{.disabled,}
	mv -v /usr/bin/cygarchive-12.dll{.disabled,}

	# Package it
	git log --stat --graph > ../../ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	REV="${VER_CURRENT%%-*}"
	cd ../..
	cp -v KindleTool/KindleTool/Release/kindletool.exe ./kindletool.exe
	cp -v KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
	mv -v KindleTool/KindleTool/VERSION ./VERSION
	# LF => CRLF...
	unix2dos CREDITS README ChangeLog
	7z a -tzip kindletool-${REV}-cygwin.zip kindletool.exe CREDITS README ChangeLog VERSION
	rm -f kindletool.exe CREDITS README ChangeLog VERSION
}

# OS X !
Build_OSX() {
	echo "* Preparing a static KindleTool build on OS X . . ."
	# Make sure it'll run on OS X 10.6, too
	export MACOSX_DEPLOYMENT_TARGET=10.6
	export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -mmacosx-version-min=10.6"
	export CXXFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -mmacosx-version-min=10.6"
	export LDFLAGS=""

	LIBARCHIVE_DIR="libarchive-3.0.4"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

	# libarchive
	if [[ ! -d "${LIBARCHIVE_DIR}" ]] ; then
		echo "* Building ${LIBARCHIVE_DIR} . . ."
		echo ""
		if [[ ! -f "./${LIBARCHIVE_DIR}.tar.gz" ]] ; then
			curl -L "https://github.com/downloads/libarchive/libarchive/${LIBARCHIVE_DIR}.tar.gz" -o "./${LIBARCHIVE_DIR}.tar.gz"
		fi
		tar -xvzf ./${LIBARCHIVE_DIR}.tar.gz
		cd ${LIBARCHIVE_DIR}
		./configure --prefix=/opt/local --enable-static --enable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-expat --without-xml2
		make
		cd ..
	fi

	# Build KT packages credits
	mkdir -p Release
	cat > Release/CREDITS << EOF
* kindletool:

KindleTool, Copyright (C) 2011-2012  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
(http://libarchive.github.com/)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	# Fake user@host tag
	if echo "$(whoami)" | grep -E -e '^[nNiIlLuUjJeE]{6}' > /dev/null 2>&1 ; then
		export KT_NO_USERATHOST_TAG="true"
		export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -mmacosx-version-min=10.6 -DKT_USERATHOST='\"niluje@ajulutsikael\"'"
	fi
	export CPPFLAGS="-Iincludes"
	export LDFLAGS="-Llib"
	cd KindleTool/KindleTool
	rm -rf lib includes
	mkdir -p lib includes
	cp -v ../../${LIBARCHIVE_DIR}/.libs/libarchive.a lib
	cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive.h includes
	cp -v ../../${LIBARCHIVE_DIR}/libarchive/archive_entry.h includes
	make clean
	make strip
	rm -rf lib includes

	# Package it
	git log --stat --graph > ../../Release/ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	REV="${VER_CURRENT%%-*}"
	cd ../..
	cd Release
	cp -v ../KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v ../KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	perl -pi -e 's/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^\* /  /g;s/\*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^([[:digit:]])\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' ./README
	cp -v ../KindleTool/KindleTool/KindleTool.1 ./KindleTool.1
	mv -v ../KindleTool/KindleTool/VERSION ./VERSION
	rm -f kindletool-${REV}-osx.zip
	# Don't store uid/gid & attr, I'm packaging this on a 3rd party's computer
	zip -X kindletool-${REV}-osx kindletool CREDITS README KindleTool.1 ChangeLog VERSION
	rm -f kindletool CREDITS README KindleTool.1 ChangeLog VERSION
	cd ..

	## NOTE: Do we need to do a MacPorts build? (link against MacPorts libs, instead of OS X's OpenSSL 0.9.8/zlib 1.2.5?
}

# Main
case "${OSTYPE}" in
	"Linux" )
		Build_Linux
	;;
	CYGWIN* )
		## NOTE: Output from uname -s is uppercase and appends info about the host's Windows version (ie. CYGWIN_NT-6.1), while uname -o will simply report Cygwin
		Build_Cygwin
	;;
	"Darwin" )
		Build_OSX
	;;
	* )
		echo "Unknown OS: ${OSTYPE}"
		exit 1
	;;
esac
