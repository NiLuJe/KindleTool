#!/usr/bin/env bash

# Make sure we put our stuff in the proper directory (ie. the same one as this script)
KT_DIR="${0%/*}"

# Build a make include with, among other things, our version tag, straight from git (heavily inspired from git's GIT-VERSION-GEN)
VER_FILE="${KT_DIR}/version-inc"
VERSION_FILE="${KT_DIR}/VERSION"

# Fallback version
FALLBACK_VER="v1.6.5-GIT"

# Apparently, bsdmake hates me, so, get uname's output from here
UNAME="$(uname -s)"

# Used to add a Linux like user@host compile-time tag
COMPILE_BY="$(whoami | sed 's/\\/\\\\/')"

case "${UNAME}" in
	CYGWIN* )
		# Cygwin's version of hostname doesn't handle the -s flag...
		COMPILE_HOST="$(hostname)"
	;;
	* )
		# We want the short hostname, OS X defaults to fqdn...
		COMPILE_HOST="$(hostname -s)"
	;;
esac

# Check libarchive's version and get the proper CPP/LDFLAGS via pkg-config, to make sure we pickup the correct libarchive version
# NOTE: On OS X, we don't tweak PKG_CONFIG_PATH="/usr/local/opt/libarchive/lib/pkgconfig" manually,
# because we don't want to override what Homebrew & our static build scripts do. The Makefile should use sane defaults as fallback.
if pkg-config --atleast-version=3.0.3 libarchive ; then
	HAS_PC_LIBARCHIVE="true"
	PC_LIBARCHIVE_CPPFLAGS="$(pkg-config libarchive --cflags-only-I)"
	PC_LIBARCHIVE_LDFLAGS="$(pkg-config libarchive --libs-only-L)"
	# We need to pickup Libs.private for MingW builds...
	if [[ "${2}" == "STATIC" ]] ; then
		PC_LIBARCHIVE_LIBS="$(pkg-config libarchive --libs-only-l --libs-only-other --static)"
	else
		PC_LIBARCHIVE_LIBS="$(pkg-config libarchive --libs-only-l --libs-only-other)"
	fi
else
	HAS_PC_LIBARCHIVE="false"
	PC_LIBARCHIVE_CPPFLAGS=""
	PC_LIBARCHIVE_LDFLAGS=""
	PC_LIBARCHIVE_LIBS=""
	echo "**!** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ **!**"
	echo "**!** @ Couldn't find libarchive >= 3.0.3 via pkg-config, don't be surprised if the build fails! @ **!**"
	echo "**!** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ **!**"
fi

# Check for a recent nettle version (Note that nettle >= 3.4 is preferred, especially on 64bits hosts)...
if pkg-config --atleast-version=2.6 nettle ; then
	HAS_PC_NETTLE="true"
	# Check for hogweed, since we need it, and static, to properly pull in gmp
	PC_NETTLE_CPPFLAGS="$(pkg-config hogweed --cflags-only-I --static)"
	PC_NETTLE_LDFLAGS="$(pkg-config hogweed --libs-only-L --static)"
	PC_NETTLE_LIBS="$(pkg-config hogweed --libs-only-l --libs-only-other --static)"
	# Export the nettle version since there is no built-in way to get it at buildtime...
	PC_NETTLE_VERSION="$(pkg-config --modversion nettle)"
else
	HAS_PC_NETTLE="false"
	PC_NETTLE_CPPFLAGS=""
	PC_NETTLE_LDFLAGS=""
	PC_NETTLE_LIBS=""
	PC_NETTLE_VERSION=""
	echo "**!** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ **!**"
	echo "**!** @ Couldn't find nettle >= 2.6 via pkg-config, don't be surprised if the build fails! @ **!**"
	echo "**!** @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ **!**"
fi

if [[ "${UNAME}" == "Linux" ]] ; then
	# Also check the distro name, we'll use pkg-config's cflags in the Makefile on every distro out there except Gentoo, in order
	# to link against the correct libarchive version on distros where libarchive-2 and libarchive-3 can coexist (Debian/Ubuntu, for example).
	# NOTE: I'm fully aware that lsb_release is not installed/properly setup by default on every distro,
	# but the only distro on which the Makefile expects this to be accurate is Gentoo, so that should cover it ;).
	if [[ -f /etc/lsb-release ]] ; then
		. /etc/lsb-release
	else
		if [[ -f /etc/gentoo-release ]] ; then
			# Make sure we detect Gentoo, even if sys-apps/lsb-release isn't installed
			DISTRIB_ID="Gentoo"
		else
			DISTRIB_ID="Linux"
		fi
	fi
elif [[ "${UNAME}" == "Darwin" ]] ; then
	DISTRIB_ID="Mac OS X $(sw_vers -productVersion)"
else
	# Cygwin?
	DISTRIB_ID="${UNAME}"
fi

