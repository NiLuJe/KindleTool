//
//  extract.c
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

#include "kindle_tool.h"

int kindle_read_bundle_header(UpdateHeader *header, FILE *input)
{
    if(fread(header, sizeof(char), MAGIC_NUMBER_LENGTH, input) < 1 || ferror(input) != 0)
    {
        return -1;
    }
    return 0;
}

int kindle_convert(FILE *input, FILE *output, FILE *sig_output, const int fake_sign)
{
    UpdateHeader header;
    BundleVersion bundle_version;
    if(kindle_read_bundle_header(&header, input) < 0)
    {
        fprintf(stderr, "Cannot read input file.\n");
        return -1;
    }
    fprintf(stderr, "Bundle         %s\n", header.magic_number);
    bundle_version = get_bundle_version(header.magic_number);
    switch(bundle_version)
    {
        case OTAUpdateV2:
            fprintf(stderr, "Bundle Type    %s\n", "OTA V2");
            return kindle_convert_ota_update_v2(input, output, fake_sign); // no absolute size, so no struct to pass
            break;
        case UpdateSignature:
            if(kindle_convert_signature(&header, input, sig_output) < 0)
            {
                fprintf(stderr, "Cannot extract signature file!\n");
                return -1;
            }
            return kindle_convert(input, output, sig_output, fake_sign);
            break;
        case OTAUpdate:
            fprintf(stderr, "Bundle Type    %s\n", "OTA V1");
            return kindle_convert_ota_update(&header, input, output, fake_sign);
            break;
        case RecoveryUpdate:
            fprintf(stderr, "Bundle Type    %s\n", "Recovery");
            return kindle_convert_recovery(&header, input, output, fake_sign);
            break;
        case UnknownUpdate:
        default:
            fprintf(stderr, "Unknown update bundle version!\n");
            break;
    }
    return -1; // if we get here, there has been an error
}

