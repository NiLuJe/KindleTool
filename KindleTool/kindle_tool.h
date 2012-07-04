//
//  kindle_tool.h
//  KindleTool
//
//  Copyright (C) 2011-2012  Yifan Lu
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef KINDLETOOL
#define KINDLETOOL

#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <archive.h>
#include <archive_entry.h>
#include <limits.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

// Die slightly more gracefully than spewing a whole lot of warnings & errors if we're not building against at least libarchive 3.0.3
#if ARCHIVE_VERSION_NUMBER < 3000003
#error Your libarchive version is too old, KindleTool depends on libarchive >= 3.0.3
#endif

#define BUFFER_SIZE 1024
#define BLOCK_SIZE 64

#define MAGIC_NUMBER_LENGTH 4
#define MD5_HASH_LENGTH 32

#define OTA_UPDATE_BLOCK_SIZE 60
#define OTA_UPDATE_V2_BLOCK_SIZE 18
#define OTA_UPDATE_V2_PART_2_BLOCK_SIZE 36
#define RECOVERY_UPDATE_BLOCK_SIZE 131068
#define UPDATE_SIGNATURE_BLOCK_SIZE 60

#define CERTIFICATE_DEV_SIZE 128
#define CERTIFICATE_1K_SIZE 128
#define CERTIFICATE_2K_SIZE 256

#define INDEX_FILE_NAME "update-filelist.dat"

#define SERIAL_NO_LENGTH 16

#define DEFAULT_BYTES_PER_BLOCK (20*512)

#define IS_SCRIPT(filename) (strncasecmp(filename+(strlen(filename)-4), ".ffs", 4) == 0)
#define IS_SHELL(filename) (strncasecmp(filename+(strlen(filename)-3), ".sh", 3) == 0)
#define IS_SIG(filename) (strncasecmp(filename+(strlen(filename)-4), ".sig", 4) == 0)
#define IS_BIN(filename) (strncasecmp(filename+(strlen(filename)-4), ".bin", 4) == 0)
#define IS_TGZ(filename) (strncasecmp(filename+(strlen(filename)-4), ".tgz", 4) == 0)
#define IS_TARBALL(filename) (strncasecmp(filename+(strlen(filename)-7), ".tar.gz", 7) == 0)
#define IS_DAT(filename) (strncasecmp(filename+(strlen(filename)-4), ".dat", 4) == 0)

// Version tag fallback
#ifndef KT_VERSION
#define KT_VERSION "v1.3.1-GIT"
#endif

// user@host tag fallback
#ifndef KT_USERATHOST
#define KT_USERATHOST "someone@somewhere"
#endif

// GCC version checks... (We check !clang in addition to GCC, because Clang 'helpfully' defines __GNUC__ ...)
#if !defined(__clang__) && defined(__GNUC__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

typedef enum
{
    UpdateSignature,
    OTAUpdateV2,
    OTAUpdate,
    RecoveryUpdate,
    UnknownUpdate = -1
} BundleVersion;

typedef enum
{
    CertificateDeveloper = 0x00,
    Certificate1K = 0x01,
    Certificate2K = 0x02,
    CertificateUnknown = 0xFF
} CertificateNumber;

typedef enum
{
    Kindle1 = 0x01,
    Kindle2US = 0x02,
    Kindle2International = 0x03,
    KindleDXUS = 0x04,
    KindleDXInternational = 0x05,
    KindleDXGraphite = 0x09,
    Kindle3Wifi = 0x08,
    Kindle3Wifi3G = 0x06,
    Kindle3Wifi3GEurope = 0x0A,
    Kindle4NonTouch = 0x0E,
    Kindle5TouchWifi3G = 0x0F,
    Kindle5TouchWifi = 0x11,
    Kindle5TouchWifi3GEurope = 0x10,
    Kindle5TouchUnknown = 0x12,
    KindleUnknown = 0x00
} Device;

typedef struct
{
    CertificateNumber certificate_number;
} UpdateSignatureHeader;

typedef struct
{
    uint32_t source_revision;
    uint32_t target_revision;
    uint16_t device;
    unsigned char optional;
    unsigned char unused;
    char md5_sum[MD5_HASH_LENGTH];
} OTAUpdateHeader;

typedef struct
{
    unsigned char unused[12];
    char md5_sum[MD5_HASH_LENGTH];
    uint32_t magic_1;
    uint32_t magic_2;
    uint32_t minor;
    uint32_t device;
} RecoveryUpdateHeader;

typedef struct
{
    char magic_number[MAGIC_NUMBER_LENGTH];
    union
    {
        OTAUpdateHeader ota_update;
        RecoveryUpdateHeader recovery_update;
        UpdateSignatureHeader signature;
        unsigned char ota_header_data[OTA_UPDATE_BLOCK_SIZE];
        unsigned char signature_header_data[UPDATE_SIGNATURE_BLOCK_SIZE];
        unsigned char recovery_header_data[RECOVERY_UPDATE_BLOCK_SIZE];
    } data;
} UpdateHeader;

typedef struct
{
    char magic_number[MAGIC_NUMBER_LENGTH];
    BundleVersion version;
    RSA *sign_pkey;
    uint64_t source_revision;
    uint64_t target_revision;
    uint32_t magic_1;
    uint32_t magic_2;
    uint32_t minor;
    uint16_t num_devices;
    Device *devices;
    CertificateNumber certificate_number;
    unsigned char optional;
    unsigned char critical;
    uint16_t num_meta;
    char **metastrings;
} UpdateInformation;

// This is modeled after libarchive's bsdtar...
struct kttar
{
    char *buff;
    size_t buff_size;
};

void md(unsigned char *, size_t);
void dm(unsigned char *, size_t);
int munger(FILE *, FILE *, size_t, const int);
int demunger(FILE *, FILE *, size_t, const int);
const char *convert_device_id(Device);
const char *convert_bundle_version(BundleVersion);
BundleVersion get_bundle_version(char *);
int md5_sum(FILE *, char *);
RSA *get_default_key(void);
int kindle_print_help(const char *);
int kindle_print_version(const char *);
int kindle_deobfuscate_main(int, char **);
int kindle_obfuscate_main(int, char **);
int kindle_info_main(int, char **);

int kindle_read_bundle_header(UpdateHeader *, FILE *);
int kindle_convert(FILE *, FILE *, FILE *, const int);
int kindle_convert_ota_update_v2(FILE *, FILE *, const int);
int kindle_convert_signature(UpdateHeader *, FILE *, FILE *);
int kindle_convert_ota_update(UpdateHeader *, FILE *, FILE *, const int);
int kindle_convert_recovery(UpdateHeader *, FILE *, FILE *, const int);
int kindle_convert_main(int, char **);
int libarchive_copy_data(struct archive *, struct archive *);
int libarchive_extract(const char *, const char *);
int kindle_extract_main(int, char **);

int sign_file(FILE *, RSA *, FILE *);
int kindle_create_package_archive(const int, char **, const int, RSA *);
int kindle_create(UpdateInformation *, FILE *, FILE *, const int);
int kindle_create_ota_update_v2(UpdateInformation *, FILE *, FILE *, const int);
int kindle_create_signature(UpdateInformation *, FILE *, FILE *);
int kindle_create_ota_update(UpdateInformation *, FILE *, FILE *, const int);
int kindle_create_recovery(UpdateInformation *, FILE *, FILE *, const int);
int kindle_create_main(int, char **);

#endif

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