# If we don't have git installed (why, oh why would you do that? :D), just use the fallback
if ! git help &>/dev/null ; then
	echo "${FALLBACK_VER}" > "${VERSION_FILE}"
fi

# If we have a VERSION file, just use that (that's useful for package managers)
# Otherwise, and if we have a proper git repo, use git (unless we asked for a new VERSION file)!
if [[ -f "${VERSION_FILE}" ]] && [[ "${1}" != "PMS" ]] ; then
	VER="$(< "${VERSION_FILE}")"
elif [[ -z "${VER}" && -d "${GIT_DIR:-${KT_DIR}/../.git}" || -f "${KT_DIR}/../.git" ]] ; then
	# Get a properly formatted version string from our latest tag
	VER="$(git describe --match "v[0-9]*" HEAD 2>/dev/null)"
	# Or from the first commit (provided we manually tagged $(git rev-list --max-parents=0 HEAD) as TAIL, which we did)
	#VER="$(git describe --match TAIL 2>/dev/null)"
	case "$VER" in
		v[0-9]*)
			# Check if our working directory is dirty
			git update-index -q --refresh
			# Not sure I understand the details, but this helps to avoid ending up with a dirty tag when building through Homebrew...
			if [[ -n "${GIT_DIR}" ]] ; then
				[[ -z "$(git diff-index --name-only HEAD -m --relative=KindleTool --)" ]] || VER="${VER}-dirty"
			else
				[[ -z "$(git diff-index --name-only HEAD --)" ]] || VER="${VER}-dirty"
			fi
			# - => .
			#VER=${VER//-/.}
			# - => ., but only the first (rev since tag)
			VER=${VER/-/.}
		;;
		TAIL*)
			git update-index -q --refresh
			# Same thing as before, special case GIT_DIR...
			if [[ -n "${GIT_DIR}" ]] ; then
				[[ -z "$(git diff-index --name-only HEAD -m --relative=KindleTool --)" ]] || VER="${VER}-dirty"
			else
				[[ -z "$(git diff-index --name-only HEAD --)" ]] || VER="${VER}-dirty"
			fi
			# - => .
			#VER=${VER//-/.}
			# TAIL- => r (ala SVN)
			VER="${VER//TAIL-/r}"
			# Technically, we get the number of commits *after* TAIL, so, effectively, TAIL is r0, not r1 like in SVN.
			# Tweak the output some more to fake that :).
			# Strip everything after the first dash
			REV="${VER%%-*}"
			# Strip the first char (r)
			REV="${REV:1}"
			# Fake our rev number
			FREV="$(( REV + 1 ))"
			# NOTE: In our case, another cheap way to get this commit count would be via $(git rev-list HEAD | wc -l)
			# Switch the rev number in our final output
			VER="${VER/r${REV}/r${FREV}}"
		;;
		*)
			VER="${FALLBACK_VER}"
	esac
else
	VER="${FALLBACK_VER}"
fi

# Strip the leading 'v'
#VER=${VER#v*}

# Get current version from include file
if [[ -r "${VER_FILE}" ]] ; then
	VER_CURRENT="$(head -n 1 "${VER_FILE}")"
	# Strip var assignment
	VER_CURRENT="${VER_CURRENT/KT_VERSION = /}"
else
	VER_CURRENT="unset"
fi

# Update our include file, if need be
if [[ "${VER}" != "${VER_CURRENT}" ]] ; then
	echo >&2 "KT_VERSION = ${VER}"
	echo "KT_VERSION = ${VER}" > "${VER_FILE}"
	{
		echo "OSTYPE = ${UNAME}";
		echo "COMPILE_BY = ${COMPILE_BY}";
		echo "COMPILE_HOST = ${COMPILE_HOST}";
		echo "HAS_PC_LIBARCHIVE = ${HAS_PC_LIBARCHIVE}";
		echo "PC_LIBARCHIVE_CPPFLAGS = ${PC_LIBARCHIVE_CPPFLAGS}";
		echo "PC_LIBARCHIVE_LDFLAGS = ${PC_LIBARCHIVE_LDFLAGS}";
		echo "PC_LIBARCHIVE_LIBS = ${PC_LIBARCHIVE_LIBS}";
		echo "HAS_PC_NETTLE = ${HAS_PC_NETTLE}";
		echo "PC_NETTLE_CPPFLAGS = ${PC_NETTLE_CPPFLAGS}";
		echo "PC_NETTLE_LDFLAGS = ${PC_NETTLE_LDFLAGS}";
		echo "PC_NETTLE_LIBS = ${PC_NETTLE_LIBS}";
		echo "PC_NETTLE_VERSION = ${PC_NETTLE_VERSION}";
		echo "DISTRIB_ID = ${DISTRIB_ID}";
	} >> "${VER_FILE}"
fi

# Build a proper VERSION file (PMS)
if [[ "${1}" == "PMS" ]] ; then
	echo "${VER}" > "${VERSION_FILE}"
fi