int kindle_convert_ota_update_v2(FILE *input, FILE *output, const int fake_sign)
{
    char *data;
    unsigned int hindex;
    uint64_t source_revision;
    uint64_t target_revision;
    uint16_t num_devices;
    uint16_t device;
    //uint16_t *devices;
    uint8_t critical;
    uint8_t padding;
    char *pkg_md5_sum;
    uint16_t num_metadata;
    size_t meta_strlen;
    uint16_t metastring_length;
    char *metastring;
    //unsigned char **metastrings;
    size_t read_size __attribute__((unused));

    // First read the set block size and determine how much to resize
    data = malloc(OTA_UPDATE_V2_BLOCK_SIZE * sizeof(char));
    read_size = fread(data, sizeof(char), OTA_UPDATE_V2_BLOCK_SIZE, input);
    hindex = 0;

    source_revision = *(uint64_t *)&data[hindex];
    hindex += sizeof(uint64_t);
    fprintf(stderr, "Minimum OTA    %llu\n", (long long) source_revision);
    target_revision = *(uint64_t *)&data[hindex];
    hindex += sizeof(uint64_t);
    fprintf(stderr, "Target OTA     %llu\n", (long long) target_revision);
    num_devices = *(uint16_t *)&data[hindex];
    //hindex += sizeof(uint16_t);       // Shut clang's sa up
    fprintf(stderr, "Devices        %hd\n", num_devices);
    free(data);

    // Now get the data
    data = malloc(num_devices * sizeof(uint16_t));
    read_size = fread(data, sizeof(uint16_t), num_devices, input);
    for(hindex = 0; hindex < num_devices * sizeof(uint16_t); hindex += sizeof(uint16_t))
    {
        device = *(uint16_t *)&data[hindex];
        // Slightly hackish way to detect unknown devices, because I don't want to refactor convert_device_id()
        if(strcmp(convert_device_id(device), "Unknown") == 0)
            fprintf(stderr, "Device         Unknown (0x%02X)\n", device);
        else
            fprintf(stderr, "Device         %s\n", convert_device_id(device));
    }
    free(data);

    // Now get second part of set sized data
    data = malloc(OTA_UPDATE_V2_PART_2_BLOCK_SIZE * sizeof(char));
    read_size = fread(data, sizeof(char), OTA_UPDATE_V2_PART_2_BLOCK_SIZE, input);
    hindex = 0;

    critical = *(uint8_t *)&data[hindex];       // Apparently critical really is supposed to be 1 byte + 1 padding byte, so obey that...
    hindex += sizeof(uint8_t);
    fprintf(stderr, "Critical       %hhu\n", critical);
    padding = *(uint8_t *)&data[hindex];        // Print the (garbage?) padding byte found in official updates...
    hindex += sizeof(uint8_t);
    fprintf(stderr, "Padding Byte   %hhu (%02X)\n", padding, padding);
    pkg_md5_sum = &data[hindex];
    dm((unsigned char *)pkg_md5_sum, MD5_HASH_LENGTH);
    hindex += MD5_HASH_LENGTH;
    fprintf(stderr, "MD5 Hash       %.*s\n", MD5_HASH_LENGTH, pkg_md5_sum);
    num_metadata = *(uint16_t *)&data[hindex];
    //hindex += sizeof(uint16_t);       // Shut clang's sa up
    fprintf(stderr, "Metadata       %hd\n", num_metadata);
    free(data);

    // Finally, get the metastrings
    for(hindex = 0; hindex < num_metadata; hindex++)
    {
        // Get correct meta string length because of the endianness swap...
        read_size = fread(&((uint8_t *)&meta_strlen)[1], sizeof(uint8_t), 1, input);
        read_size = fread(&((uint8_t *)&meta_strlen)[0], sizeof(uint8_t), 1, input);
        metastring_length = meta_strlen;
        metastring = malloc(metastring_length);
        read_size = fread(metastring, sizeof(char), metastring_length, input);
        dm((unsigned char *)metastring, metastring_length);      // Deobfuscate string (FIXME: Should meta strings really be obfuscated?)
        fprintf(stderr, "Metastring     %.*s\n", metastring_length, metastring);
        free(metastring);
    }

    if(ferror(input) != 0)
    {
        fprintf(stderr, "Cannot read update correctly.\n");
        return -1;
    }

    if(output == NULL)
    {
        return 0;
    }

    // Now we can decrypt the data
    return demunger(input, output, 0, fake_sign);
}

int kindle_convert_signature(UpdateHeader *header, FILE *input, FILE *output)
{
    CertificateNumber cert_num;
    char *cert_name;
    size_t seek;
    unsigned char *signature;

    if(fread(header->data.signature_header_data, sizeof(char), UPDATE_SIGNATURE_BLOCK_SIZE, input) < UPDATE_SIGNATURE_BLOCK_SIZE)
    {
        fprintf(stderr, "Cannot read signature header.\n");
        return -1;
    }
    cert_num = (CertificateNumber)(header->data.signature.certificate_number);
    fprintf(stderr, "Cert number    %u\n", cert_num);
    switch(cert_num)
    {
        case CertificateDeveloper:
            cert_name = "pubdevkey01.pem";
            seek = CERTIFICATE_DEV_SIZE;
            break;
        case Certificate1K:
            cert_name = "pubprodkey01.pem";
            seek = CERTIFICATE_1K_SIZE;
            break;
        case Certificate2K:
            cert_name = "pubprodkey02.pem";
            seek = CERTIFICATE_2K_SIZE;
            break;
        case CertificateUnknown:
        default:
            fprintf(stderr, "Unknown signature size, cannot continue.\n");
            return -1;
            break;
    }
    fprintf(stderr, "Cert file      %s\n", cert_name);
    if(output == NULL)
    {
        return fseek(input, seek, SEEK_CUR);
    }
    else
    {
        signature = malloc(seek);
        if(fread(signature, sizeof(char), seek, input) < seek)
        {
            fprintf(stderr, "Cannot read signature!\n");
            free(signature);
            return -1;
        }
        if(fwrite(signature, sizeof(char), seek, output) < seek)
        {
            fprintf(stderr, "Cannot write signature file!\n");
            free(signature);
            return -1;
        }
        free(signature);
    }
    return 0;
}

