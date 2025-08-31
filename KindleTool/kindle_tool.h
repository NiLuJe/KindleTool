/*
**  KindleTool, kindle_tool.h
**
**  Copyright (C) 2011-2012  Yifan Lu
**  Copyright (C) 2012-2023  NiLuJe
**  Concept based on an original Python implementation by Igor Skochinsky & Jean-Yves Avenard,
**    cf., http://www.mobileread.com/forums/showthread.php?t=63225
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KINDLETOOL_H
#define __KINDLETOOL_H

// NOTE: Mainly to shut KDevelop up without any actual impact...
//       We do build MinGW w/ _GNU_SOURCE though.
#if defined(__linux__)
#	ifndef _DEFAULT_SOURCE
#		define _DEFAULT_SOURCE
#	endif
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#if !defined(_WIN32) && !defined(__CYGWIN__)
#	include <pwd.h>
#endif
#include <time.h>
#if defined(__linux__)
#	include <linux/limits.h>
#endif
#include <libgen.h>

// libarchive does not pull that in for us anymore ;).
#if defined(_WIN32) && !defined(__CYGWIN__)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
// For _SH_* constants for kt_win_tmpfile
#	include <share.h>
#endif

#include <archive.h>
#include <archive_entry.h>

#include <gmp.h>
#include <nettle/base16.h>
#include <nettle/base64.h>
#include <nettle/buffer.h>
#include <nettle/md5.h>
#include <nettle/rsa.h>
#include <nettle/sha2.h>

// Die in a slightly more graceful manner than by spewing a whole lot of warnings & errors
// if we're not building against at least libarchive 3.0.3
#if ARCHIVE_VERSION_NUMBER < 3000003
#	error Your libarchive version is too old, KindleTool depends on libarchive >= 3.0.3
#endif

#define BUFFER_SIZE         PIPE_BUF    // 4K
#define BLOCK_SIZE          64
#define RECOVERY_BLOCK_SIZE 131072

#define MAGIC_NUMBER_LENGTH 4
#define MD5_HASH_LENGTH     32
#define SHA256_HASH_LENGTH  64

#define OTA_UPDATE_BLOCK_SIZE           60
#define OTA_UPDATE_V2_BLOCK_SIZE        18
#define OTA_UPDATE_V2_PART_2_BLOCK_SIZE 36
#define RECOVERY_UPDATE_BLOCK_SIZE      131068
#define UPDATE_SIGNATURE_BLOCK_SIZE     60

#define CERTIFICATE_DEV_SIZE 128
#define CERTIFICATE_1K_SIZE  128
#define CERTIFICATE_2K_SIZE  256

#define INDEX_FILE_NAME "update-filelist.dat"

#define SERIAL_NO_LENGTH 16

#define DEFAULT_BYTES_PER_BLOCK (20 * 512)

#define IS_SCRIPT(filename)  (strncasecmp(filename + (strlen(filename) - 4), ".ffs", 4) == 0)       // Flawfinder: ignore
#define IS_SHELL(filename)   (strncasecmp(filename + (strlen(filename) - 3), ".sh", 3) == 0)        // Flawfinder: ignore
#define IS_SIG(filename)     (strncasecmp(filename + (strlen(filename) - 4), ".sig", 4) == 0)       // Flawfinder: ignore
#define IS_BIN(filename)     (strncasecmp(filename + (strlen(filename) - 4), ".bin", 4) == 0)       // Flawfinder: ignore
#define IS_STGZ(filename)    (strncasecmp(filename + (strlen(filename) - 5), ".stgz", 5) == 0)      // Flawfinder: ignore
#define IS_TGZ(filename)     (strncasecmp(filename + (strlen(filename) - 4), ".tgz", 4) == 0)       // Flawfinder: ignore
#define IS_TARBALL(filename) (strncasecmp(filename + (strlen(filename) - 7), ".tar.gz", 7) == 0)    // Flawfinder: ignore
#define IS_DAT(filename)     (strncasecmp(filename + (strlen(filename) - 4), ".dat", 4) == 0)       // Flawfinder: ignore
#define IS_UIMAGE(filename)  (strncmp(filename + (strlen(filename) - 6), "uImage", 6) == 0)         // Flawfinder: ignore

// Don't break tempfiles on Win32... It doesn't like paths starting with // because that means an 'extended' path
// (network shares and more weird stuff like that), but P_tmpdir defaults to / on Win32,
// and we prepend our own constants with / because it's /tmp on POSIX...
// Note that this is only used as a last resort, if for some reason GetTempPath returns something we can't use...
// In any case, don't even try to put tempfiles on the root drive (because unprivileged users can't write there),
// so use "./" (current dir) instead as a crappy workaround.
// NOTE: Geekmaster also experimented with using "../" (parent dir), which may or may not be a better idea...
#if defined(_WIN32) && !defined(__CYGWIN__)
#	define KT_TMPDIR "."

// NOTE: cf. kindle_tool.c
FILE* kt_win_tmpfile(void);

// NOTE: Override the functions the hard way, shutting up GCC in the proces...
#	ifdef tmpfile
#		undef tmpfile
#	endif
#	define tmpfile kt_win_tmpfile
// -> POSIX, assume P_tmpdir (usually /tmp) is a sane fallback.
#else
#	define KT_TMPDIR P_tmpdir
#endif

// HOST_NAME_MAX is undefined on macOS, it instead kindly asks you to query _SC_HOST_NAME_MAX via sysconf()...
#ifndef HOST_NAME_MAX
#	define HOST_NAME_MAX 256
#endif

// Bundlefile status bitmasks
#define BUNDLE_OPEN    1    // 1 << 0       (bit 0)
#define BUNDLE_CREATED 2    // 1 << 1       (bit 1)

// Version tag fallback
#ifndef KT_VERSION
#	define KT_VERSION "v1.6.5-GIT"
#endif

// user@host tag fallback
#ifndef KT_USERATHOST
#	define KT_USERATHOST "someone@somewhere on something"
#endif

// nettle version fallback
#ifndef NETTLE_VERSION
#	define NETTLE_VERSION ">= 2.6"
#endif

// GCC version checks... (We check !clang in addition to GCC, because Clang 'helpfully' defines __GNUC__ ...)
#if !defined(__clang__) && defined(__GNUC__)
#	define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

typedef enum
{
	UpdateSignature,
	OTAUpdateV2,
	OTAUpdate,
	RecoveryUpdate,
	RecoveryUpdateV2,
	UserDataPackage,    // Actually just a gzipped tarball, but easier to implement this way...
	AndroidUpdate,      // Actually a JAR, designed for the weird Kindle X Migu Chinese exclusive
	ComponentUpdate,
	UnknownUpdate = -1
} BundleVersion;

typedef enum
{
	BundleNone = 0,
	BundleMD5,
	BundleSHA256,
	BundleUnknown = -1,
} BundleHashAlgorithm;

typedef enum
{
	CertificateDeveloper = 0x00,
	Certificate1K        = 0x01,
	Certificate2K        = 0x02,
	CertificateUnknown   = 0xFF
} CertificateNumber;

typedef enum
{
	Kindle1                       = 0x01,
	Kindle2US                     = 0x02,
	Kindle2International          = 0x03,
	KindleDXUS                    = 0x04,
	KindleDXInternational         = 0x05,
	KindleDXGraphite              = 0x09,
	Kindle3WiFi                   = 0x08,
	Kindle3WiFi3G                 = 0x06,
	Kindle3WiFi3GEurope           = 0x0A,
	Kindle4NonTouch               = 0x0E,    // Kindle 4 with a silver bezel, released fall 2011
	Kindle5TouchWiFi3G            = 0x0F,
	Kindle5TouchWiFi              = 0x11,
	Kindle5TouchWiFi3GEurope      = 0x10,
	Kindle5TouchUnknown           = 0x12,
	Kindle4NonTouchBlack          = 0x23,    // Kindle 4 with a black bezel, released fall 2012
	KindlePaperWhiteWiFi          = 0x24,    // Kindle PaperWhite (black bezel), released fall 2012 on FW 5.2.0
	KindlePaperWhiteWiFi3G        = 0x1B,
	KindlePaperWhiteWiFi3GCanada  = 0x1C,
	KindlePaperWhiteWiFi3GEurope  = 0x1D,
	KindlePaperWhiteWiFi3GJapan   = 0x1F,
	KindlePaperWhiteWiFi3GBrazil  = 0x20,
	KindlePaperWhite2WiFi         = 0xD4,    // Kindle PaperWhite 2 (black bezel), released fall 2013 on FW 5.4.0
	KindlePaperWhite2WiFiJapan    = 0x5A,
	KindlePaperWhite2WiFi3G       = 0xD5,
	KindlePaperWhite2WiFi3GCanada = 0xD6,
	KindlePaperWhite2WiFi3GEurope = 0xD7,
	KindlePaperWhite2WiFi3GRussia = 0xD8,
	KindlePaperWhite2WiFi3GJapan  = 0xF2,
	KindlePaperWhite2WiFi4GBInternational = 0x17,
	KindlePaperWhite2WiFi3G4GBEurope      = 0x60,
	KindlePaperWhite2Unknown_0xF4         = 0xF4,
	KindlePaperWhite2Unknown_0xF9         = 0xF9,
	KindlePaperWhite2WiFi3G4GB            = 0x62,
	KindlePaperWhite2WiFi3G4GBBrazil      = 0x61,
	KindlePaperWhite2WiFi3G4GBCanada      = 0x5F,
	KindleBasic                           = 0xC6,    // Kindle Basic (Pearl, Touch), released fall 2014 on FW 5.6.0
	KindleVoyageWiFi                      = 0x13,    // Kindle Voyage, released fall 2014 on FW 5.5.0
	ValidKindleUnknown_0x16               = 0x16,
	ValidKindleUnknown_0x21               = 0x21,
	KindleVoyageWiFi3G                    = 0x54,
	KindleVoyageWiFi3GJapan               = 0x2A,
	KindleVoyageWiFi3G_0x4F               = 0x4F,    // CA?
	KindleVoyageWiFi3GMexico              = 0x52,
	KindleVoyageWiFi3GEurope              = 0x53,
	ValidKindleUnknown_0x07               = 0x07,
	ValidKindleUnknown_0x0B               = 0x0B,
	ValidKindleUnknown_0x0C               = 0x0C,
	ValidKindleUnknown_0x0D               = 0x0D,
	ValidKindleUnknown_0x99               = 0x99,
	KindleBasicKiwi                       = 0xDD,
	/* KindlePaperWhite3 = 0x90, */    // Kindle PaperWhite 3, released summer 2015 on FW 5.6.1 (NOTE: This is a bogus ID, the proper one is now found at chars 4 to 6 of the S/N)
	KindlePaperWhite3WiFi                 = 0x201,    // 0G1
	KindlePaperWhite3WiFi3G               = 0x202,    // 0G2
	KindlePaperWhite3WiFi3GMexico         = 0x204,    // 0G4  NOTE: Might be better flagged as "Southern America"?
	KindlePaperWhite3WiFi3GEurope         = 0x205,    // 0G5
	KindlePaperWhite3WiFi3GCanada         = 0x206,    // 0G6
	KindlePaperWhite3WiFi3GJapan          = 0x207,    // 0G7
	// Kindle PaperWhite 3, White, appeared w/ FW 5.7.3.1, released summer 2016 on FW 5.7.x?
	KindlePaperWhite3WhiteWiFi            = 0x26B,            // 0KB
	KindlePaperWhite3WhiteWiFi3GJapan     = 0x26C,            // 0KC
	KindlePW3WhiteUnknown_0KD             = 0x26D,            // 0KD?
	KindlePaperWhite3WhiteWiFi3GInternational    = 0x26E,     // 0KE
	KindlePaperWhite3WhiteWiFi3GInternationalBis = 0x26F,     // 0KF
	KindlePW3WhiteUnknown_0KG                    = 0x270,     // 0KG?
	KindlePaperWhite3BlackWiFi32GBJapan          = 0x293,     // 0LK
	KindlePaperWhite3WhiteWiFi32GBJapan          = 0x294,     // 0LL
	KindlePW3Unknown_TTT                         = 0x6F7B,    // TTT?
	// Kindle Oasis, released late spring 2016 on FW 5.7.1.1
	KindleOasisWiFi                              = 0x20C,    // 0GC
	KindleOasisWiFi3G                            = 0x20D,    // 0GD
	KindleOasisWiFi3GInternational               = 0x219,    // 0GR
	KindleOasisUnknown_0GS                       = 0x21A,    // 0GS?
	KindleOasisWiFi3GChina                       = 0x21B,    // 0GT
	KindleOasisWiFi3GEurope                      = 0x21C,    // 0GU
	// Kindle Basic 2, released summer 2016 on FW 5.8.0
	KindleBasic2Unknown_0DU       = 0x1BC,    // 0DU??  FIXME: A good ID to check the sanity of my base32 tweaks...
	KindleBasic2                  = 0x269,    // 0K9 (Black)
	KindleBasic2White             = 0x26A,    // 0KA (White)
	// Kindle Oasis 2, released winter 2017 on FW 5.9.0.6
	KindleOasis2Unknown_0LM       = 0x295,    // 0LM?
	KindleOasis2Unknown_0LN       = 0x296,    // 0LN?
	KindleOasis2Unknown_0LP       = 0x297,    // 0LP?
	KindleOasis2Unknown_0LQ       = 0x298,    // 0LQ?
	KindleOasis2WiFi32GBChampagne = 0x2E1,    // 0P1
	KindleOasis2Unknown_0P2       = 0x2E2,    // 0P2?
	KindleOasis2Unknown_0P6 = 0x2E6,    // 0P6 (FIXME: Seen in the wild, WiFi+4G, 32GB, Graphite, not enough info)
	KindleOasis2Unknown_0P7 = 0x2E7,    // 0P7?
	KindleOasis2WiFi8GB     = 0x2E8,    // 0P8
	KindleOasis2WiFi3G32GB  = 0x341,    // 0S1
	KindleOasis2WiFi3G32GBEurope      = 0x342,    // 0S2
	KindleOasis2Unknown_0S3           = 0x343,    // 0S3?
	KindleOasis2Unknown_0S4           = 0x344,    // 0S4?
	KindleOasis2Unknown_0S7           = 0x347,    // 0S7?
	KindleOasis2WiFi32GB              = 0x34A,    // 0SA
	// Kindle PaperWhite 4, released November 7 2018 on FW 5.10.0.1/5.10.0.2
	KindlePaperWhite4WiFi8GB          = 0x2F7,    // 0PP
	KindlePaperWhite4WiFi4G32GB       = 0x361,    // 0T1
	KindlePaperWhite4WiFi4G32GBEurope = 0x362,    // 0T2
	KindlePaperWhite4WiFi4G32GBJapan  = 0x363,    // 0T3
	KindlePaperWhite4Unknown_0T4      = 0x364,    // 0T4?
	KindlePaperWhite4Unknown_0T5      = 0x365,    // 0T5?
	KindlePaperWhite4WiFi32GB         = 0x366,    // 0T6
	KindlePaperWhite4Unknown_0T7      = 0x367,    // 0T7?
	KindlePaperWhite4Unknown_0TJ      = 0x372,    // 0TJ?
	KindlePaperWhite4Unknown_0TK      = 0x373,    // 0TK?
	KindlePaperWhite4Unknown_0TL      = 0x374,    // 0TL?
	KindlePaperWhite4Unknown_0TM      = 0x375,    // 0TM?
	KindlePaperWhite4Unknown_0TN      = 0x376,    // 0TN?
	KindlePaperWhite4WiFi8GBIndia     = 0x402,    // 102 NOTE: Appeared in 5.10.1.3...
	KindlePaperWhite4WiFi32GBIndia    = 0x403,    // 103
	KindlePaperWhite4WiFi32GBBlue     = 0x4D8,    // 16Q (Twilight Blue, ??) NOTE: Appeared in 5.11.2...
	KindlePaperWhite4WiFi32GBPlum     = 0x4D9,    // 16R
	KindlePaperWhite4WiFi32GBSage     = 0x4DA,    // 16S
	KindlePaperWhite4WiFi8GBBlue      = 0x4DB,    // 16T (Twilight Blue, DE)
	KindlePaperWhite4WiFi8GBPlum      = 0x4DC,    // 16U (Plum. New batch of colors released summer 2020, on 5.12.3)
	KindlePaperWhite4WiFi8GBSage      = 0x4DD,    // 16V (Sage. Ditto)
	KindlePW4Unknown_0PL              = 0x2F4,    // 0PL?
	// Kindle Basic 3, released April 10 2019 on FW 5.1x.y
	KindleBasic3                      = 0x414,    // 10L
	KindleBasic3White8GB              = 0x3CF,    // 0WF (White, WiFi, DE. 4GB -> 8GB)
	KindleBasic3Unknown_0WG           = 0x3D0,    // 0WG?
	KindleBasic3White                 = 0x3D1,    // 0WH
	KindleBasic3Unknown_0WJ           = 0x3D2,    // 0WJ?
	KindleBasic3KidsEdition = 0x3AB,    // 0VB NOTE: Ships on a custom OTA-only FW branch. May be a special snowflake.
	// Kindle Oasis 3, released July 24 2019 on FW 5.12.0
	KindleOasis3WiFi32GBChampagne     = 0x434,    // 11L (Champagne, US)
	KindleOasis3WiFi4G32GBJapan       = 0x3D8,    // 0WQ (Graphite, JP)
	KindleOasis3WiFi4G32GBIndia       = 0x3D7,    // 0WP (Graphite, IN)
	KindleOasis3WiFi4G32GB            = 0x3D6,    // 0WN (Graphite, US)
	KindleOasis3WiFi32GB              = 0x3D5,    // 0WM (Graphite, DE)
	KindleOasis3WiFi8GB               = 0x3D4,    // 0WL (Graphite, DE)
	// Kindle PaperWhite 5, released October 27 2021 on FW 5.14.0
	KindlePaperWhite5SignatureEdition = 0x690,    // 1LG (Black, 32GB, US)
	KindlePaperWhite5Unknown_1Q0      = 0x700,    // 1Q0?
	KindlePaperWhite5                 = 0x6FF,    // 1PX (Black & White, 8GB, UK, FR, IT)
	KindlePaperWhite5Unknown_1VD      = 0x7AD,    // 1VD?
	KindlePaperWhite5SE_219           = 0x829,    // 219 (SE, 32GB, Denim, US)
	KindlePaperWhite5_21A             = 0x82A,    // 21A
	KindlePaperWhite5SE_2BH           = 0x971,    // 2BH NOTE: Appeared in 5.14.2... (SE)
	KindlePaperWhite5Unknown_2BJ      = 0x972,    // 2BJ?
	KindlePaperWhite5_2DK             = 0x9B3,    // 2DK NOTE: Appeared in 5.14.3... (Black, Kids or not, US)
	// Kindle Basic 4, released October 12 2022 on FW 5.15.0
	KindleBasic4Unknown_22D           = 0x84D,    // 22D?
	KindleBasic4Unknown_25T           = 0x8BB,    // 25T?
	KindleBasic4Unknown_23A           = 0x86A,    // 23A?
	KindleBasic4_2AQ                  = 0x958,    // 2AQ (Refurb seen in the wild)
	KindleBasic4_2AP                  = 0x957,    // 2AP (Seen in the wild, possibly EU-ish)
	KindleBasic4Unknown_1XH           = 0x7F1,    // 1XH?
	KindleBasic4Unknown_22C           = 0x84C,    // 22C?
	// Kindle Scribe, released December 2022 on FW 5.16.0
	KindleScribeUnknown_27J           = 0x8F2,    // 27J?
	KindleScribeUnknown_2BL           = 0x974,    // 2BL?
	KindleScribeUnknown_263           = 0x8C3,    // 263?
	KindleScribe16GB_227              = 0x847,    // 227 (JP, 16GB, Premium Pen)
	KindleScribeUnknown_2BM           = 0x975,    // 2BM?
	KindleScribe_23L                  = 0x874,    // 23L
	KindleScribe64GB_23M              = 0x875,    // 23M (US, 64GB, Premium Pen)
	KindleScribeUnknown_270           = 0x8E0,    // 270?
	// Kindle Basic 5, released October 2024 on FW 5.17.x
	KindleBasic5Unknown_3L5           = 0xE85,     // 3L5?
	KindleBasic5Unknown_3L6           = 0xE86,     // 3L6?
	KindleBasic5Unknown_3L4           = 0xE84,     // 3L4?
	KindleBasic5Unknown_3L3           = 0xE83,     // 3L3?
	KindleBasic5Unknown_A89           = 0x2909,    // A89?
	KindleBasic5Unknown_3L2           = 0xE82,     // 3L2?
	KindleBasic5Unknown_3KM           = 0xE75,     // 3KM
	// Kindle PaperWhite 6, released October 2024 on FW 5.17.x
	KindlePaperWhite6Unknown_349      = 0xC89,    // 349?
	KindlePaperWhite6Unknown_346      = 0xC86,    // 346?
	KindlePaperWhite6Unknown_33X      = 0xC7F,    // 33X
	KindlePaperWhite6Unknown_33W      = 0xC7E,    // 33W?
	KindlePaperWhite6Unknown_3HA      = 0xE2A,    // 3HA?
	KindlePaperWhite6Unknown_3H5      = 0xE25,    // 3H5?
	KindlePaperWhite6Unknown_3H3      = 0xE23,    // 3H3?
	KindlePaperWhite6Unknown_3H8      = 0xE28,    // 3H8?
	KindlePaperWhite6Unknown_3J5      = 0xE45,    // 3J5?
	KindlePaperWhite6Unknown_3JS      = 0xE5A,    // 3JS?
	// Kindle Scribe 2, released October 2024 on FW 5.17.x
	KindleScribe2Unknown_3V0		  = 0xFA0,     // 3V0?
	KindleScribe2Unknown_3V1		  = 0xFA1,     // 3V1?
	KindleScribe2Unknown_3X5		  = 0xFE5,     // 3X5?
	KindleScribe2Unknown_3UV		  = 0xF9D,     // 3UV?
	KindleScribe2Unknown_3X4		  = 0xFE4,     // 3X4?
	KindleScribe2Unknown_3X3		  = 0xFE3,     // 3X3?
	KindleScribe2Unknown_41E		  = 0x102E,    // 41E?
	KindleScribe2Unknown_41D		  = 0x102D,    // 41D?
	// Kindle ColorSoft, released October 2024 on FW 5.18.0
	KindleColorSoftUnknown_3H9        = 0xE29,     // 3H9?
	KindleColorSoftUnknown_3H4        = 0xE24,     // 3H4?
	KindleColorSoftUnknown_3HB        = 0xE2B,     // 3HB?
	KindleColorSoftUnknown_3H6        = 0xE26,     // 3H6?
	KindleColorSoftUnknown_3H2        = 0xE22,     // 3H2?
	KindleColorSoftUnknown_34X        = 0xC9F,     // 34X?
	KindleColorSoftUnknown_3H7        = 0xE27,     // 3H7
	KindleColorSoftUnknown_3JT        = 0xE5B,     // 3JT?
	KindleColorSoftUnknown_3J6        = 0xE46,     // 3J6?
	KindleColorSoftUnknown_456        = 0x10A6,    // 456?
	KindleUnknown                     = 0x00
} Device;

