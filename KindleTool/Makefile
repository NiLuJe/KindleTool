CC?=gcc
STRIP?=strip
DEBUG_CFLAGS=-O0 -march=native -fno-omit-frame-pointer -g3
OPT_CFLAGS=-O2 -fomit-frame-pointer -march=native
K3_CFLAGS=-O2 -ffast-math -march=armv6j -mtune=arm1136jf-s -fomit-frame-pointer -fno-stack-protector -U_FORTIFY_SOURCE
MINGW_CFLAGS=-O2 -ffast-math -march=i686 -mtune=i686 -fomit-frame-pointer
# When we want to use clang's asan (http://clang.llvm.org/docs/AddressSanitizer.html), because it rocks.
ASAN_CLFAGS="-O2 -march=native -faddress-sanitizer -fno-omit-frame-pointer -g3"

# Kindlecross toolchain prefix
ifeq "$(KINDLE)" "true"
	CROSS_PREFIX?=arm-kindle-linux-gnueabi-
endif
# MinGW cross toolchain prefix
ifeq "$(MINGW)" "true"
	CROSS_PREFIX?=i686-w64-mingw32-
endif

SRCS=kindle_tool.c create.c convert.c

default: all

# OS, version, pkgconfig & misc stuff handling (Heavily inspired from git's Makefiles ;))
# It's basically our fake automated configure script, and it uses bash features, so override the default shell!
SHELL:=/bin/bash
version-inc:
	@$(SHELL) ./version.sh
-include version-inc

# Try to use sane defaults for DESTDIR/PREFIX, while still playing nice with PMS
ifndef DESTDIR
	PREFIX:=/.
else
	ifeq "$(OSTYPE)" "Darwin"
		# Play nice with MacPorts
		PREFIX:=/opt/local
	else
		# Play nice with Linux PMS
		PREFIX:=/usr
	endif
endif
ifeq "$(OSTYPE)" "Darwin"
	DESTDIR?=/opt/local
else
	DESTDIR?=/usr/local
endif
BINDIR:=$(DESTDIR)/$(PREFIX)/bin
MANDIR:=$(DESTDIR)/$(PREFIX)/share/man/man1

ifeq "$(OSTYPE)" "Darwin"
	CPPFLAGS?=-I/opt/local/include
else
	CPPFLAGS?=-Iincludes
endif
LIBS=-larchive -lcrypto -lz

# Needed for linking statically against OpenSSL on Linux, and as-needed takes care of it when building dynamically, so it doesn't hurt ;).
ifeq "$(OSTYPE)" "Linux"
	# There's no libdl on Windows ;)
	ifneq "$(MINGW)" "true"
		LIBS+=-ldl
	endif
endif
# Needed for linking statically against OpenSSL on MinGW
ifeq "$(MINGW)" "true"
	LIBS+=-lgdi32
endif
# If we want to use part of gperftools (http://gperftools.googlecode.com/svn/trunk/doc/heap_checker.html for example)
#ifeq "$(OSTYPE)" "Linux"
#	LIBS+=-ltcmalloc
#endif

ifeq "$(DEBUG)" "true"
	OUT_DIR=Debug
	CFLAGS:=$(DEBUG_CFLAGS)
else
	OUT_DIR=Release
	CFLAGS?=$(OPT_CFLAGS)
endif

ifeq "$(KINDLE)" "true"
	OUT_DIR=Kindle
	CFLAGS?=$(K3_CFLAGS)
	LDFLAGS?=-Wl,-O1 -Wl,--as-needed
	CC:=$(CROSS_PREFIX)gcc
	STRIP:=$(CROSS_PREFIX)strip
endif

ifeq "$(MINGW)" "true"
	OUT_DIR=MinGW
	CFLAGS?=$(MINGW_CFLAGS)
	LDFLAGS?=-Wl,-O1 -Wl,--as-needed
	CC:=$(CROSS_PREFIX)gcc
	STRIP:=$(CROSS_PREFIX)strip
endif

# Oh, OS X...
ifeq "$(OSTYPE)" "Darwin"
	STRIP_OPTS:=
else
	STRIP_OPTS:=-s
endif

# Use a different varname in order to support appending stuff here even when setting CFLAGS as an argument to make...
KT_CFLAGS:=$(CFLAGS)
KT_CFLAGS+=-pipe
# Moar warnings!
KT_CFLAGS+=-Wall -Wformat -Wformat-security
KT_CFLAGS+=-Wextra -Wunused
KT_CFLAGS+=-Wshadow
KT_CFLAGS+=-Wmissing-prototypes
KT_CFLAGS+=-Wcast-qual
# libarchive is always built with large files support, do the same to avoid issues.
KT_CFLAGS+=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
# Get a printf function family with GNU extensions support on MinGW...
ifeq "$(MINGW)" "true"
	# That's enough for MinGW
	KT_CFLAGS+=-D_GNU_SOURCE
	# But not for MinGW-w64...
	KT_CFLAGS+=-D__USE_MINGW_ANSI_STDIO=1