int kindle_convert_ota_update(UpdateHeader *header, FILE *input, FILE *output, const int fake_sign)
{
    if(fread(header->data.ota_header_data, sizeof(char), OTA_UPDATE_BLOCK_SIZE, input) < OTA_UPDATE_BLOCK_SIZE)
    {
        fprintf(stderr, "Cannot read OTA header.\n");
        return -1;
    }
    dm((unsigned char *)header->data.ota_update.md5_sum, MD5_HASH_LENGTH);
    fprintf(stderr, "MD5 Hash       %.*s\n", MD5_HASH_LENGTH, header->data.ota_update.md5_sum);
    fprintf(stderr, "Minimum OTA    %u\n", header->data.ota_update.source_revision);
    fprintf(stderr, "Target OTA     %u\n", header->data.ota_update.target_revision);
    fprintf(stderr, "Device         %s\n", convert_device_id(header->data.ota_update.device));
    fprintf(stderr, "Optional       %hhu\n", header->data.ota_update.optional);

    if(output == NULL)
    {
        return 0;
    }

    return demunger(input, output, 0, fake_sign);
}

int kindle_convert_recovery(UpdateHeader *header, FILE *input, FILE *output, const int fake_sign)
{
    if(fread(header->data.recovery_header_data, sizeof(char), RECOVERY_UPDATE_BLOCK_SIZE, input) < RECOVERY_UPDATE_BLOCK_SIZE)
    {
        fprintf(stderr, "Cannot read recovery update header.\n");
        return -1;
    }
    dm((unsigned char *)header->data.recovery_update.md5_sum, MD5_HASH_LENGTH);
    fprintf(stderr, "MD5 Hash       %.*s\n", MD5_HASH_LENGTH, header->data.recovery_update.md5_sum);
    fprintf(stderr, "Magic 1        %d\n", header->data.recovery_update.magic_1);
    fprintf(stderr, "Magic 2        %d\n", header->data.recovery_update.magic_2);
    fprintf(stderr, "Minor          %d\n", header->data.recovery_update.minor);
    fprintf(stderr, "Device         %s\n", convert_device_id(header->data.recovery_update.device));

    if(output == NULL)
    {
        return 0;
    }

    return demunger(input, output, 0, fake_sign);
}