typedef enum
{
	Plat_Unspecified = 0x00,
	MarioDeprecated  = 0x01,    // Kindle 2
	Luigi            = 0x02,    // Kindle 3
	Banjo            = 0x03,    // ??
	Yoshi            = 0x04,    // Kindle Touch (and Kindle 4)
	YoshimeProto     = 0x05,    // Early PW proto? (NB: Platform AKA Yoshime)
	Yoshime          = 0x06,    // Kindle PW (NB: Platform AKA Yoshime3)
	Wario            = 0x07,    // Kindle PW2, Basic, Voyage, PW3
	Duet             = 0x08,    // Kindle Oasis
	Heisenberg       = 0x09,    // Kindle Basic 2 (8th gen)
	Zelda            = 0x0A,    // Kindle Oasis 2, Oasis 3
	Rex              = 0x0B,    // Kindle PW4, Basic 3 (10th gen)
	Bellatrix        = 0x0C,    // Kindle PW5 (11th gen), Basic 4
	Bellatrix3       = 0x0D,    // Kindle Scribe
	Bellatrix4       = 0x0E,    // Kindle PW6 (12th gen), ColorSoft
} Platform;

typedef enum
{
	Board_Unspecified = 0x00,    // Used since the PW (skip board check)
	Tequila           = 0x03,    // Silver Kindle 4
	Whitney           = 0x05     // Kindle Touch
				     // Other potentially relevant (OTA|Recovery)v2 ready boards:
				     /*
	Sauza             = 0xFF     // Black Kindle 4
	Celeste           = 0xFF     // Kindle PW
	Icewine           = 0xFF     // Kindle Voyage (also a dev/proto on the Yoshime3 platform)
	Pinot             = 0xFF     // Kindle PW2
	Bourbon           = 0xFF     // Kindle Basic
	Muscat            = 0xFF     // Kindle PW3
	Whisky            = 0xFF     // Kindle Oasis
	Woody             = 0xFF     // ?? (in the Basic line? (no 3G))
	Eanab             = 0xFF     // Kindle Basic 2
	Cognac            = 0xFF     // Kindle Oasis 2
	Moonshine         = 0xFF     // Kindle PW4
	Jaeger            = 0xFF     // Kindle Basic 3
	Stinger           = 0xFF     // Kindle Oasis 3
	Malbec            = 0xFF     // Kindle PW5
	Cava              = 0xFF     // Kindle Basic 4
	Barolo            = 0xFF     // Kindle Scribe
	Rossini           = 0xFF     // Kindle Basic 5
	Sangria           = 0xFF     // Kindle PW6
	SeaBreeze         = 0xFF     // Kindle ColorSoft
				     */
} Board;

