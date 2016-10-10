#!/bin/bash -e

OSTYPE="$(uname -s)"
ARCH="$(uname -m)"
KERNREL="$(uname -r)"

## Setup parallellization... Shamelessly stolen from crosstool-ng ;).
AUTO_JOBS=$(($(getconf _NPROCESSORS_ONLN 2> /dev/null || echo 0) + 1))
JOBSFLAGS="-j${AUTO_JOBS}"

## Linux!
Build_Linux() {
	echo "* Preparing a static KindleTool build on Linux . . ."
	if [[ "${ARCH}" == "x86_64" ]] ; then
		export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -frename-registers -fweb -fno-stack-protector -U_FORTIFY_SOURCE"
		export CXXFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -frename-registers -fweb -fno-stack-protector -U_FORTIFY_SOURCE"
		export GMPABI="64"
		# Mangle i686 builds on my desktop...
		if [[ "${KERNREL}" == *-niluje* ]] && [[ "${KERNREL}" != *-hardened* ]] ; then
			export CFLAGS="-march=i686 -mtune=generic -m32 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
			export CXXFLAGS="-march=i686 -mtune=generic -m32 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
			export GMPABI="32"
			ARCH="i686"
		fi
	else
		export CFLAGS="-march=i686 -mtune=generic -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
		export CXXFLAGS="-march=i686 -mtune=generic -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE"
		export GMPABI="32"
	fi

	GMP_VER="6.1.1"
	GMP_DIR="gmp-${GMP_VER%a}"
	NETTLE_VER="3.2"
	NETTLE_DIR="nettle-${NETTLE_VER}"
	LIBARCHIVE_VER="3.2.1"
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
		make ${JOBSFLAGS}
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
			sed -e '/SUBDIRS/s/testsuite examples//' -i Makefile.in
			autoreconf -fi
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
			make ${JOBSFLAGS}
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
			sed -e '/SUBDIRS/s/testsuite examples//' -i Makefile.in
			sh ./.bootstrap
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
			make ${JOBSFLAGS}
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
			make ${JOBSFLAGS}
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
			./configure --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-openssl --without-expat --without-xml2 --without-lz4
			make ${JOBSFLAGS}
			make install
			unset ac_cv_header_ext2fs_ext2_fs_h
			cd ..
		fi
	fi

	# Build KT package credits
	cat > CREDITS << EOF
* kindletool:

KindleTool, Copyright (C) 2011-2015  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |
  |->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
  |   (http://libarchive.github.com/)
  |
  |->   GMP, GNU MP Library, Copyright 1991-2014 Free Software Foundation, Inc.,
  |   licensed under the GNU Lesser General Public License version 3+ (http://www.gnu.org/licenses/lgpl.html).
  |   (http://gmplib.org/)
  |
  \`->   nettle, Copyright (C) 2001-2014 Niels Möller,
      licensed under the GNU Lesser General Public License version 2.1+ (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
      (http://www.lysator.liu.se/~nisse/nettle)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	# Fake user@host tag
	if [[ "$(whoami)" == "niluje" ]] ; then
		export KT_NO_USERATHOST_TAG="true"
		if [[ "${ARCH}" == "x86_64" ]] ; then
			export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -frename-registers -fweb -fno-stack-protector -U_FORTIFY_SOURCE -DKT_USERATHOST='\"niluje@ajulutsikael on Gentoo\"'"
		else
			export CFLAGS="-march=i686 -mtune=generic -m32 -pipe -O2 -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE -DKT_USERATHOST='\"niluje@ajulutsikael on Gentoo\"'"
		fi
	fi
	cd KindleTool/KindleTool
	rm -rf lib includes
	make clean
	make strip

	# Package it
	git log --stat --graph > ../../ChangeLog
	./KindleTool/version.sh PMS
	VER_FILE="./KindleTool/VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	# Strip the git commit
	REV="${VER_CURRENT%%-*}"
	#REV="${VER_CURRENT}"
	cd ../..
	cp -v KindleTool/KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v KindleTool/KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/<b>//g;s/<\/b>//g;s/<i>//g;s/<\/i>//g;s/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
	cp -v KindleTool/KindleTool/KindleTool/kindletool.1 ./kindletool.1
	cp -v KindleTool/KindleTool/KindleTool/VERSION ./VERSION
	tar -cvzf kindletool-${REV}-linux-${ARCH}.tar.gz kindletool CREDITS README kindletool.1 ChangeLog VERSION
	rm -f kindletool CREDITS README kindletool.1 ChangeLog VERSION
}

# Win32 !
Build_Cygwin() {
	echo "* Preparing a static KindleTool build on Cygwin . . ."
	# NOTE: Horrible hack. _NSIG isn't defined on Cygwin (it's defined in the linux headers), and CMake doesn't give a damn about CPPFLAGS.
	export CFLAGS="-D_NSIG=64 -march=i686 -mtune=generic -pipe -O2 -fomit-frame-pointer"
	export CXXFLAGS="-march=i686 -mtune=generic -pipe -O2 -fomit-frame-pointer"
	export LDFLAGS="-Wl,-O1 -Wl,--as-needed"

	LIBARCHIVE_VER="3.2.1"
	LIBARCHIVE_DIR="libarchive-${LIBARCHIVE_VER}"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

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
			# NOTE: The win crypto stuff breaks horribly with the current Cygwin packages...
			# Switch to cmake, which will properly use Nettle on Cygwin, and hope it doesn't break everything, because the tests still fail horribly to build...
			cmake -DCMAKE_INSTALL_PREFIX="/usr" -DCMAKE_BUILD_TYPE="Release" -DENABLE_TEST=FALSE -DBUILD_TESTING=FALSE -DENABLE_TAR=ON -DENABLE_XATTR=FALSE -DENABLE_ACL=FALSE -DENABLE_ICONV=FALSE -DENABLE_CPIO=FALSE -DENABLE_NETTLE=ON -DENABLE_OPENSSL=FALSE
			make
			make install
			cd ..
		fi
	else
		if [[ ! -d "libarchive-git" ]] ; then
			echo "* Building libarchive . . ."
			echo ""
			git clone https://github.com/libarchive/libarchive.git libarchive-git
			cd libarchive-git
			# NOTE: CMake isn't up to date in the Cygwin repos, but is new enough for our purposes. Revert part of 1052c76, it doesn't concern us on Cygwin anyway.
			sed -e 's/CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)/CMAKE_MINIMUM_REQUIRED(VERSION 2.8.6 FATAL_ERROR)/' -i CMakeLists.txt
			# NOTE: The win crypto stuff breaks horribly with the current Cygwin packages...
			# Switch to cmake, which will properly use Nettle on Cygwin, and hope it doesn't break everything, because the tests still fail horribly to build...
			cmake -DCMAKE_INSTALL_PREFIX="/usr" -DCMAKE_BUILD_TYPE="Release" -DENABLE_TEST=FALSE -DBUILD_TESTING=FALSE -DENABLE_TAR=ON -DENABLE_XATTR=FALSE -DENABLE_ACL=FALSE -DENABLE_ICONV=FALSE -DENABLE_CPIO=FALSE -DENABLE_NETTLE=ON -DENABLE_OPENSSL=FALSE -DENABLE_LZMA=FALSE -DENABLE_ZLIB=ON -DENABLE_BZip2=FALSE -DENABLE_EXPAT=FALSE
			make
			make install
			cd ..
		fi
	fi

	# Build KT package credits
	cat > CREDITS << EOF
* kindletool.exe:

KindleTool, Copyright (C) 2011-2015  Yifan Lu, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |
  \`->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
      (http://libarchive.github.com/)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	# Fake user@host tag
	if [[ "$(whoami)" == "NiLuJe" ]] ; then
		export KT_NO_USERATHOST_TAG="true"
		export CFLAGS="-march=i686 -mtune=generic -pipe -O2 -fomit-frame-pointer -DKT_USERATHOST='\"NiLuJe@Ajulutsikael on $(uname -s)\"'"
	fi
	cd KindleTool/KindleTool
	# Disable dynamic libraries...
	mv -v /usr/lib/libarchive.dll.a{,.disabled}
	mv -v /usr/bin/cygarchive-14.dll{,.disabled}
	make clean
	make strip
	## Restore dynamic libraries...
	mv -v /usr/lib/libarchive.dll.a{.disabled,}
	mv -v /usr/bin/cygarchive-14.dll{.disabled,}

	# Package it
	git log --stat --graph > ../../ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	# Strip the git commit
	REV="${VER_CURRENT%%-*}"
	#REV="${VER_CURRENT}"
	cd ../..
	cp -v KindleTool/KindleTool/Release/kindletool.exe ./kindletool.exe
	cp -v KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	sed -si 's/<b>//g;s/<\/b>//g;s/<i>//g;s/<\/i>//g;s/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^* /  /g;s/*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^\([[:digit:]]\)\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' README
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
	# NOTE: Don't pull fstatat & openat, they were introduced in 10.10, and I don't want to have to keep an old SDK around to handle this the right way...
	export ac_cv_func_fstatat=no
	export ac_cv_func_openat=no

	GMP_VER="6.1.1"
	GMP_DIR="gmp-${GMP_VER%a}"
	NETTLE_VER="3.2"
	NETTLE_DIR="nettle-${NETTLE_VER}"
	LIBARCHIVE_VER="3.2.1"
	LIBARCHIVE_DIR="libarchive-${LIBARCHIVE_VER}"

	# Make sure we're up to date
	git pull

	# Get out of our git tree
	cd ../..

	KT_SYSROOT="${PWD}/kt-sysroot-osx"
	# NOTE: Use -isystem so that gmp doesn't do crazy stuff...
	export CPPFLAGS="-isystem${KT_SYSROOT}/include"
	export LDFLAGS="-L${KT_SYSROOT}/lib"

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
			curl -L "https://gmplib.org/download/gmp/${GMP_DIR}.tar.xz" -o "./${GMP_DIR}.tar.xz"
		fi
		tar -xvJf ./${GMP_DIR}.tar.xz
		cd ${GMP_DIR}
		# Don't target my host cpu...
		my_host="core2-$(clang --version | grep Target | awk '{print $2}' | cut -d- -f2-)"
		./configure --host=${my_host} --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-cxx
		make ${JOBSFLAGS}
		make install
		cd ..
	fi

	# nettle
	if [[ "${USE_STABLE_NETTLE}" == "true" ]] ; then
		if [[ ! -d "${NETTLE_DIR}" ]] ; then
			echo "* Building ${NETTLE_DIR} . . ."
			echo ""
			if [[ ! -f "./${NETTLE_DIR}.tar.gz" ]] ; then
				curl -L "http://www.lysator.liu.se/~nisse/archive/${NETTLE_DIR}.tar.gz" -o "./${NETTLE_DIR}.tar.gz"
			fi
			tar -xvzf ./${NETTLE_DIR}.tar.gz
			cd ${NETTLE_DIR}
			sed -e '/CFLAGS=/s: -ggdb3::' -e 's/solaris\*)/sunldsolaris*)/' -i '' configure.ac
			sed -e '/SUBDIRS/s/testsuite examples//' -i '' Makefile.in
			autoreconf -fi
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
			make ${JOBSFLAGS}
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
			sed -e '/CFLAGS=/s: -ggdb3::' -e 's/solaris\*)/sunldsolaris*)/' -i '' configure.ac
			sed -e '/SUBDIRS/s/testsuite examples//' -i '' Makefile.in
			sh ./.bootstrap
			./configure --prefix="${KT_SYSROOT}" --libdir="${KT_SYSROOT}/lib" --enable-static --disable-shared --enable-public-key --disable-openssl --disable-documentation
			make ${JOBSFLAGS}
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
				curl -L "http://github.com/libarchive/libarchive/archive/v${LIBARCHIVE_VER}.tar.gz" -o "./${LIBARCHIVE_DIR}.tar.gz"
			fi
			tar -xvzf ./${LIBARCHIVE_DIR}.tar.gz
			cd ${LIBARCHIVE_DIR}
			./build/autogen.sh
			./configure --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-openssl --without-expat --without-xml2
			make ${JOBSFLAGS}
			make install
			cd ..
		fi
	else
		if [[ ! -d "libarchive-git" ]] ; then
			echo "* Building libarchive . . ."
			echo ""
			git clone https://github.com/libarchive/libarchive.git libarchive-git
			cd libarchive-git
			# Kill -Werror, git master doesn't always build with it...
			sed -e 's/-Werror //' -i '' ./Makefile.am
			./build/autogen.sh
			./configure --prefix="${KT_SYSROOT}" --enable-static --disable-shared --disable-xattr --disable-acl --with-zlib --without-bz2lib --without-lzmadec --without-iconv --without-lzma --without-nettle --without-openssl --without-expat --without-xml2 --without-lz4
			make ${JOBSFLAGS}
			make install
			cd ..
		fi
	fi

	# Prepare our Release directory to avoid some case sensitivity sillyness...
	mkdir -p Release

	# Build KT package credits
	cat > Release/CREDITS << EOF
* kindletool:

KindleTool, Copyright (C) 2011-2012  Yifan Lu & Copyright (C) 2012-2016  NiLuJe, licensed under the GNU General Public License version 3+ (http://www.gnu.org/licenses/gpl.html).
(https://github.com/NiLuJe/KindleTool/)

  |
  |->   libarchive, Copyright (C) Tim Kientzle, licensed under the New BSD License (http://www.opensource.org/licenses/bsd-license.php)
  |   (http://libarchive.github.com/)
  |
  |->   GMP, GNU MP Library, Copyright 1991-2014 Free Software Foundation, Inc.,
  |   licensed under the GNU Lesser General Public License version 3+ (http://www.gnu.org/licenses/lgpl.html).
  |   (http://gmplib.org/)
  |
  \`->   nettle, Copyright (C) 2001-2014 Niels Möller,
      licensed under the GNU Lesser General Public License version 2.1+ (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
      (http://www.lysator.liu.se/~nisse/nettle)
EOF

	# KindleTool
	echo "* Building KindleTool . . ."
	echo ""
	# Fake user@host tag
	if echo "$(whoami)" | grep -E -e '^[nNiIlLuUjJeE]{6}' > /dev/null 2>&1 ; then
		export KT_NO_USERATHOST_TAG="true"
		export CFLAGS="-march=core2 -pipe -O2 -fomit-frame-pointer -mmacosx-version-min=10.6 -DKT_USERATHOST='\"niluje@ajulutsikael on Mac OS X $(sw_vers -productVersion)\"'"
	fi
	cd KindleTool/KindleTool
	rm -rf lib includes
	make clean
	make strip

	# Package it
	git log --stat --graph > ../../Release/ChangeLog
	./version.sh PMS
	VER_FILE="VERSION"
	VER_CURRENT="$(<${VER_FILE})"
	# Strip the git commit
	REV="${VER_CURRENT%%-*}"
	#REV="${VER_CURRENT}"
	cd ../..
	cd Release
	cp -v ../KindleTool/KindleTool/Release/kindletool ./kindletool
	cp -v ../KindleTool/README.md ./README
	# Quick! Markdown => plaintext
	perl -pi -e 's/<b>//g;s/<\/b>//g;s/<i>//g;s/<\/i>//g;s/&lt;/</g;s/&gt;/>/g;s/&amp;/&/g;s/^\* /  /g;s/\*//g;s/>> /\t/g;s/^> /  /g;s/^## //g;s/### //g;s/\t/    /g;s/^([[:digit:]])\./  \1)/g;s/^#.*$//;s/[[:blank:]]*$//g' ./README
	cp -v ../KindleTool/KindleTool/kindletool.1 ./kindletool.1
	mv -v ../KindleTool/KindleTool/VERSION ./VERSION
	rm -f kindletool-${REV}-osx.zip
	# Don't store uid/gid & attr, I'm packaging this on a 3rd party's computer
	zip -X kindletool-${REV}-osx.zip kindletool CREDITS README kindletool.1 ChangeLog VERSION
	rm -f kindletool CREDITS README kindletool.1 ChangeLog VERSION
	cd ..
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