int kindle_convert_main(int argc, char *argv[])
{
    int opt;
    int opt_index;
    static const struct option opts[] =
    {
        { "stdout", no_argument, NULL, 'c' },
        { "info", no_argument, NULL, 'i' },
        { "keep", no_argument, NULL, 'k' },
        { "sig", no_argument, NULL, 's' },
        { "unsigned", no_argument, NULL, 'u' }
    };
    FILE *input;
    FILE *output;
    FILE *sig_output;
    const char *in_name;
    char *out_name = NULL;
    char *sig_name = NULL;
    size_t len;
    struct stat st;
    int info_only;
    int keep_ori;
    int extract_sig;
    int fake_sign;
    int fail;

    sig_output = NULL;
    output = NULL;
    info_only = 0;
    keep_ori = 0;
    extract_sig = 0;
    fake_sign = 0;
    fail = 1;
    while((opt = getopt_long(argc, argv, "icksu", opts, &opt_index)) != -1)
    {
        switch(opt)
        {
            case 'i':
                info_only = 1;
                break;
            case 'k':
                keep_ori = 1;
                break;
            case 'c':
                output = stdout;
                break;
            case 's':
                extract_sig = 1;
                break;
            case 'u':
                fake_sign = 1;
                break;
            default:
                fprintf(stderr, "Unknown option code 0%o\n", opt);
                break;
        }
    }
    // Don't try to output to stdout or extract the package sig if we asked for info only
    if(info_only)
    {
        output = NULL;
        extract_sig = 0;
    }
    // Don't try to extract the signature of an unsiged package
    if(fake_sign)
        extract_sig = 0;

    if(optind < argc)
    {
        // Iterate over non-options (the file(s) we passed) (stdout output is probably pretty dumb when passing multiple files...)
        while(optind < argc)
        {
            fail = 0;
            in_name = argv[optind++];
            // Check that a valid package input properly ends in .bin, unless we just want to parse the header
            if(!fake_sign && !info_only && !IS_BIN(in_name))
            {
                fprintf(stderr, "The input file must be a '.bin' update package.\n");
                fail = 1;
                continue;   // It's fatal, go away
            }
            if(!info_only && output != stdout) // not info only AND not stdout
            {
                len = strlen(in_name);
                out_name = malloc(len + 1 + (3 - fake_sign));
                memcpy(out_name, in_name, len - (4 + fake_sign));
                out_name[len - (4 + fake_sign)] = 0;    // . => \0
                strncat(out_name, ".tar.gz", 7);
                if((output = fopen(out_name, "wb")) == NULL)
                {
                    fprintf(stderr, "Cannot open output '%s' for writing.\n", out_name);
                    fail = 1;
                    free(out_name);
                    continue;   // It's fatal, go away
                }
            }
            if(extract_sig) // we want the package sig (implies not info only)
            {
                len = strlen(in_name);
                sig_name = malloc(len + 1);
                memcpy(sig_name, in_name, len - 4);
                sig_name[len - 4] = 0;  // . => \0
                strncat(sig_name, ".sig", 4);
                if((sig_output = fopen(sig_name, "wb")) == NULL)
                {
                    fprintf(stderr, "Cannot open signature output '%s' for writing.\n", sig_name);
                    fail = 1;
                    if(!info_only && output != stdout)
                        free(out_name);
                    free(sig_name);
                    continue;   // It's fatal, go away
                }
            }
            if((input = fopen(in_name, "rb")) == NULL)
            {
                fprintf(stderr, "Cannot open input '%s' for reading.\n", in_name);
                fail = 1;
                if(!info_only && output != stdout)
                    free(out_name);
                if(extract_sig)
                    free(sig_name);
                continue;   // It's fatal, go away
            }
            // If we're outputting to stdout, set a dummy human readable output name
            if(!info_only && output == stdout)
            {
                out_name = strdup("standard output");
            }
            // Print a recap of what we're doing
            if(info_only)
            {
                fprintf(stderr, "Checking %supdate package %s\n", (fake_sign ? "fake " : ""), in_name);
            }
            else
            {
                fprintf(stderr, "Converting %supdate package %s to %s (%s, %s)\n", (fake_sign ? "fake " : ""), in_name, out_name, (extract_sig ? "with sig" : "without sig"), (keep_ori ? "keep input" : "delete input"));
            }
            if(kindle_convert(input, output, sig_output, fake_sign) < 0)
            {
                fprintf(stderr, "Error converting update '%s'.\n", in_name);
                if(output != NULL && output != stdout)
                    unlink(out_name); // clean up our mess, if we made one
                fail = 1;
            }
            if(output != stdout && !info_only && !keep_ori && !fail) // if output was some file, and we didn't ask to keep it, and we didn't fail to convert it, delete the original
                unlink(in_name);

            // Cleanup behind us
            if(output != NULL && output != stdout)
                fclose(output);
            if(input != NULL)
                fclose(input);
            if(sig_output != NULL)
                fclose(sig_output);
            free(out_name);
            // Remove empty sigs (since we have to open the fd before calling kindle_convert, we end up with an empty file for packages that aren't wrapped in an UpdateSignature)
            if(extract_sig)
            {
                stat(sig_name, &st);
                if(st.st_size == 0)
                    unlink(sig_name);
            }
            free(sig_name);

            // If we're not the last file, throw an LF to untangle the output
            if(optind < argc)
                fprintf(stderr, "\n");
        }
    }
    else
    {
        fprintf(stderr, "No input specified.\n");
        return -1;
    }

    // Return
    if(fail)
        return -1;
    else
        return 0;
}