// For reference, list of boards (AFAICT, in chronological order), trailing name is the inane marketing name used on the *US* market:
// ADS                        // K1 proto? (w/ ETH)
// Fiona                      // Kindle 1 - Kindle (1st Generation)
// Mario                      // Kindle 2? (w/ ETH) [Also a platform]
// Nell/NellSL/NellWW         // DX & DXG & DXi? - Kindle DX (2nd Generation)
// Turing/TuringWW            // Kindle 2 & Kindle 2 International - Kindle (2nd Generation)
// Luigi/Luigi3               // ?? (r3 w/ ETH) [Also a platform]
// Shasta (+ WFO variant)     // Kindle 3 - Kindle Keyboard (Wi-Fi), Kindle Keyboard 3G (Free 3G + Wi-Fi) (3rd Generation)
// Yoshi                      // ?? [Also a platform]
// Primer                     // Deprecated proto
// Harv                       // K4 proto?
// Tequila (is WFO)           // Silver Kindle 4 - Kindle Wi-Fi, 6" E Ink Display (4th and 5th Generation)
// Sauza                      // Black Kindle 4? (NOT in chronological order)
// Finkle                     // Touch proto?
// Whitney (+ WFO variant)    // Kindle Touch - Kindle Touch, Kindle Touch 3G (Free 3G + Wi-Fi) (4th Generation)
// Yoshime                    // Temp. Yoshime dev board [Also a Platform, which we call YoshimeProto]
// Yoshime3                   // Temp. Yoshime3 dev boards (w/ ETH). PW proto? [Also a Platform, which we call Yoshime]
// Celeste (+ WFO variant)    // Kindle PW - Kindle Paperwhite (5th Generation)
// Icewine (+ WFO variants)   // Dev/Proto, next rumored product [Used on two different platforms (so far), Yoshime3 & Wario]
// Wario                      // Temp. Wario dev boards [Also a Platform]
// Pinot (+ WFO variant)      // Kindle PW2 - Kindle Paperwhite (6th Generation)
// Bourbon                    // Kindle Basic (KT2) - Kindle (7th Generation)
// Icewine (on Wario)         // Kindle Voyage - Kindle Voyage (7th Generation)
// Muscat                     // Kindle PW3 - Kindle Paperwhite (7th Generation)
// Whisky                     // Kindle Oasis - Kindle Oasis (8th Generation)
// Woody                      // ?? (Dev/Proto? Duet platform, Basic line)
// Eanab                      // Kindle Basic 2 (KT3) - Kindle (8th Generation)
// Cognac                     // Kindle Oasis 2 - Kindle Oasis (9th Generation)
// Moonshine                  // Kindle PW4 - Kindle Paperwhite (10th Generation)
// Jaeger                     // Kindle Basic 3 (KT4) - Kindle (10th Generation)
// Stinger                    // Kindle Oasis 3 - Kindle Oasis (10th Generation)
// Malbec                     // Kindle PW5 (First Bellatrix board. No longer an i.MX SoC, but a MediaTek one: MT8110, likely based on the MT8512) - Kindle Paperwhite (11th Generation)
// Cava                       // Kindle Basic 4 (KT5) [Kindle 11th gen] - Kindle (11th Generation)
// Barolo                     // Kindle Scribe (First Bellatrix3 board) - Kindle Scribe
// Rossini                    // Kindle Basic 5 (KT6) [Kindle 11th gen - 2024] - Kindle (11th Generation) - 2024 Release
// Sangria                    // Kindle PW6 (First Bellatrix4 board w/ the CS) - Kindle Paperwhite (12th Generation) - 2024 Release
// SeaBreeze                  // Kindle CS - Kindle ColorSoft

