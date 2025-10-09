CC?=gcc
STRIP?=strip
DEBUG_CFLAGS:=-Og -march=native -fno-omit-frame-pointer -pipe -g3
CLANG_DEBUG_CFLAGS:=-O0 -march=native -fno-omit-frame-pointer -pipe -g3
OPT_CFLAGS:=-O3 -ffast-math -march=native -fomit-frame-pointer -frename-registers -fweb -pipe
CLANG_OPT_CFLAGS:=-O3 -ffast-math -march=native -fomit-frame-pointer -pipe
K3_CFLAGS:=-O3 -ffast-math -march=armv6j -mtune=arm1136jf-s -fomit-frame-pointer -frename-registers -fweb -pipe -fno-stack-protector -U_FORTIFY_SOURCE
MINGW_CFLAGS:=-O3 -ffast-math -march=x86-64 -mtune=generic -fomit-frame-pointer -pipe
# When we want to use clang's asan (http://clang.llvm.org/docs/AddressSanitizer.html), because it rocks.
ASAN_CLFAGS:=-O1 -march=native -fno-omit-frame-pointer -pipe -fno-optimize-sibling-calls -Weverything -Wno-disabled-macro-expansion -fsanitize=address -g3
# In the same vein, since valgrind 2.8.0 now runs properly on my Gentoo system, here's a few notes about its current report:
# (Using --leak-check=full --track-origins=yes --show-reachable=yes)
# The getpwuid_r/getgrgid_r thing from libarchive's archive_read_disk_set_standard_lookup is mostly harmless (cf. http://sourceware.org/bugzilla/show_bug.cgi?id=2314),
# and only happens when using the NSS compat stuff (which happened to be the default on my system...)

# Kindle cross toolchain prefix
ifdef KINDLE
	CROSS_PREFIX?=arm-kindle-linux-gnueabi-
endif
# MinGW-w64 (64bit) cross toolchain prefix
ifdef MINGW
	CROSS_PREFIX?=x86_64-w64-mingw32-
	NEED_STATIC_PKGC:=STATIC
endif

SRCS:=kindle_tool.c create.c convert.c nettle_pem.c

default: all

# OS, version, pkgconfig & misc stuff handling (heavily inspired from git's Makefiles ;))
# It's basically our fake automated configure script, and it uses bash features, so override the default shell!
SHELL:=/usr/bin/env bash
version-inc:
	@$(SHELL) ./version.sh GIT $(NEED_STATIC_PKGC)
-include version-inc

# Try to use sane defaults for DESTDIR/PREFIX, while still playing nice with PMS
ifndef DESTDIR
	PREFIX:=/.
else
	ifeq "$(OSTYPE)" "Darwin"
		# Play nice with Homebrew (not so much, in fact, but whatever).
		PREFIX?=/usr/local
	else
		# Should play nice with various Linux PMS, specifically non-prefixed Portage/Paludis setups
		PREFIX:=/usr
	endif
endif
DESTDIR?=/usr/local
BINDIR:=$(DESTDIR)/$(PREFIX)/bin
MANDIR:=$(DESTDIR)/$(PREFIX)/share/man/man1

ifeq "$(OSTYPE)" "Darwin"
	# Homebrew default paths... (w/ libarchive keg)
	CPPFLAGS?=-I/usr/local/include -I/usr/local/opt/libarchive/include
else
	CPPFLAGS?=-Iincludes
endif

# We of course need libarchive (try to use sane fallbacks on the off chance we have it somewhere but pkg-config failed to find it...)
ifeq "$(HAS_PC_LIBARCHIVE)" "true"
	LIBS:=$(PC_LIBARCHIVE_LIBS)
else
	LIBS:=-larchive
endif
# And nettle...
ifeq "$(HAS_PC_NETTLE)" "true"
	LIBS+=$(PC_NETTLE_LIBS)
else
	LIBS+=-lhogweed -lgmp -lnettle
endif
# And zlib (for libarchive)
LIBS+=-lz

# If we want to use part of gperftools (http://gperftools.googlecode.com/svn/trunk/doc/heap_checker.html for example)
#ifeq "$(OSTYPE)" "Linux"
#	LIBS+=-ltcmalloc
#endif

# Detect GCC version because reasons...
# (namely, GCC emitting an error instead of a warning on unknown -W options)
MOAR_WARNIGS:=0
# Tests heavily inspired from Linux's build system ;).
CC_IS_CLANG:=$(shell $(CC) -v 2>&1 | grep -q "clang version" && echo 1 || echo 0)
CC_VERSION:=$(shell printf "%02d%02d%02d" `echo __GNUC__ | $(CC) -E -x c - | tail -n 1` `echo __GNUC_MINOR__ | $(CC) -E -x c - | tail -n 1` `echo __GNUC_PATCHLEVEL__ | $(CC) -E -x c - | tail -n 1`)
# Detect Clang's SA, too...
ifeq "$(CC_IS_CLANG)" "0"
	ifeq "$(lastword $(subst /, ,$(CC)))" "ccc-analyzer"
		CC_IS_CLANG:=1
	endif