endif
KT_CFLAGS+=-DKT_VERSION='"$(KT_VERSION)"'
# Add a user@host build tag, unless explicitly forbidden
ifndef KT_NO_USERATHOST_TAG
	KT_CFLAGS+=-DKT_USERATHOST='"$(COMPILE_BY)@$(COMPILE_HOST)"'
endif

ifndef LDFLAGS
	ifeq "$(OSTYPE)" "Darwin"
		LDFLAGS:=-L/opt/local/lib
	else
		LDFLAGS:=-Llib
		LDFLAGS+=-Wl,-O1 -Wl,--as-needed
	endif
endif

# If we're on Linux, not on Gentoo, and pkgconfig detected libarchive >= 3.0.3, use its CPP/LDFLAGS for include & lib searchpath
# That should help us link against the correct libarchive version on Debian/Ubuntu, where libarchive-2 and libarchive-3 coexist with a single -dev package
ifeq "$(OSTYPE)" "Linux"
	ifneq "$(DISTRIB_ID)" "Gentoo"
		ifeq "$(HAS_PC_LIBARCHIVE)" "true"
			CPPFLAGS+=$(PC_LIBARCHIVE_CPPFLAGS)
			LDFLAGS+=$(PC_LIBARCHIVE_LDFLAGS)
		endif
	endif
endif

# Let's say we want to default to building stuff that will work on OS X 10.6/10.7... If I get how this stuff works, that should do it...
#ifeq "$(OSTYPE)" "Darwin"
#	# Check that we don't mess with MacPorts, which defaults to the OS version... (I'm not sure MacPorts even sets it for us, since we're not using autotools...)
#	# Although it supposedly defaults to the OS version since OS X 10.5 anyway, so...
#	ifndef MACOSX_DEPLOYMENT_TARGET
#		export MACOSX_DEPLOYMENT_TARGET=10.6
#		KT_CFLAGS+=-mmacosx-version-min=10.6
#	endif
#endif

OBJS:=$(SRCS:%.c=$(OUT_DIR)/%.o)

$(OUT_DIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(KT_CFLAGS) -o $@ -c $<

outdir:
	mkdir -p $(OUT_DIR)

ifeq "$(OSTYPE)" "Linux"
kindletool.1:
	cp -af KindleTool.1 kindletool.1
else
kindletool.1:
endif

all: outdir kindletool kindletool.1

ifeq "$(MINGW)" "true"
kindletool: version-inc $(OBJS)
	$(CC) $(CPPFLAGS) $(KT_CFLAGS) $(LDFLAGS) -o$(OUT_DIR)/$@.exe $(OBJS) $(LIBS)
else
kindletool: version-inc $(OBJS)
	$(CC) $(CPPFLAGS) $(KT_CFLAGS) $(LDFLAGS) -o$(OUT_DIR)/$@ $(OBJS) $(LIBS)
endif

ifeq "$(MINGW)" "true"
strip: all
	$(STRIP) $(STRIP_OPTS) $(OUT_DIR)/kindletool.exe
else
strip: all
	$(STRIP) $(STRIP_OPTS) $(OUT_DIR)/kindletool
endif

debug:
	$(MAKE) all DEBUG=true

kindle:
	$(MAKE) strip KINDLE=true

mingw:
	$(MAKE) strip MINGW=true

ifeq "$(OSTYPE)" "Linux"
clean_man:
	rm -rf kindletool.1
else
clean_man:
endif

clean: clean_man
	rm -rf Release/*.o
	rm -rf Release/kindletool
	rm -rf Debug/*.o
	rm -rf Debug/kindletool
	rm -rf Kindle/*.o
	rm -rf Kindle/kindletool
	rm -rf MinGW/*.o
	rm -rf MinGW/kindletool.exe
	rm -rf version-inc

ifeq "$(OSTYPE)" "Linux"
install_man:
	install -d -m 755 $(MANDIR)
	install -m 644 kindletool.1 $(MANDIR)
else
install_man:
	install -d -m 755 $(MANDIR)
	install -m 644 KindleTool.1 $(MANDIR)
endif

install: all install_man
	install -d -m 755 $(BINDIR)
	install '$(OUT_DIR)/kindletool' $(BINDIR)
	install -d -m 755 $(MANDIR)
	install -m 644 kindletool.1 $(MANDIR)


.PHONY: all install install_man clean clean_man default outdir kindletool strip debug kindle mingw