typedef struct
{
	CertificateNumber certificate_number;
} UpdateSignatureHeader;

typedef struct
{
	uint32_t      source_revision;
	uint32_t      target_revision;
	uint16_t      device;
	unsigned char optional;
	unsigned char unused;
	char          md5_sum[MD5_HASH_LENGTH];
} OTAUpdateHeader;

typedef struct
{
	unsigned char unused[12];
	char          md5_sum[MD5_HASH_LENGTH];
	uint32_t      magic_1;
	uint32_t      magic_2;
	uint32_t      minor;
	uint32_t      device;
} RecoveryUpdateHeader;

typedef struct
{
	unsigned char foo[4];
	uint64_t      target_revision;    // NOTE: This would enforce 8 bytes padding/alignment, hence the packing
	char          md5_sum[MD5_HASH_LENGTH];
	uint32_t      magic_1;
	uint32_t      magic_2;
	uint32_t      minor;
	uint32_t      platform;
	uint32_t      header_rev;
	uint32_t      board;
} __attribute__((packed)) RecoveryH2UpdateHeader;    // FB02 with V2 Header, not FB03

typedef struct
{
	char magic_number[MAGIC_NUMBER_LENGTH] __attribute__((nonstring));
	union
	{
		OTAUpdateHeader        ota_update;
		RecoveryUpdateHeader   recovery_update;
		RecoveryH2UpdateHeader recovery_h2_update;
		UpdateSignatureHeader  signature;
		unsigned char          ota_header_data[OTA_UPDATE_BLOCK_SIZE];
		unsigned char          signature_header_data[UPDATE_SIGNATURE_BLOCK_SIZE];
		unsigned char          recovery_header_data[RECOVERY_UPDATE_BLOCK_SIZE];
	} data;
} UpdateHeader;