endif
ifeq "$(CC_IS_CLANG)" "1"
	# This is Clang
	MOAR_WARNIGS:=1
endif
ifeq "$(shell expr $(CC_VERSION) \>= 070000)" "1"
	# This is GCC >= 7
	MOAR_WARNIGS:=1
endif

ifdef DEBUG
	OUT_DIR:=Debug
	# Clang doesn't support -Og...
	ifeq "$(CC_IS_CLANG)" "1"
		CFLAGS:=$(CLANG_DEBUG_CFLAGS)
	else
		CFLAGS:=$(DEBUG_CFLAGS)
	endif
else
	OUT_DIR:=Release
	# Don't default to stuff that'll make clang unhappy
	ifeq "$(CC_IS_CLANG)" "1"
		CFLAGS?=$(CLANG_OPT_CFLAGS)
	else
		CFLAGS?=$(OPT_CFLAGS)
	endif
	# Enforce aggressive optimizations...
	ifeq (,$(findstring -O3,$(CFLAGS)))
		override CFLAGS:=$(CFLAGS:-O%=-O3)
	endif
	ifeq (,$(findstring -ffast-math,$(CFLAGS)))
		KT_CFLAGS+=-ffast-math
	endif
	ifeq (,$(findstring -ftree-vectorize,$(CFLAGS)))
		KT_CFLAGS+=-ftree-vectorize
	endif
	ifeq (,$(findstring -funroll-loops,$(CFLAGS)))
		KT_CFLAGS+=-funroll-loops
	endif
endif

ifdef KINDLE
	OUT_DIR:=Kindle
	CFLAGS?=$(K3_CFLAGS)
	CC:=$(CROSS_PREFIX)gcc
	STRIP:=$(CROSS_PREFIX)strip
endif

ifdef MINGW
	OUT_DIR:=MinGW
	CFLAGS?=$(MINGW_CFLAGS)
	CC:=$(CROSS_PREFIX)gcc
	STRIP:=$(CROSS_PREFIX)strip
endif

# Oh, OS X...
ifeq "$(OSTYPE)" "Darwin"
	STRIP_OPTS:=
else
	STRIP_OPTS:=--strip-unneeded
endif

# It appears that Windows is using a strange & incongruous file extension for its binaries... ;D
ifdef MINGW
	# Cygwin is smart enough to take care of it itself, so just check MinGW ;)
	BINEXT:=.exe
else
	BINEXT:=
endif

# NOTE: All the KT_ prefixed *FLAGS are stuff that we *always* want set, no matter what the user does.
# Moar warnings!
ifeq "$(MOAR_WARNIGS)" "1"
	KT_CFLAGS+=-Wall
	KT_CFLAGS+=-Wextra -Wunused
	KT_CFLAGS+=-Wformat=2
	KT_CFLAGS+=-Wformat-signedness
	# NOTE: This doesn't really play nice w/ FORTIFY, leading to an assload of false-positives, unless LTO is enabled
	ifneq (,$(findstring flto,$(CFLAGS)))
		KT_CFLAGS+=-Wformat-truncation=2
	else
		KT_CFLAGS+=-Wno-format-truncation
	endif
	KT_CFLAGS+=-Wnull-dereference
	KT_CFLAGS+=-Wuninitialized
	KT_CFLAGS+=-Wduplicated-branches -Wduplicated-cond
	KT_CFLAGS+=-Wundef
	KT_CFLAGS+=-Wbad-function-cast
	KT_CFLAGS+=-Wwrite-strings
	KT_CFLAGS+=-Wjump-misses-init
	KT_CFLAGS+=-Wlogical-op
	KT_CFLAGS+=-Wstrict-prototypes -Wold-style-definition
	KT_CFLAGS+=-Wshadow
	KT_CFLAGS+=-Wmissing-prototypes -Wmissing-declarations
	KT_CFLAGS+=-Wnested-externs
	KT_CFLAGS+=-Winline
	KT_CFLAGS+=-Wcast-qual
	# NOTE: GCC 8 introduces -Wcast-align=strict to warn regardless of the target architecture (i.e., like clang)
	KT_CFLAGS+=-Wcast-align
	KT_CFLAGS+=-Wconversion
	# Output padding info when debugging (NOTE: Clang is slightly more verbose)
	# As well as function attribute hints
	ifdef DEBUG
		KT_CFLAGS+=-Wpadded
		KT_CFLAGS+=-Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wmissing-format-attribute
	endif
	# Annoying with our typedefs
	KT_CFLAGS+=-Wno-bad-function-cast
	# And disable some very verbose and/or annoying stuff
	KT_CFLAGS+=-Wno-jump-misses-init
	# And just because that's annoying...
	ifeq "$(CC_IS_CLANG)" "1"
		KT_CFLAGS+=-Wno-ignored-optimization-argument -Wno-unknown-warning-option -Wno-unknown-attributes
	endif