// libarchive helper funcs, more or less verbatim from the examples/doc
int libarchive_copy_data(struct archive *ar, struct archive *aw)
{
    int r;
    const void *buff;
    size_t size;
    int64_t offset;

    for(;;)
    {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if(r == ARCHIVE_EOF)
            return ARCHIVE_OK;
        if(r != ARCHIVE_OK)
            return r;
        r = archive_write_data_block(aw, buff, size, offset);
        if(r != ARCHIVE_OK)
        {
            fprintf(stderr, "archive_write_data_block() failed: %s\n", archive_error_string(aw));
            return r;
        }
    }
}

int libarchive_extract(const char *filename, const char *prefix)
{
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;
    int r;
    const char *path = NULL;
    char *fixed_path = NULL;
    size_t len;
    int dirty_archive = 0;

    // Select which attributes we want to restore.
    flags = ARCHIVE_EXTRACT_TIME;
    // Don't preserve permissions, as most files in kindle packages will be owned by root, and if the perms are effed up, it gets annoying.
    // We could also just rewrite every entry in the archive with sane permissions, but that seems a bit overkill.
    //flags |= ARCHIVE_EXTRACT_PERM;
    //flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    a = archive_read_new();
    // Let's handle a wide range or tar formats, just to be on the safe side
    archive_read_support_format_tar(a);
    archive_read_support_format_gnutar(a);
    archive_read_support_filter_gzip(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if(filename != NULL && strcmp(filename, "-") == 0)
        filename = NULL;
    if((r = archive_read_open_file(a, filename, 10240)))
    {
        fprintf(stderr, "archive_read_open_file() failure: %s\n", archive_error_string(a));
        archive_read_free(a);
        goto cleanup;
    }
    dirty_archive = 1;

    for(;;)
    {
        r = archive_read_next_header(a, &entry);
        if(r == ARCHIVE_EOF)
            break;
        if(r != ARCHIVE_OK)
            fprintf(stderr, "archive_read_next_header() failed: %s\n", archive_error_string(a));
        if(r < ARCHIVE_WARN)
            goto cleanup;

        // Print what we're extracting, like bsdtar
        path = archive_entry_pathname(entry);
        fprintf(stderr, "x %s\n", path);
        // Rewrite the entry's pathname to extract in the right output directory
        len = strlen(prefix) + 1 + strlen(path) + 1;
        fixed_path = malloc(len);
        snprintf(fixed_path, len, "%s/%s", prefix, path);
        archive_entry_copy_pathname(entry, fixed_path);

        r = archive_write_header(ext, entry);
        if(r != ARCHIVE_OK)
            fprintf(stderr, "archive_write_header() failed: %s\n", archive_error_string(ext));
        else if(archive_entry_size(entry) > 0)
        {
            libarchive_copy_data(a, ext);
            if(r != ARCHIVE_OK)
                fprintf(stderr, "copy_data() failed: %s\n", archive_error_string(ext));
            if(r < ARCHIVE_WARN)
            {
                free(fixed_path);
                goto cleanup;
            }
        }

        r = archive_write_finish_entry(ext);
        if(r != ARCHIVE_OK)
            fprintf(stderr, "archive_write_finish_entry() failed: %s\n", archive_error_string(ext));
        if(r < ARCHIVE_WARN)
        {
            free(fixed_path);
            goto cleanup;
        }

        // Cleanup
        free(fixed_path);
    }
    archive_read_close(a);
    archive_read_free(a);
    //dirty_archive = 0;        // Make clang's sa happy
    archive_write_close(ext);
    archive_write_free(ext);

    return 0;

cleanup:
    if(dirty_archive)
    {
        archive_read_close(a);
        archive_read_free(a);
    }
    archive_write_close(ext);
    archive_write_free(ext);

    return 1;
}

int kindle_extract_main(int argc, char *argv[])
{
    char *bin_filename;
#if defined(_WIN32) && !defined(__CYGWIN__)
    // FIXME: This is crappy, because we need Administrator rights to write in /, but tmpfile probably (according to libarchive) does the same anyway...
    char tgz_filename[] = "/kindletool_extract_tgz_XXXXXX";
#else
    char tgz_filename[] = "/tmp/kindletool_extract_tgz_XXXXXX";
#endif
    char *output_dir;
    FILE *bin_input;
    int tgz_fd;
    FILE *tgz_output;

    // Skip command
    argv++;
    argc--;
    if(argc < 2)
    {
        fprintf(stderr, "Invalid number of arguments.\n");
        return -1;
    }
    bin_filename = argv[0];
    // Check that input properly ends in .bin
    if(!IS_BIN(bin_filename))
    {
        fprintf(stderr, "The input file must be a '.bin' update package.\n");
        return -1;
    }
    // NOTE: Do some sanity checks for output directory handling?
    // The 'rewrite pathname entry' cheap method we currently use is pretty 'dumb' (it assumes the path is correct, creating it if need be),
    // but the other (more correct?) way to handle this (chdir) would need some babysitting (cf. bsdtar's *_chdir() in tar/util.c)...
    output_dir = argv[1];
    if((bin_input = fopen(bin_filename, "rb")) == NULL)
    {
        fprintf(stderr, "Cannot open update input '%s'.\n", bin_filename);
        return -1;
    }
    // Use a non-racy tempfile, hopefully... (Heavily inspired from http://www.tldp.org/HOWTO/Secure-Programs-HOWTO/avoid-race.html)
    // We always create them in /tmp, and rely on the OS implementation to handle the umask,
    // it'll cost us less LOC that way since I don't really want to introduce a dedicated utility function for tempfile handling...
    // NOTE: Probably not as race-proof on MinGW, according to libarchive...
#if defined(_WIN32) && !defined(__CYGWIN__)
    // Inspired from libgit2's Posix emulation layer (https://github.com/libgit2/libgit2)
    if(_mktemp(tgz_filename) == NULL)
    {
        fprintf(stderr, "Couldn't create temporary file template.\n");
        fclose(bin_input);
        return -1;
    }
    tgz_fd = open(tgz_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0744);
#else
    tgz_fd = mkstemp(tgz_filename);
#endif
    if(tgz_fd == -1)
    {
        fprintf(stderr, "Couldn't open temporary file.\n");
        fclose(bin_input);
        return -1;
    }
    if((tgz_output = fdopen(tgz_fd, "wb")) == NULL)
    {
        fprintf(stderr, "Cannot open temp output '%s' for writing.\n", tgz_filename);
        fclose(bin_input);
        close(tgz_fd);
        unlink(tgz_filename);
        return -1;
    }
    // Print a recap of what we're about to do
    fprintf(stderr, "Extracting update package %s to %s via %s\n", bin_filename, output_dir, tgz_filename);
    if(kindle_convert(bin_input, tgz_output, NULL, 0) < 0)
    {
        fprintf(stderr, "Error converting update '%s'.\n", bin_filename);
        fclose(bin_input);
        fclose(tgz_output);
        return -1;
    }
    fclose(bin_input);
    fclose(tgz_output);
    if(libarchive_extract(tgz_filename, output_dir) < 0)
    {
        fprintf(stderr, "Error extracting temp tarball '%s' to '%s'.\n", tgz_filename, output_dir);
        unlink(tgz_filename);
        return -1;
    }
    unlink(tgz_filename);
    return 0;
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on;