// Ugly global. Used to cache the state of the KT_WITH_UNKNOWN_DEVCODES env var...
// NOTE: While this looks like the ideal candidate to be a bool,
//       we can't do that because we use its value in unsigned operations,
//       and I can't be arsed to add a bunch of casts there (because for some mystical reason, bool is signed :?)
extern unsigned int kt_with_unknown_devcodes;

// Another for the shell metadata dumps in convert
extern const char* kt_pkg_metadata_dump;

// And another to store the tmpdir...
extern char kt_tempdir[PATH_MAX];

uint32_t from_base(const char*, uint8_t);

void          md(unsigned char*, size_t);
void          dm(unsigned char*, size_t);
int           munger(FILE*, FILE*, size_t, const bool);
int           demunger(FILE*, FILE*, size_t, const bool);
const char*   convert_device_id(Device) __attribute__((const));
const char*   convert_platform_id(Platform) __attribute__((const));
const char*   convert_board_id(Board) __attribute__((const));
BundleVersion get_bundle_version(const char[MAGIC_NUMBER_LENGTH]) __attribute__((pure));
int           md5_sum(FILE*, char output_string[BASE16_ENCODE_LENGTH(MD5_DIGEST_SIZE)]);
int           sha256_sum(FILE*, char output_string[BASE16_ENCODE_LENGTH(SHA256_DIGEST_SIZE)]);

int kindle_convert_main(int, char**);

int kindle_extract_main(int, char**);

int kindle_create_main(int, char**);

int nettle_rsa_privkey_from_pem(const char*, struct rsa_private_key*);

#endif