endif

# libarchive is always built with large files support, do the same to avoid issues.
KT_CPPFLAGS+=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
# Get a printf function family with GNU extensions support on MinGW...
ifdef MINGW
	# That's enough for MinGW
	KT_CPPFLAGS+=-D_GNU_SOURCE
	# But not for MinGW-w64...
	KT_CPPFLAGS+=-D__USE_MINGW_ANSI_STDIO=1
endif
KT_CPPFLAGS+=-DKT_VERSION='"$(KT_VERSION)"'
# Add a user@host build tag, unless explicitly forbidden
ifndef KT_NO_USERATHOST_TAG
	KT_CPPFLAGS+=-DKT_USERATHOST='"$(COMPILE_BY)@$(COMPILE_HOST) on $(DISTRIB_ID)"'
endif
# Also pass our stupid hack to get the buildtime nettle version, if it's not empty...
ifneq "$(PC_NETTLE_VERSION)" ""
	KT_CPPFLAGS+=-DNETTLE_VERSION='"$(PC_NETTLE_VERSION)"'
endif

ifndef LDFLAGS
	ifeq "$(OSTYPE)" "Darwin"
		# Again, Homebrew default paths, w/ libarchive keg
		LDFLAGS:=-L/usr/local/lib -L/usr/local/opt/libarchive/lib
	else
		LDFLAGS:=-Llib
		LDFLAGS+=-Wl,-O1 -Wl,--as-needed
	endif
endif

# On platforms where we rely on pkg-config (Linux & OS X), make sure we obey it!
# That should help us link against the correct libarchive version on Debian/Ubuntu, where libarchive-2 and libarchive-3 coexist with a single -dev package;
# And on OS X, where libarchive is keg-only with Homebrew.
ifeq "$(HAS_PC_LIBARCHIVE)" "true"
	CPPFLAGS+=$(PC_LIBARCHIVE_CPPFLAGS)
	LDFLAGS+=$(PC_LIBARCHIVE_LDFLAGS)
endif
# Do the same for nettle
ifeq "$(HAS_PC_NETTLE)" "true"
	CPPFLAGS+=$(PC_NETTLE_CPPFLAGS)
	LDFLAGS+=$(PC_NETTLE_LDFLAGS)
endif

# Let's say we want to default to building stuff that will work on OS X 10.6/10.7... If I get how this stuff works, that should do it...
#ifeq "$(OSTYPE)" "Darwin"
#	# Check that we don't mess with Homebrew, which defaults to the OS version... (I'm not sure Homebrew even sets it for us, since we're not using autotools...)
#	# Although it supposedly defaults to the OS version since OS X 10.5 anyway, so...
#	ifndef MACOSX_DEPLOYMENT_TARGET
#		export MACOSX_DEPLOYMENT_TARGET:=10.6
#		KT_CFLAGS+=-mmacosx-version-min=10.6
#	endif
#endif

OBJS:=$(addprefix $(OUT_DIR)/, $(SRCS:.c=.o))

$(OUT_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(KT_CPPFLAGS) $(CFLAGS) $(KT_CFLAGS) -o $@ -c $<

outdir:
	mkdir -p $(OUT_DIR)

# Make absolutely sure we create our output directories first, even with unfortunate // timings!
# c.f., https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html#Prerequisite-Types
$(OBJS): | outdir

all: kindletool

kindletool: version-inc $(OBJS)
	$(CC) $(CPPFLAGS) $(KT_CPPFLAGS) $(CFLAGS) $(KT_CFLAGS) $(LDFLAGS) -o$(OUT_DIR)/$@$(BINEXT) $(OBJS) $(LIBS)

strip: all
	$(STRIP) $(STRIP_OPTS) $(OUT_DIR)/kindletool$(BINEXT)

debug:
	$(MAKE) all DEBUG=true

kindle:
	$(MAKE) strip KINDLE=true

mingw:
	$(MAKE) strip MINGW=true

clean:
	rm -rf Release/*.o
	rm -rf Release/kindletool
	rm -rf Debug/*.o
	rm -rf Debug/kindletool
	rm -rf Kindle/*.o
	rm -rf Kindle/kindletool
	rm -rf MinGW/*.o
	rm -rf MinGW/kindletool.exe
	rm -rf version-inc
	rm -rf VERSION

install: all
	install -d -m 755 $(BINDIR)
	install '$(OUT_DIR)/kindletool' $(BINDIR)
	install -d -m 755 $(MANDIR)
	install -m 644 kindletool.1 $(MANDIR)


.PHONY: all install clean default outdir kindletool strip debug kindle mingw
