/*
**  KindleTool, create.c
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

#include "create.h"

static const char*
    convert_bundle_version(BundleVersion bundlev)
{
	switch (bundlev) {
		case UpdateSignature:
			return "Signature";
		case OTAUpdateV2:
			return "OTA V2";
		case OTAUpdate:
			return "OTA V1";
		case RecoveryUpdate:
			return "Recovery";
		case RecoveryUpdateV2:
			return "Recovery V2";
		case UnknownUpdate:
		default:
			return "Unknown";
	}
}

static struct rsa_private_key
    get_default_key(void)
{
	// Make nettle happy... (Array created from the bin2h (grub2 has one) output of pkcs1-conv on our pem file)
	static const uint8_t sign_key_sexp[] = {
		0x28, 0x31, 0x31, 0x3a, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x2d, 0x6b, 0x65, 0x79, 0x28, 0x39,
		0x3a, 0x72, 0x73, 0x61, 0x2d, 0x70, 0x6b, 0x63, 0x73, 0x31, 0x28, 0x31, 0x3a, 0x6e, 0x31, 0x32, 0x39,
		0x3a, 0x00, 0xc9, 0x9f, 0x58, 0xd6, 0x53, 0xec, 0x71, 0x56, 0xff, 0xde, 0x44, 0xa7, 0xc2, 0x3d, 0x1f,
		0x5e, 0xe3, 0xb9, 0x4f, 0x58, 0xdd, 0xab, 0x1f, 0x7d, 0xf3, 0xf5, 0x06, 0xdf, 0x9e, 0xa9, 0x82, 0xc4,
		0x14, 0x4b, 0x3f, 0xa9, 0x8c, 0x8c, 0x6c, 0xba, 0x00, 0xfc, 0xb2, 0x71, 0x05, 0xe0, 0xde, 0x73, 0xe2,
		0xe5, 0xf7, 0x1b, 0xef, 0x96, 0xa5, 0x66, 0x8f, 0x8e, 0x87, 0x4d, 0x76, 0x1e, 0x93, 0x1e, 0xf4, 0xb9,
		0xe9, 0x78, 0x48, 0x25, 0xa0, 0x87, 0x66, 0xd4, 0x4e, 0x0b, 0x3a, 0xcc, 0xab, 0xcf, 0x89, 0x2d, 0xb5,
		0x0b, 0x46, 0x46, 0x5c, 0xc2, 0x12, 0xb9, 0x81, 0x1a, 0xde, 0xbe, 0x70, 0x05, 0x44, 0x57, 0xce, 0xb2,
		0xda, 0x98, 0x4e, 0x27, 0x79, 0x8b, 0x93, 0x41, 0x24, 0xf5, 0x44, 0x17, 0x6c, 0x85, 0x1f, 0xae, 0xfc,
		0x89, 0x9d, 0x2d, 0x8c, 0x28, 0xb1, 0xb6, 0x71, 0xcc, 0xe3, 0x95, 0x29, 0x28, 0x31, 0x3a, 0x65, 0x33,
		0x3a, 0x01, 0x00, 0x01, 0x29, 0x28, 0x31, 0x3a, 0x64, 0x31, 0x32, 0x38, 0x3a, 0x48, 0xbc, 0xa6, 0xd4,
		0xf3, 0x83, 0xda, 0x43, 0xb3, 0x9d, 0x21, 0x11, 0x90, 0x5e, 0x72, 0xa1, 0xcd, 0xef, 0xbd, 0x73, 0x66,
		0xcc, 0xe4, 0x58, 0x91, 0x19, 0x35, 0x78, 0x99, 0x09, 0xb8, 0x36, 0x3a, 0xc8, 0x06, 0xd8, 0x88, 0xee,
		0xe4, 0x0e, 0x9a, 0x6a, 0x8f, 0x89, 0x7c, 0xc0, 0x6a, 0x20, 0x4e, 0x9b, 0xfd, 0xf0, 0xe3, 0x17, 0x6a,
		0xe6, 0x3c, 0x26, 0x04, 0x23, 0xea, 0xd8, 0x0e, 0xe4, 0xb9, 0x18, 0xda, 0xea, 0x6d, 0xb6, 0xe9, 0x03,
		0xaf, 0xcb, 0xa1, 0x13, 0x6c, 0xfd, 0x0e, 0x1e, 0xc7, 0x31, 0x95, 0x7f, 0xac, 0x36, 0x1a, 0xfb, 0xda,
		0xf2, 0x6c, 0x9b, 0xac, 0x46, 0x20, 0x10, 0x0e, 0x61, 0x7e, 0x54, 0x2c, 0xd8, 0xd8, 0x78, 0xab, 0x8e,
		0x9b, 0x12, 0xce, 0x04, 0x6e, 0xd2, 0xbf, 0x36, 0x34, 0x2f, 0x33, 0x9c, 0xd9, 0xb6, 0x78, 0x63, 0x91,
		0xca, 0xcf, 0x41, 0xbe, 0x61, 0x29, 0x28, 0x31, 0x3a, 0x70, 0x36, 0x35, 0x3a, 0x00, 0xe8, 0x22, 0x89,
		0x0e, 0xaf, 0x47, 0xd8, 0xcf, 0x75, 0x13, 0x49, 0xb1, 0xdf, 0x0f, 0x77, 0xa7, 0x81, 0x71, 0x4f, 0x67,
		0xe2, 0x5a, 0x26, 0xa5, 0x3c, 0xc5, 0xac, 0x91, 0xec, 0x2f, 0x86, 0xa7, 0x92, 0x34, 0x0a, 0x04, 0xa7,
		0x08, 0x34, 0xd0, 0x56, 0x07, 0x64, 0x54, 0x66, 0xcf, 0xb8, 0xb5, 0x58, 0x89, 0x60, 0xc8, 0x70, 0x46,
		0xb1, 0x8e, 0xf5, 0x6b, 0x85, 0x76, 0x2d, 0xd8, 0x07, 0x3d, 0x29, 0x28, 0x31, 0x3a, 0x71, 0x36, 0x35,
		0x3a, 0x00, 0xde, 0x59, 0xc4, 0x46, 0x08, 0x34, 0x46, 0x65, 0x81, 0x0b, 0x72, 0xbc, 0xb6, 0x80, 0xb2,
		0x7c, 0x3b, 0xeb, 0xf1, 0xe5, 0xda, 0xa3, 0xec, 0x60, 0x50, 0x9d, 0xe5, 0x35, 0x66, 0xea, 0x4b, 0x41,
		0xed, 0xc3, 0x17, 0x33, 0xc2, 0x72, 0x04, 0x1f, 0x8f, 0x48, 0x20, 0x3a, 0x23, 0x6d, 0x39, 0xcb, 0x52,
		0xbd, 0xce, 0x8a, 0xd1, 0x4c, 0x66, 0xe6, 0x89, 0xb9, 0x3d, 0x8c, 0xb5, 0x6c, 0xd3, 0x39, 0x29, 0x28,
		0x31, 0x3a, 0x61, 0x36, 0x35, 0x3a, 0x00, 0xae, 0x86, 0x08, 0x75, 0x39, 0xe2, 0xd2, 0x66, 0x66, 0xa6,
		0xf1, 0xa9, 0x01, 0x03, 0x27, 0xfa, 0x8f, 0x9f, 0x19, 0x0c, 0x09, 0x69, 0xad, 0xd4, 0x5d, 0x34, 0x60,
		0xe1, 0xf4, 0xa8, 0x66, 0x9c, 0x65, 0x97, 0x2a, 0x51, 0x05, 0x23, 0x6e, 0x51, 0x93, 0xdc, 0x4a, 0xda,
		0x09, 0xd1, 0xf2, 0x14, 0xa5, 0x53, 0xe3, 0xa7, 0xce, 0x81, 0xd7, 0xcc, 0x9b, 0x47, 0x13, 0x38, 0x1e,
		0x8f, 0x64, 0x21, 0x29, 0x28, 0x31, 0x3a, 0x62, 0x36, 0x35, 0x3a, 0x00, 0xc8, 0xb3, 0x96, 0x6a, 0xf0,
		0x74, 0xdf, 0x26, 0x38, 0x39, 0x31, 0x34, 0x0e, 0x38, 0x54, 0xe3, 0xb6, 0xe2, 0xde, 0xd2, 0x6f, 0x6c,
		0x8f, 0xac, 0xd0, 0x97, 0xf5, 0x91, 0x22, 0x78, 0x51, 0xbe, 0x0c, 0xf3, 0x90, 0x39, 0xf4, 0x46, 0x1e,
		0x5a, 0xae, 0x66, 0x98, 0x50, 0x62, 0x31, 0xf1, 0x7d, 0x0a, 0x0e, 0xb2, 0x24, 0xb3, 0x8f, 0x97, 0x42,
		0x79, 0x06, 0x6f, 0xfc, 0x56, 0xb7, 0x08, 0x61, 0x29, 0x28, 0x31, 0x3a, 0x63, 0x36, 0x35, 0x3a, 0x00,
		0xdc, 0x57, 0x67, 0xae, 0xc1, 0x62, 0x08, 0xd3, 0x49, 0x86, 0xf8, 0xad, 0xd9, 0xa4, 0xe6, 0xb4, 0xbc,
		0xd7, 0xc5, 0x4e, 0x3a, 0x2b, 0xeb, 0x15, 0xe8, 0xd2, 0x18, 0xd6, 0xd1, 0x09, 0x1b, 0xe4, 0x45, 0xcc,
		0xb4, 0x70, 0x3b, 0x82, 0x05, 0x0d, 0x8e, 0x1a, 0xfd, 0xda, 0x28, 0x87, 0x56, 0x21, 0xd6, 0x21, 0x45,
		0x1a, 0x37, 0x26, 0xa6, 0xac, 0xda, 0xea, 0xd4, 0x6e, 0xb5, 0xac, 0x3c, 0xcc, 0x29, 0x29, 0x29
	};

	struct rsa_private_key rsa_pkey;
	rsa_private_key_init(&rsa_pkey);

	if (!rsa_keypair_from_sexp(NULL, &rsa_pkey, 0, sizeof(sign_key_sexp), sign_key_sexp)) {
		fprintf(stderr, "Invalid default private key!\n");
		// In the unlikely event this ever happens, it'll be caught later on in sign_file ;).
	}

	return rsa_pkey;
}

static int
    sign_file(FILE* in_file, const struct rsa_private_key* rsa_pkey, FILE* sigout_file)
{
	unsigned char     buffer[BUFFER_SIZE];
	size_t            len;
	struct sha256_ctx hash;
	mpz_t             sig;
	// NOTE: Don't do this at home, kids! We can get away with it because we know we can't use keys > 2K anyway...
	unsigned char     raw_sig[CERTIFICATE_2K_SIZE];
	size_t            siglen;

	// Like we just said, handle 2K keys at most!
	if (rsa_pkey->size > CERTIFICATE_2K_SIZE) {
		fprintf(stderr, "RSA key is too large (2K at most)!\n");
		return -1;
	}

	sha256_init(&hash);
	while ((len = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, in_file)) > 0) {
		sha256_update(&hash, len, buffer);
	}
	if (ferror(in_file) != 0) {
		fprintf(stderr, "Error reading input file: %s.\n", strerror(errno));
		return -1;
	}
	mpz_init(sig);
	if (!rsa_sha256_sign(rsa_pkey, &hash, sig)) {
		fprintf(stderr, "RSA key is too small!\n");
		mpz_clear(sig);
		return -1;
	}

	// NOTE: mpz_out_raw outputs a format that doesn't quite fit our needs (it prepends 4 bytes of size info)...
	//       Do it ourselves with mpz_export! That's:
	//       Words of the proper amount of bytes for the host, most significant word & byte first (BE), full words.
	mpz_export(raw_sig, &siglen, 1, sizeof(unsigned char*), 1, 0, sig);
	mpz_clear(sig);
	// Check that the sig looks sane...
	if (siglen * sizeof(unsigned char*) != rsa_pkey->size) {
		fprintf(stderr, "Signature is too short (or too large?) for our key!\n");
		return -1;
	}

	// And finally, write our sig!
	if (fwrite(raw_sig, sizeof(unsigned char), rsa_pkey->size, sigout_file) < rsa_pkey->size) {
		fprintf(stderr, "Error writing signature file: %s.\n", strerror(errno));
		return -1;
	}

	return 0;
}

// As usual, largely based on libarchive's doc, examples, and source ;)
static int
    metadata_filter(struct archive* a, void* _data __attribute__((unused)), struct archive_entry* entry)
{
	struct archive* matching;
	int             r;

	// Don't exclude directories!
	if (archive_read_disk_can_descend(a)) {
		// It's a directory, don't even try to perform pattern matching, just walk it
		archive_read_disk_descend(a);
		return 1;
	} else {
		// Exclude *.sig files in a case insensitive way, to avoid duplicates
		matching = archive_match_new();
		if (archive_match_exclude_pattern(matching, "./*\\.[Ss][Ii][Gg]$") != ARCHIVE_OK) {
			fprintf(stderr, "archive_match_exclude_pattern() failed: %s.\n", archive_error_string(matching));
		}
		// Exclude *.dat too, to avoid ending up with multiple bundlefiles!
		// NOTE: If we wanted to be more lenient, we could exclude "./update*\\.[Dd][Aa][Tt]$" instead
		if (archive_match_exclude_pattern(matching, "./*\\.[Dd][Aa][Tt]$") != ARCHIVE_OK) {
			fprintf(stderr, "archive_match_exclude_pattern() failed: %s.\n", archive_error_string(matching));
		}
		// Exclude *nix hidden files, too?
		// NOTE: The ARCHIVE_READDISK_MAC_COPYFILE flag for read_disk is disabled by default,
		//       so we should already be creating 'sane' archives on OS X, without the crazy ._* acl/xattr files ;)
		//       On the other hand, if the user passed us a self-built tarball, we can't do anything about it.
		//       OS X users: export COPYFILE_DISABLE=1 is your friend!
		/*
		if(archive_match_exclude_pattern(matching, "./\\.*$") != ARCHIVE_OK) {
			fprintf(stderr, "archive_match_exclude_pattern() failed: %s.\n", archive_error_string(matching));
		}
		*/
#if defined(_WIN32) && !defined(__CYGWIN__)
		// NOTE: Exclude our own tempfiles, since we may create them in PWD, because otherwise,
		//       depending on what the user uses as input (i.e., * or .), we might inadvertently snarf them up.
		//       Right now, the only one susceptible of being part of our directory walking
		//       is our own tarball temporary file...
		if (archive_match_exclude_pattern(matching, "^kindletool_create_tarball_*") != ARCHIVE_OK) {
			fprintf(stderr, "archive_match_exclude_pattern() failed: %s.\n", archive_error_string(matching));
		}
#endif

		r = archive_match_path_excluded(matching, entry);
		if (r < 0) {
			fprintf(stderr, "archive_match_path_excluded() failed: %s.\n", archive_error_string(matching));
			archive_match_free(matching);
			return 0;
		}
		if (r) {
			// Skip original bundle/sig files to avoid duplicates
			fprintf(stderr, "! %s\n", archive_entry_pathname(entry));
			archive_match_free(matching);
			return 0;
		} else {
			// We're a nice, proper file, carry on ;)
			archive_match_free(matching);
			return 1;
		}
	}
}

// Write a single file (or directory or other filesystem object) to the archive [from libarchive's tar/write.c].
static int
    write_file(const struct kttar* kttar, struct archive* a, struct archive* in_a, struct archive_entry* entry)
{
	if (write_entry(kttar, a, in_a, entry) != 0) {
		return 1;
	}
	return 0;
}

// Write a single entry to the archive [from libarchive's tar/write.c].
static int
    write_entry(const struct kttar* kttar, struct archive* a, struct archive* in_a, struct archive_entry* entry)
{
	int e;

	e = archive_write_header(a, entry);
	if (e != ARCHIVE_OK) {
		fprintf(stderr, "archive_write_header() failed: %s.\n", archive_error_string(a));
	}

	if (e == ARCHIVE_FATAL) {
		return 1;
	}

	// If we opened a file earlier, write it out now.
	// Note that the format handler might have reset the size field to zero
	// to inform us that the archive body won't get stored.
	// In that case, just skip the write.
	if (e >= ARCHIVE_WARN && archive_entry_size(entry) > 0) {
		if (copy_file_data_block(kttar, a, in_a, entry) != 0) {
			return 1;
		}
	}
	return 0;
}

// Helper function to copy file to archive [from libarchive's tar/write.c].
static int
    copy_file_data_block(const struct kttar* kttar, struct archive* a, struct archive* in_a, struct archive_entry* entry)
{
	size_t         bytes_read;
	ssize_t        bytes_written;
	int64_t        offset, progress = 0;
	unsigned char* null_buff = NULL;
	const void*    buff;
	int            r;

	while ((r = archive_read_data_block(in_a, &buff, &bytes_read, &offset)) == ARCHIVE_OK) {
		if (offset > progress) {
			int64_t sparse = offset - progress;
			size_t  ns;

			if (null_buff == NULL) {
				null_buff = kttar->buff;
				memset(null_buff, 0, kttar->buff_size);
			}

			while (sparse > 0) {
				if (sparse > (int64_t) kttar->buff_size) {
					ns = kttar->buff_size;
				} else {
					ns = (size_t) sparse;
				}
				bytes_written = archive_write_data(a, null_buff, ns);
				if (bytes_written < 0) {
					// Write failed; this is bad
					fprintf(stderr, "archive_write_data() failed: %s.\n", archive_error_string(a));
					return -1;
				}
				if ((size_t) bytes_written < ns) {
					// Write was truncated; warn but continue.
					fprintf(stderr,
						"%s: Truncated write; file may have grown while being archived.\n",
						archive_entry_pathname(entry));
					return 0;
				}
				progress += bytes_written;
				sparse   -= bytes_written;
			}
		}

		bytes_written = archive_write_data(a, buff, bytes_read);
		if (bytes_written < 0) {
			// Write failed; this is bad
			fprintf(stderr, "archive_write_data() failed: %s.\n", archive_error_string(a));
			return -1;
		}
		if ((size_t) bytes_written < bytes_read) {
			// Write was truncated; warn but continue.
			fprintf(stderr,
				"%s: Truncated write; file may have grown while being archived.\n",
				archive_entry_pathname(entry));
			return 0;
		}
		progress += bytes_written;
	}
	if (r < ARCHIVE_WARN) {
		fprintf(stderr, "archive_read_data_block() failed: %s.\n", archive_error_string(a));
		return -1;
	}
	return 0;
}

// Helper function to populate & write entries from a read_disk_open loop, tailored to our needs.
// Helps avoid code duplication, since we're doing this in two passes.
static int
    create_from_archive_read_disk(struct kttar*      kttar,
				  struct archive*    a,
				  const char*        input_filename,
				  bool               first_pass,
				  const char*        signame,
				  const unsigned int real_blocksize)
{
	int   r;
	bool  is_exec       = false;
	bool  is_kernel     = false;
	char* original_path = NULL;
	char* tweaked_path  = NULL;

	struct archive*       disk;
	struct archive_entry* entry;

	disk  = archive_read_disk_new();
	entry = archive_entry_new();

	if (first_pass) {
		// Perform pattern matching in a metadata filter to apply our exclude list to reguar files.
		// NOTE: We're not using archive_read_disk_set_matching anymore
		//       because it does *pattern* matching too early to determine if we're a directory...
		archive_read_disk_set_metadata_filter_callback(disk, metadata_filter, NULL);
	}
	archive_read_disk_set_standard_lookup(disk);

	r = archive_read_disk_open(disk, input_filename);
	if (r != ARCHIVE_OK) {
		fprintf(stderr, "archive_read_disk_open() failed: %s.\n", archive_error_string(disk));
		archive_read_free(disk);
		archive_entry_free(entry);
		return 1;
	}

	for (;;) {
		archive_entry_clear(entry);
		r = archive_read_next_header2(disk, entry);
		if (r == ARCHIVE_EOF) {
			break;
		} else if (r != ARCHIVE_OK) {
			fprintf(stderr, "archive_read_next_header2() failed: %s", archive_error_string(disk));
			if (r == ARCHIVE_FATAL) {
				fprintf(stderr, " (FATAL).\n");
				goto cleanup;
			} else if (r < ARCHIVE_WARN) {
				fprintf(stderr, " (FAILED).\n");
				// NOTE: We don't want to end up with an incomplete archive, abort.
				goto cleanup;
			}
		}

		if (!first_pass) {
			// Fix the entry pathname, we used a tempfile...
			// (if we're in legacy mode, signame has already been set to the tweaked path ;))
			archive_entry_copy_pathname(entry, signame);
		} else {
			// Tweak the pathname if we were asked to behave like Yifan's KindleTool...
			if (kttar->tweak_pointer_index != 0) {
				// Handle the 'root' source directory itself..
				// NOTE: We check that strlen <= pointer_index
				//       because libarchive strips trailing path separators in the entry pathname,
				//       but we might have passed one on the CL,
				//       so pointer_index might be larger than strlen ;)
				if (archive_entry_filetype(entry) == AE_IFDIR &&
				    // Flawfinder: ignore
				    strlen(archive_entry_pathname(entry)) <= kttar->tweak_pointer_index) {
					// Print what we're stripping, ala GNU tar...
					fprintf(stderr,
						"kindletool: Removing leading '%s/' from member names.\n",
						archive_entry_pathname(entry));
					// Just skip it, we don't need a redundant and explicit root directory entry in our tarball...
					archive_read_disk_descend(disk);
					continue;
				} else {
					original_path = strdup(archive_entry_pathname(entry));
					// Try to handle a trailing path separator properly...
					// NOTE: This probably isn't very robust.
					//       Also, no need to handle MinGW,
					//       it already spectacularly fails to handle this case ^^
					if (original_path[kttar->tweak_pointer_index] == '/') {
						// We found a path separator, skip it, too
						tweaked_path = original_path + (kttar->tweak_pointer_index + 1);
					} else {
						tweaked_path = original_path + kttar->tweak_pointer_index;
					}
					archive_entry_copy_pathname(entry, tweaked_path);
				}
			}
		}

		// And then override a bunch of stuff (namely, uid/gid/chmod)
		archive_entry_set_uid(entry, 0);
		archive_entry_set_uname(entry, "root");
		archive_entry_set_gid(entry, 0);
		archive_entry_set_gname(entry, "root");

		if (first_pass) {
			// If we have a regular file, and it's a script, make it executable (probably overkill, but hey :))
			if (archive_entry_filetype(entry) == AE_IFREG &&
			    (IS_SCRIPT(archive_entry_pathname(entry)) || IS_SHELL(archive_entry_pathname(entry)))) {
				archive_entry_set_perm(entry, 0755);
				// It's a script, keep track of it
				is_exec           = true;
				kttar->has_script = is_exec;
				is_kernel         = false;
			}
			// If we have a regular file, and it's a kernel, and we're a recovery update, keep track of it
			else if (archive_entry_filetype(entry) == AE_IFREG && real_blocksize == RECOVERY_BLOCK_SIZE &&
				 IS_UIMAGE(archive_entry_pathname(entry))) {
				archive_entry_set_perm(entry, 0644);
				is_exec   = false;
				// It's a kernel, keep track of it
				is_kernel = true;
			}
			// If we have a directory, make it searchable...
			else if (archive_entry_filetype(entry) == AE_IFDIR) {
				archive_entry_set_perm(entry, 0755);
				is_exec   = false;
				is_kernel = false;
			} else {
				archive_entry_set_perm(entry, 0644);
				is_exec   = false;
				is_kernel = false;
			}

			// Non-regular files get archived with zero size.
			if (archive_entry_filetype(entry) != AE_IFREG) {
				archive_entry_set_size(entry, 0);
			}
		} else {
			archive_entry_set_perm(entry, 0644);
		}

		archive_read_disk_descend(disk);
		// Print what we're adding, ala bsdtar
		fprintf(stderr,
			"a %s%s\n",
			archive_entry_pathname(entry),
			(is_kernel ? "\t\t|<" : (is_exec ? "\t\t<-" : "")));

		// Write our entry to the archive, completely via libarchive,
		// to avoid having to open our entry file again, which would fail on non-POSIX systems...
		if (write_file(kttar, a, disk, entry) != 0) {
			goto cleanup;
		}

		if (first_pass) {
			// If we just added a regular file, hash it, sign it, add it to the index, and put the sig in our tarball
			if (archive_entry_filetype(entry) == AE_IFREG) {
				// But just build a filelist for now, and do it later,
				// I'm not up to refactoring sign_file & md5_sum to be useable during copy_file_data_block...
				// We can't just do it now with the current sign_file & md5_sum implementation
				// because we'd need to open() the input file (to sign & hash it),
				// while it's already open through libarchive's read_disk API.
				// That's apparently not possible on non POSIX systems.
				// (You get a very helpful 'Permission denied' error on Windows...)
				kttar->to_sign_and_bundle_list         = realloc(kttar->to_sign_and_bundle_list,
                                                                         ++kttar->sign_and_bundle_index * sizeof(char*));
				// And do the same with our tweaked pathname for legacy mode...
				kttar->tweaked_to_sign_and_bundle_list = realloc(
				    kttar->tweaked_to_sign_and_bundle_list, kttar->sign_and_bundle_index * sizeof(char*));
				// Use the correct paths if we tweaked the entry pathname...
				if (kttar->tweak_pointer_index != 0) {
					kttar->to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] =
					    strdup(original_path);
					kttar->tweaked_to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] =
					    strdup(tweaked_path);
				} else {
					kttar->to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] =
					    strdup(archive_entry_pathname(entry));
					kttar->tweaked_to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] =
					    strdup(archive_entry_pathname(entry));
				}
			}
		} else {
			// Delete the file once we're done, be it a signature or the bundlefile
			unlink(input_filename);
		}
		free(original_path);
		tweaked_path = NULL;
	}

	archive_read_close(disk);
	archive_read_free(disk);
	archive_entry_free(entry);

	return 0;

cleanup:
	free(original_path);
	tweaked_path = NULL;
	archive_read_close(disk);
	archive_read_free(disk);
	archive_entry_free(entry);

	return 1;
}

// Archiving code inspired from libarchive tar/write.c ;).
static int
    kindle_create_package_archive(const int                     outfd,
				  char**                        filename,
				  const unsigned int            total_files,
				  const struct rsa_private_key* rsa_pkey_file,
				  const unsigned int            legacy,
				  const unsigned int            real_blocksize)
{
	struct archive* a;
	struct kttar *  kttar, kttar_storage;
	unsigned int    i;
	FILE*           file;
	FILE*           sigfile;
	char            md5[MD5_HASH_LENGTH + 1];
	uint8_t         bundlefile_status = 0;
	size_t          pathlen;
	char*           signame = NULL;
	char            sigabsolutepath[PATH_MAX];
	snprintf(sigabsolutepath, PATH_MAX, "%s/%s", kt_tempdir, "kindletool_create_sig_XXXXXX");
	int   sigfd;
	char* pathnamecpy = NULL;
	char  bundle_filename[PATH_MAX];
	snprintf(bundle_filename, PATH_MAX, "%s/%s", kt_tempdir, "kindletool_create_idx_XXXXXX");
	int         bundle_fd  = -1;
	FILE*       bundlefile = NULL;
	struct stat st;

	// Use a pointer for consistency, but stack-allocated storage for ease of cleanup.
	kttar = &kttar_storage;
	memset(kttar, 0, sizeof(*kttar));
	// Choose a suitable copy buffer size
	kttar->buff_size = 64 * 1024;
	while (kttar->buff_size < (size_t) DEFAULT_BYTES_PER_BLOCK) {
		kttar->buff_size *= 2;
	}
	// Try to compensate for space we'll lose to alignment.
	kttar->buff_size += 16 * 1024;

	// Allocate a buffer for file data.
	if ((kttar->buff = malloc(kttar->buff_size)) == NULL) {
		fprintf(stderr, "Cannot allocate memory for archive copy buffer.\n");
		return 1;
	}

	a = archive_write_new();
	archive_write_add_filter_gzip(a);
	archive_write_set_format_gnutar(a);

	// These should be the default (cf. archive_write_new @ libarchive/archive_write.c), but reset them to be on the safe side...
	archive_write_set_bytes_per_block(a, DEFAULT_BYTES_PER_BLOCK);
	archive_write_set_bytes_in_last_block(a, -1);

	archive_write_open_fd(a, outfd);

	// Loop over our input files/directories...
	for (i = 0; i < total_files; i++) {
		// Don't tweak entries pathname by default
		kttar->tweak_pointer_index = 0;
		// Check if we want to behave like Yifan's KindleTool
		if (legacy) {
			if (stat(filename[i], &st) == 0) {
				if (S_ISDIR(st.st_mode)) {
					kttar->tweak_pointer_index = strlen(filename[i]);    // Flawfinder: ignore
				}
			}
		}

		// Populate & write our entries from read_disk_open's directory walking...
		if (create_from_archive_read_disk(kttar, a, filename[i], true, NULL, real_blocksize) != 0) {
			goto cleanup;
		}
	}

	// Add our bundle index to the end of the list...
	// And we'll be creating it in a tempfile, to add to the fun...
	bundle_fd = mkstemp(bundle_filename);
	if (bundle_fd == -1) {
		fprintf(stderr, "Couldn't open temporary file: %s.\n", strerror(errno));
		goto cleanup;
	}
	if ((bundlefile = fdopen(bundle_fd, "w+b")) == NULL) {
		fprintf(stderr, "Cannot open temp bundlefile '%s' for writing: %s.\n", bundle_filename, strerror(errno));
		close(bundle_fd);
		unlink(bundle_filename);
		goto cleanup;
	}
	// Now that it's there, mark it as open and created
	bundlefile_status = BUNDLE_OPEN | BUNDLE_CREATED;
	// And append it as the last file...
	kttar->to_sign_and_bundle_list =
	    realloc(kttar->to_sign_and_bundle_list, ++kttar->sign_and_bundle_index * sizeof(char*));
	kttar->to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] = strdup(bundle_filename);
	// We'll never tweak the bundlefile pathname, but we rely on this being sane & consistent, so set it
	kttar->tweaked_to_sign_and_bundle_list =
	    realloc(kttar->tweaked_to_sign_and_bundle_list, kttar->sign_and_bundle_index * sizeof(char*));
	kttar->tweaked_to_sign_and_bundle_list[kttar->sign_and_bundle_index - 1] = strdup(bundle_filename);

	// And now loop again over the stuff we need to sign, hash & bundle...
	for (i = 0; i <= kttar->sign_and_bundle_index; i++) {
		// Dirty hack ahoy. If we're the last file in our list, that means we're the bundlefile, close our fd
		if (i == kttar->sign_and_bundle_index - 1) {
			fclose(bundlefile);
			// It's closed, remove our flag
			bundlefile_status &= (uint8_t) ~BUNDLE_OPEN;
		}

		// Dirty hack, the return. We loop twice on the bundlefile, once to sign it, and once to archive it...
		if (i == kttar->sign_and_bundle_index) {
			// It's the second time we're looping over the bundlefile, just archive it
			// Just set the correct pathnames...
			signame = strdup(INDEX_FILE_NAME);
			// NOTE: Fragile, sigabsolutepath & bundle_filename need to be of the same length...
			//       Slightly less of a concern now that both are using PATH_MAX, but, still...
			strcpy(sigabsolutepath, bundle_filename);
		} else {
			// First things first, if we're not the bundlefile,
			// we're gonna need our size for a field of the bundlefile, so get on that...
			if ((bundlefile_status & BUNDLE_OPEN) == BUNDLE_OPEN) {
				// We're out of a libarchive read loop, so do a stat call ourselves
				stat(kttar->to_sign_and_bundle_list[i], &st);
			}

			// Go on as usual, hash, sign & bundle :)
			if ((file = fopen(kttar->to_sign_and_bundle_list[i], "rb")) == NULL) {
				fprintf(stderr,
					"Cannot open '%s' for reading: %s!\n",
					kttar->to_sign_and_bundle_list[i],
					strerror(errno));
				// Avoid a double free (beginning from the second iteration,
				// since we freed signame at the end of the first iteration,
				// but it's not allocated yet, and cleanup will try to free...)
				signame = NULL;
				goto cleanup;
			}
			// Don't hash our bundlefile
			if ((bundlefile_status & BUNDLE_OPEN) == BUNDLE_OPEN) {
				if (md5_sum(file, md5) != 0) {
					fprintf(stderr,
						"Cannot calculate hash sum for '%s'.\n",
						kttar->to_sign_and_bundle_list[i]);
					fclose(file);
					// Avoid a double free, bis.
					signame = NULL;
					goto cleanup;
				}
				md5[MD5_HASH_LENGTH] = 0;
				rewind(file);
			}

			// If we're the bundlefile, fix the relative path to not use the tempfile path...
			if ((bundlefile_status & BUNDLE_OPEN) != BUNDLE_OPEN) {
				pathlen = strlen(INDEX_FILE_NAME);    // Flawfinder: ignore
				signame = malloc(pathlen + 4 + 1);
				snprintf(signame, pathlen + 4 + 1, "%s.%s", INDEX_FILE_NAME, "sig");
			} else {
				// Always use the tweaked paths
				// (they're properly set to the real path when we're not in legacy mode)
				pathlen = strlen(kttar->tweaked_to_sign_and_bundle_list[i]);    // Flawfinder: ignore
				signame = malloc(pathlen + 4 + 1);
				snprintf(
				    signame, pathlen + 4 + 1, "%s.%s", kttar->tweaked_to_sign_and_bundle_list[i], "sig");
			}
			// Create our sigfile in a tempfile
			// We have to make sure mkstemp's template is reset first...
			snprintf(sigabsolutepath, PATH_MAX, "%s/%s", kt_tempdir, "kindletool_create_sig_XXXXXX");
			sigfd = mkstemp(sigabsolutepath);
			if (sigfd == -1) {
				fprintf(stderr, "Couldn't open temporary signature file: %s.\n", strerror(errno));
				fclose(file);
				goto cleanup;
			}
			if ((sigfile = fdopen(sigfd, "wb")) == NULL) {
				fprintf(stderr,
					"Cannot open temp signature file '%s' for writing: %s.\n",
					signame,
					strerror(errno));
				fclose(file);
				close(sigfd);
				unlink(sigabsolutepath);
				goto cleanup;
			}
			if (sign_file(file, rsa_pkey_file, sigfile) < 0) {
				fprintf(stderr, "Cannot sign '%s'.\n", kttar->to_sign_and_bundle_list[i]);
				fclose(file);
				fclose(sigfile);
				unlink(sigabsolutepath);    // Delete empty/broken sigfile
				goto cleanup;
			}

			// Don't add the bundlefile to itself
			if ((bundlefile_status & BUNDLE_OPEN) == BUNDLE_OPEN) {
				// The last field is a display name, take a hint from the Python tool,
				// and use the file's basename with a simple suffix.
				// Use a copy of to_sign_and_bundle_list[i] to get our basename,
				// since the POSIX implementation may alter its arg, and that would be very bad...
				// And we're using the tweaked pathname in case we're in legacy mode ;)
				pathnamecpy = strdup(kttar->to_sign_and_bundle_list[i]);
				// Only flag kernels in recovery update...
				// FWIW, the format is as follows:
				//   file_type_id md5sum file_name blocksize file_display_name
				// where the id is 1 for kernel images (in recovery updates only),
				// 129 for install scripts, and 128 for assets,
				// and the blocksize is based on the file size relative to the update type blocksize.
				if (fprintf(bundlefile,
					    "%u %s %s %jd %s_ktool_file\n",
					    ((real_blocksize == RECOVERY_BLOCK_SIZE &&
						      IS_UIMAGE(kttar->to_sign_and_bundle_list[i])
						  ? 1U
					      : (IS_SCRIPT(kttar->to_sign_and_bundle_list[i]) ||
						 IS_SHELL(kttar->to_sign_and_bundle_list[i]))
						  ? 129U
						  : 128U)),
					    md5,
					    kttar->tweaked_to_sign_and_bundle_list[i],
					    (intmax_t) st.st_size / real_blocksize,
					    basename(pathnamecpy)) < 0) {
					fprintf(stderr, "Cannot write to bundle index file.\n");
					// Cleanup a bit before crapping out
					fclose(file);
					fclose(sigfile);
					unlink(sigabsolutepath);
					free(pathnamecpy);
					goto cleanup;
				}
				free(pathnamecpy);
			}

			// Cleanup
			fclose(file);
			fclose(sigfile);
		}

		// And now, for the fun part! Append our sigfile to the archive...
		// Populate & write our entries...
		if (create_from_archive_read_disk(kttar, a, sigabsolutepath, false, signame, real_blocksize) != 0) {
			unlink(sigabsolutepath);
			goto cleanup;
		}

		// Cleanup
		free(signame);
	}

	free(kttar->buff);
	for (i = 0; i < kttar->sign_and_bundle_index; i++) {
		free(kttar->to_sign_and_bundle_list[i]);
	}
	free(kttar->to_sign_and_bundle_list);
	for (i = 0; i < kttar->sign_and_bundle_index; i++) {
		free(kttar->tweaked_to_sign_and_bundle_list[i]);
	}
	free(kttar->tweaked_to_sign_and_bundle_list);
	archive_write_close(a);
	archive_write_free(a);

	// Print a warning if no scripts were detected (in an OTA update)...
	if (!kttar->has_script && real_blocksize == BLOCK_SIZE) {
		fprintf(stderr, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		fprintf(stderr, "@ No script was detected in your input, this update package won't do a thing! @\n");
		fprintf(stderr, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	}
	// If we're building a recovery update, warn that this possibly isn't the brightest idea, given the very specific requirements...
	if (real_blocksize == RECOVERY_BLOCK_SIZE) {
		fprintf(stderr,
			"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		fprintf(stderr,
			"@ You're building a recovery update from scratch! Make sure you know what you're doing... @\n");
		fprintf(stderr,
			"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	}

	return 0;

cleanup:
	// Close & remove the bundlefile if we crapped out in the middle of processing
	if ((bundlefile_status & BUNDLE_OPEN) == BUNDLE_OPEN) {
		fclose(bundlefile);
		unlink(bundle_filename);
	} else {
		// And if it's not open anymore, but was created, remove it too
		// (that's a short window of time, namely, when we're looping over the bundlefile itself & its sigfile)
		if ((bundlefile_status & BUNDLE_CREATED) == BUNDLE_CREATED) {
			unlink(bundle_filename);
		}
	}
	// Free what we might have alloc'ed
	free(signame);
	// The big stuff, too...
	free(kttar->buff);
	if (kttar->sign_and_bundle_index > 0) {
		for (i = 0; i < kttar->sign_and_bundle_index; i++) {
			free(kttar->to_sign_and_bundle_list[i]);
		}
		free(kttar->to_sign_and_bundle_list);
		for (i = 0; i < kttar->sign_and_bundle_index; i++) {
			free(kttar->tweaked_to_sign_and_bundle_list[i]);
		}
		free(kttar->tweaked_to_sign_and_bundle_list);
	}
	archive_write_close(a);
	archive_write_free(a);
	return 1;
}

static int
    kindle_create(const UpdateInformation* info, FILE* input_tgz, FILE* output, const bool fake_sign)
{
	unsigned char buffer[BUFFER_SIZE];
	size_t        count;
	FILE*         temp;

	switch (info->version) {
		case OTAUpdateV2:
			if ((temp = tmpfile()) == NULL) {
				fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
				return -1;
			}
			// Create the update
			if (kindle_create_ota_update_v2(info, input_tgz, temp, fake_sign) < 0) {
				fprintf(stderr, "Error creating update package.\n");
				fclose(temp);
				return -1;
			}
			rewind(temp);    // Rewind the file before reading back
			if (!fake_sign) {
				// Write the signature (unless we asked for an unsigned package)
				if (kindle_create_signature(info, temp, output) < 0) {
					fprintf(stderr, "Error signing update package.\n");
					fclose(temp);
					return -1;
				}
				rewind(temp);    // Rewind the file before writing it to output
			}
			// Write the update
			while ((count = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, temp)) > 0) {
				if (fwrite(buffer, sizeof(unsigned char), count, output) < count) {
					fprintf(stderr, "Error writing update to output: %s.\n", strerror(errno));
					fclose(temp);
					return -1;
				}
			}
			if (ferror(temp) != 0) {
				fprintf(stderr, "Error reading generated update: %s.\n", strerror(errno));
				fclose(temp);
				return -1;
			}
			fclose(temp);
			return 0;
			break;
		case OTAUpdate:
			return kindle_create_ota_update(info, input_tgz, output, fake_sign);
			break;
		case RecoveryUpdate:
			// NOTE: I was assuming that this, even FB02 @ rev. 2, shouldn't be wrapped in an UpdateSignature...
			//       That happened to be the case until Rex, where the target ota field also appeared...
			//       So wrap FB02h2 by default, you can always unwrap it later if need be...
			if (memcmp(info->magic_number, "FB02", MAGIC_NUMBER_LENGTH) == 0 && info->header_rev == 2) {
				if ((temp = tmpfile()) == NULL) {
					fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
					return -1;
				}
				if (kindle_create_recovery(info, input_tgz, temp, fake_sign) < 0) {
					fprintf(stderr, "Error creating update package.\n");
					fclose(temp);
					return -1;
				}
				rewind(temp);
				if (!fake_sign) {
					if (kindle_create_signature(info, temp, output) < 0) {
						fprintf(stderr, "Error signing update package.\n");
						fclose(temp);
						return -1;
					}
					rewind(temp);
				}
				while ((count = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, temp)) > 0) {
					if (fwrite(buffer, sizeof(unsigned char), count, output) < count) {
						fprintf(stderr, "Error writing update to output: %s.\n", strerror(errno));
						fclose(temp);
						return -1;
					}
				}
				if (ferror(temp) != 0) {
					fprintf(stderr, "Error reading generated update: %s.\n", strerror(errno));
					fclose(temp);
					return -1;
				}
				fclose(temp);
				return 0;
			} else {
				return kindle_create_recovery(info, input_tgz, output, fake_sign);
			}
			break;
		case RecoveryUpdateV2:
			if ((temp = tmpfile()) == NULL) {
				fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
				return -1;
			}
			if (kindle_create_recovery_v2(info, input_tgz, temp, fake_sign) < 0) {
				fprintf(stderr, "Error creating update package.\n");
				fclose(temp);
				return -1;
			}
			rewind(temp);
			if (!fake_sign) {
				if (kindle_create_signature(info, temp, output) < 0) {
					fprintf(stderr, "Error signing update package.\n");
					fclose(temp);
					return -1;
				}
				rewind(temp);
			}
			while ((count = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, temp)) > 0) {
				if (fwrite(buffer, sizeof(unsigned char), count, output) < count) {
					fprintf(stderr, "Error writing update to output: %s.\n", strerror(errno));
					fclose(temp);
					return -1;
				}
			}
			if (ferror(temp) != 0) {
				fprintf(stderr, "Error reading generated update: %s.\n", strerror(errno));
				fclose(temp);
				return -1;
			}
			fclose(temp);
			return 0;
			break;
		case UpdateSignature:
			// NOTE: Should only be reached when building a signed userdata package
			// We only need to sign the input tarball...
			if (kindle_create_signature(info, input_tgz, output) < 0) {
				fprintf(stderr, "Error signing userdata package.\n");
				return -1;
			}
			rewind(input_tgz);
			// ...And then simply append the input tarball as-is
			while ((count = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, input_tgz)) > 0) {
				if (fwrite(buffer, sizeof(unsigned char), count, output) < count) {
					fprintf(
					    stderr, "Error appending userdata tarball to output: %s.\n", strerror(errno));
					return -1;
				}
			}
			if (ferror(input_tgz) != 0) {
				fprintf(stderr, "Error reading original userdata tarball update: %s.\n", strerror(errno));
				return -1;
			}
			return 0;
			break;
		case UnknownUpdate:
		default:
			fprintf(stderr, "Unknown update type.\n");
			break;
	}
	return -1;
}

static int
    kindle_create_ota_update_v2(const UpdateInformation* info, FILE* input_tgz, FILE* output, const bool fake_sign)
{
	size_t         header_size;
	unsigned char* header;
	size_t         hindex = 0;
	int            i;
	FILE*          demunged_tgz;
	size_t         str_len;

	demunged_tgz = NULL;

	// First part of the set sized data
	header_size = MAGIC_NUMBER_LENGTH + OTA_UPDATE_V2_BLOCK_SIZE;
	header      = malloc(header_size);
	strncpy((char*) header, info->magic_number, MAGIC_NUMBER_LENGTH);    // Flawfinder: ignore
	hindex += MAGIC_NUMBER_LENGTH;
	memcpy(&header[hindex], &info->source_revision, sizeof(uint64_t));    // Source
	hindex += sizeof(uint64_t);
	memcpy(&header[hindex], &info->target_revision, sizeof(uint64_t));    // Target
	hindex += sizeof(uint64_t);
	memcpy(&header[hindex], &info->num_devices, sizeof(uint16_t));    // Device count
	hindex += sizeof(uint16_t);

	// Next, we write the devices
	header_size += info->num_devices * sizeof(uint16_t);
	header       = realloc(header, header_size);
	for (i = 0; i < info->num_devices; i++) {
		memcpy(&header[hindex], &info->devices[i], sizeof(uint16_t));    // Device
		hindex += sizeof(uint16_t);
	}

	// Part two of the set sized data
	header_size += OTA_UPDATE_V2_PART_2_BLOCK_SIZE;
	header       = realloc(header, header_size);
	memcpy(&header[hindex], &info->critical, sizeof(uint8_t));    // Critical
	hindex += sizeof(uint8_t);
	memset(&header[hindex], 0, sizeof(uint8_t));    // 1 byte padding
	hindex += sizeof(uint8_t);

	// Even if we asked for a fake package, the Kindle still expects a proper package...
	// md5 hash a temp deobfuscated tarball to fake it ;)
	if (fake_sign) {
		if ((demunged_tgz = tmpfile()) == NULL) {
			fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
			free(header);
			return -1;
		}
		demunger(input_tgz, demunged_tgz, 0, false);
		rewind(input_tgz);
		rewind(demunged_tgz);
		if (md5_sum(demunged_tgz, (char*) &header[hindex]) < 0) {
			fprintf(stderr, "Error calculating MD5 of fake package.\n");
			free(header);
			return -1;
		}
		fclose(demunged_tgz);
	} else {
		if (md5_sum(input_tgz, (char*) &header[hindex]) < 0)    // md5 hash
		{
			fprintf(stderr, "Error calculating MD5 of package.\n");
			free(header);
			return -1;
		}
		rewind(input_tgz);    // Reset input for later reading
	}

	md(&header[hindex], MD5_HASH_LENGTH);    // Obfuscate md5 hash
	hindex += MD5_HASH_LENGTH;
	memcpy(&header[hindex], &info->num_meta, sizeof(uint16_t));    // num_meta, cannot be cast
	hindex += sizeof(uint16_t);

	// Next, we write the meta strings
	for (i = 0; i < info->num_meta; i++) {
		str_len      = strlen(info->metastrings[i]);    // Flawfinder: ignore
		header_size += str_len + sizeof(uint16_t);
		header       = realloc(header, header_size);
		// String length: little endian -> big endian
		// FIXME: While otaup expects this endianness switch, it would seem that otacheck doesn't,
		//        and chokes with an headerTooShortInMetadataField error as soon as we pass more than one metastring...
		//        If we don't switch the endianness, otacheck passes, but otaup chokes... >_<"
		memcpy(&header[hindex], &((uint8_t*) &str_len)[1], sizeof(uint8_t));
		hindex += sizeof(uint8_t);
		memcpy(&header[hindex], &((uint8_t*) &str_len)[0], sizeof(uint8_t));
		hindex += sizeof(uint8_t);
		// Obfuscate meta string
		md((unsigned char*) info->metastrings[i], str_len);
		// FIXME: Should this really be munged? Following otaup would point to yes,
		//        but I've never seen an update with meta strings in the wild,
		//        and the aforementionned issue with the string length doesn't help...
		strncpy((char*) &header[hindex], info->metastrings[i], str_len);    // Flawfinder: ignore
		hindex += str_len;
	}

	// Now, we write the header to the file
	if (fwrite(header, sizeof(unsigned char), header_size, output) < header_size) {
		fprintf(stderr, "Error writing update header: %s.\n", strerror(errno));
		free(header);
		return -1;
	}

	// Write the actual update
	free(header);
	return munger(input_tgz, output, 0, fake_sign);
}

static int
    kindle_create_signature(const UpdateInformation* info, FILE* input_bin, FILE* output)
{
	UpdateHeader header;    // Header to write

	memset(&header, 0, sizeof(UpdateHeader));                                          // Zero init
	memcpy(header.magic_number, "SP01", MAGIC_NUMBER_LENGTH);                          // Write magic number
	header.data.signature.certificate_number = (uint32_t) info->certificate_number;    // 4 byte certificate number
	if (fwrite(&header, sizeof(unsigned char), MAGIC_NUMBER_LENGTH + UPDATE_SIGNATURE_BLOCK_SIZE, output) <
	    MAGIC_NUMBER_LENGTH + UPDATE_SIGNATURE_BLOCK_SIZE) {
		fprintf(stderr, "Error writing update header: %s.\n", strerror(errno));
		return -1;
	}
	// Write signature to output
	if (sign_file(input_bin, &info->sign_pkey, output) < 0) {
		fprintf(stderr, "Error signing update package payload.\n");
		return -1;
	}
	return 0;
}

static int
    kindle_create_ota_update(const UpdateInformation* info, FILE* input_tgz, FILE* output, const bool fake_sign)
{
	UpdateHeader header;
	FILE*        obfuscated_tgz;

	obfuscated_tgz = NULL;

	memset(&header, 0, sizeof(UpdateHeader));    // Zero init
	// Flawfinder: ignore
	strncpy(header.magic_number, info->magic_number, MAGIC_NUMBER_LENGTH);        // Magic number
	header.data.ota_update.source_revision = (uint32_t) info->source_revision;    // Source
	header.data.ota_update.target_revision = (uint32_t) info->target_revision;    // Target
	header.data.ota_update.device          = (uint16_t) info->devices[0];         // Device
	header.data.ota_update.optional        = (unsigned char) info->optional;      // Optional

	if (fake_sign) {
		if ((obfuscated_tgz = tmpfile()) == NULL) {
			fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
			return -1;
		}
		demunger(input_tgz, obfuscated_tgz, 0, false);
		rewind(input_tgz);
		rewind(obfuscated_tgz);
		if (md5_sum(obfuscated_tgz, header.data.ota_update.md5_sum) < 0) {
			fprintf(stderr, "Error calculating MD5 of package.\n");
			return -1;
		}
		fclose(obfuscated_tgz);
	} else {
		if (md5_sum(input_tgz, header.data.ota_update.md5_sum) < 0) {
			fprintf(stderr, "Error calculating MD5 of input tgz.\n");
			return -1;
		}
		rewind(input_tgz);    // Rewind input
	}
	md((unsigned char*) header.data.ota_update.md5_sum, MD5_HASH_LENGTH);    // Obfuscate md5 hash

	// Write header to output
	if (fwrite(&header, sizeof(unsigned char), MAGIC_NUMBER_LENGTH + OTA_UPDATE_BLOCK_SIZE, output) <
	    MAGIC_NUMBER_LENGTH + OTA_UPDATE_BLOCK_SIZE) {
		fprintf(stderr, "Error writing update header: %s.\n", strerror(errno));
		return -1;
	}

	// Write package to output
	return munger(input_tgz, output, 0, fake_sign);
}

static int
    kindle_create_recovery(const UpdateInformation* info, FILE* input_tgz, FILE* output, const bool fake_sign)
{
	UpdateHeader header;
	FILE*        obfuscated_tgz;

	obfuscated_tgz = NULL;

	memset(&header, 0, sizeof(UpdateHeader));    // Zero init

	// Flawfinder: ignore
	strncpy(header.magic_number, info->magic_number, MAGIC_NUMBER_LENGTH);    // Magic number
	header.data.recovery_update.magic_1 = (uint32_t) info->magic_1;           // Magic 1
	header.data.recovery_update.magic_2 = (uint32_t) info->magic_2;           // Magic 2
	header.data.recovery_update.minor   = (uint32_t) info->minor;             // Minor

	// Handle FB02 with a V2 Header Rev. Different length, but still fixed...
	if (info->header_rev == 2) {
		// NOTE: It expects some new stuff that I'm not too sure about... Here be dragons.
		// NOTE: Target OTA is optional, appeared (and is mandatory there) on Rex
		//       (where those are also wrapped in a signature envelope!)
		header.data.recovery_h2_update.target_revision = (uint64_t) info->target_revision;
		header.data.recovery_h2_update.platform        = (uint32_t) info->platform;
		header.data.recovery_h2_update.header_rev      = (uint32_t) info->header_rev;
		header.data.recovery_h2_update.board           = (uint32_t) info->board;
	} else {
		// Assume what we did before was okay, and put a device id in there...
		header.data.recovery_update.device = (uint32_t) info->devices[0];    // Device
	}

	if (fake_sign) {
		if ((obfuscated_tgz = tmpfile()) == NULL) {
			fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
			return -1;
		}
		demunger(input_tgz, obfuscated_tgz, 0, false);
		rewind(input_tgz);
		rewind(obfuscated_tgz);
		if (md5_sum(obfuscated_tgz, header.data.recovery_update.md5_sum) < 0) {
			fprintf(stderr, "Error calculating MD5 of package.\n");
			return -1;
		}
		fclose(obfuscated_tgz);
	} else {
		if (md5_sum(input_tgz, header.data.recovery_update.md5_sum) < 0) {
			fprintf(stderr, "Error calculating MD5 of input tgz.\n");
			return -1;
		}
		rewind(input_tgz);    // Rewind input
	}
	md((unsigned char*) header.data.recovery_update.md5_sum, MD5_HASH_LENGTH);    // Obfuscate md5 hash

	// Write header to output
	if (fwrite(&header, sizeof(unsigned char), MAGIC_NUMBER_LENGTH + RECOVERY_UPDATE_BLOCK_SIZE, output) <
	    MAGIC_NUMBER_LENGTH + RECOVERY_UPDATE_BLOCK_SIZE) {
		fprintf(stderr, "Error writing update header: %s.\n", strerror(errno));
		return -1;
	}

	// Write package to output
	return munger(input_tgz, output, 0, fake_sign);
}

static int
    kindle_create_recovery_v2(const UpdateInformation* info, FILE* input_tgz, FILE* output, const bool fake_sign)
{
	size_t         header_size;
	unsigned char* header;
	size_t         hindex = 0;
	int            i;
	FILE*          demunged_tgz;
	unsigned char  recovery_num_devices;

	demunged_tgz = NULL;

	// Its total size is fixed, but some stuff inside is variable/padded...
	header_size = MAGIC_NUMBER_LENGTH + RECOVERY_UPDATE_BLOCK_SIZE;
	header      = malloc(header_size);
	// Zero init everything first...
	memset(header, 0, header_size);

	// Flawfinder: ignore
	strncpy((char*) header, info->magic_number, MAGIC_NUMBER_LENGTH);
	hindex += MAGIC_NUMBER_LENGTH;
	hindex += sizeof(uint32_t);                                           // Padding
	memcpy(&header[hindex], &info->target_revision, sizeof(uint64_t));    // Target
	hindex += sizeof(uint64_t);

	// Even if we asked for a fake package, the Kindle still expects a proper package...
	// md5 hash a temp deobfuscated tarball to fake it ;)
	if (fake_sign) {
		if ((demunged_tgz = tmpfile()) == NULL) {
			fprintf(stderr, "Error opening temp file: %s.\n", strerror(errno));
			free(header);
			return -1;
		}
		demunger(input_tgz, demunged_tgz, 0, false);
		rewind(input_tgz);
		rewind(demunged_tgz);
		if (md5_sum(demunged_tgz, (char*) &header[hindex]) < 0) {
			fprintf(stderr, "Error calculating MD5 of fake package.\n");
			free(header);
			return -1;
		}
		fclose(demunged_tgz);
	} else {
		if (md5_sum(input_tgz, (char*) &header[hindex]) < 0)    // md5 hash
		{
			fprintf(stderr, "Error calculating MD5 of package.\n");
			free(header);
			return -1;
		}
		rewind(input_tgz);    // Reset input for later reading
	}

	md(&header[hindex], MD5_HASH_LENGTH);    // Obfuscate md5 hash
	hindex += MD5_HASH_LENGTH;

	memcpy(&header[hindex], &info->magic_1, sizeof(uint32_t));    // Magic 1
	hindex += sizeof(uint32_t);
	memcpy(&header[hindex], &info->magic_2, sizeof(uint32_t));    // Magic 2
	hindex += sizeof(uint32_t);
	memcpy(&header[hindex], &info->minor, sizeof(uint32_t));    // Minor
	hindex += sizeof(uint32_t);
	memcpy(&header[hindex], &info->platform, sizeof(uint32_t));    // Platform
	hindex += sizeof(uint32_t);
	memcpy(&header[hindex], &info->header_rev, sizeof(uint32_t));    // Header rev
	hindex += sizeof(uint32_t);
	memcpy(&header[hindex], &info->board, sizeof(uint32_t));    // Board
	hindex += sizeof(uint32_t);

	hindex              += sizeof(uint32_t);                            // Padding
	hindex              += sizeof(uint16_t);                            // ... Padding
	hindex              += sizeof(uint8_t);                             // And more weird padding
	recovery_num_devices = (uint8_t) info->num_devices;                 // u16 to u8...
	memcpy(&header[hindex], &recovery_num_devices, sizeof(uint8_t));    // Device count
	hindex += sizeof(uint8_t);

	for (i = 0; i < info->num_devices; i++) {
		memcpy(&header[hindex], &info->devices[i], sizeof(uint16_t));    // Device
		hindex += sizeof(uint16_t);
	}

	// Now, we write the header to the file
	if (fwrite(header, sizeof(unsigned char), header_size, output) < header_size) {
		fprintf(stderr, "Error writing update header: %s.\n", strerror(errno));
		free(header);
		return -1;
	}

	// Write the actual update
	free(header);
	return munger(input_tgz, output, 0, fake_sign);
}

int
    kindle_create_main(int argc, char* argv[])
{
	int                        opt;
	int                        opt_index;
	static const struct option opts[] = {
		{    "device", required_argument, NULL, 'd' },
		{       "key", required_argument, NULL, 'k' },
		{    "bundle", required_argument, NULL, 'b' },
		{    "srcrev", required_argument, NULL, 's' },
		{    "tgtrev", required_argument, NULL, 't' },
		{    "magic1", required_argument, NULL, '1' },
		{    "magic2", required_argument, NULL, '2' },
		{     "minor", required_argument, NULL, 'm' },
		{  "platform", required_argument, NULL, 'p' },
		{     "board", required_argument, NULL, 'B' },
		{    "hdrrev", required_argument, NULL, 'h' },
		{      "cert", required_argument, NULL, 'c' },
		{       "opt", required_argument, NULL, 'o' },
		{      "crit", required_argument, NULL, 'r' },
		{      "meta", required_argument, NULL, 'x' },
		{   "archive",       no_argument, NULL, 'a' },
		{  "unsigned",       no_argument, NULL, 'u' },
		{  "userdata",       no_argument, NULL, 'U' },
		{       "ota",       no_argument, NULL, 'O' },
		{    "legacy",       no_argument, NULL, 'C' },
		{ "packaging",       no_argument, NULL, 'X' },
		{        NULL,                 0, NULL,   0 }
	};
	UpdateInformation     info   = { "\0\0\0\0",
					 UnknownUpdate,
					 get_default_key(),
					 0,
					 UINT64_MAX,
					 0,
					 0,
					 0,
					 0,
					 NULL,
					 0,
					 0,
					 0,
					 CertificateDeveloper,
					 0,
					 0,
					 0,
					 NULL };
	FILE*                 input  = NULL;
	FILE*                 output = stdout;
	int                   i;
	unsigned int          ui;
	char*                 output_filename           = NULL;
	char**                input_list                = NULL;
	unsigned int          input_index               = 0;
	char*                 tarball_filename          = NULL;
	char*                 valid_update_file_pattern = NULL;
	int                   tarball_fd                = -1;
	const unsigned int    num_packaging_metastrings = 3;
	bool                  keep_archive              = false;
	bool                  skip_archive              = false;
	bool                  fake_sign                 = false;
	bool                  userdata_only             = false;
	bool                  enforce_ota               = false;
	bool                  enforce_source_rev        = false;
	bool                  enforce_target_rev        = false;
	bool                  legacy                    = false;
	unsigned int          real_blocksize;
	struct archive_entry* entry;
	struct archive*       match;
	int                   r;

	// Skip command
	argv++;
	argc--;

	// Update type
	if (argc < 1) {
		fprintf(stderr, "Not enough arguments.\n");
		return -1;
	}
	if (strncmp(argv[0], "ota2", 4) == 0) {
		info.version = OTAUpdateV2;
		memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
		real_blocksize = BLOCK_SIZE;
	} else if (strncmp(argv[0], "ota", 3) == 0) {
		info.version = OTAUpdate;
		memcpy(info.magic_number, "FC02", MAGIC_NUMBER_LENGTH);
		info.target_revision = UINT32_MAX;
		real_blocksize       = BLOCK_SIZE;
	} else if (strncmp(argv[0], "recovery2", 9) == 0) {
		info.version = RecoveryUpdateV2;
		memcpy(info.magic_number, "FB03", MAGIC_NUMBER_LENGTH);
		real_blocksize = RECOVERY_BLOCK_SIZE;
		// FB03 is at header_rev 0, don't force it to 2
	} else if (strncmp(argv[0], "recovery", 8) == 0) {
		info.version = RecoveryUpdate;
		memcpy(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH);
		info.target_revision = UINT32_MAX;
		real_blocksize       = RECOVERY_BLOCK_SIZE;
	} else if (strncmp(argv[0], "sig", 3) == 0) {
		info.version   = UpdateSignature;
		// For reference only, since we only support converting an existing tarball, we don't really care about that...
		//memcpy(info.magic_number, "SP01", MAGIC_NUMBER_LENGTH);
		real_blocksize = BLOCK_SIZE;
	} else {
		fprintf(stderr, "'%s' is not a valid update type.\n", argv[0]);
		goto do_error;
	}

	// Arguments
	while ((opt = getopt_long(argc, argv, "d:k:b:s:t:1:2:m:p:B:h:c:o:r:x:auUOCX", opts, &opt_index)) != -1) {
		switch (opt) {
			case 'd':
				// The aliases handle their memory allocation on their own, in one shot.
				if (strcasecmp(optarg, "kindle4") == 0) {
					memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = Kindle4NonTouch;
					info.devices[info.num_devices++]       = Kindle4NonTouchBlack;
				} else if (strcasecmp(optarg, "touch") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 3 + kt_with_unknown_devcodes;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = Kindle5TouchWiFi;
					info.devices[info.num_devices++]       = Kindle5TouchWiFi3G;
					info.devices[info.num_devices++]       = Kindle5TouchWiFi3GEurope;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = Kindle5TouchUnknown;
					}
				} else if (strcasecmp(optarg, "paperwhite") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 6;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi;
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi3G;
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi3GCanada;
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi3GEurope;
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi3GJapan;
					info.devices[info.num_devices++]       = KindlePaperWhiteWiFi3GBrazil;
				} else if (strcasecmp(optarg, "paperwhite2") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 12 + (kt_with_unknown_devcodes * 2);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFiJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3G;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3GCanada;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3GEurope;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3GRussia;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3GJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi4GBInternational;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3G4GBEurope;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3G4GB;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3G4GBCanada;
					info.devices[info.num_devices++]       = KindlePaperWhite2WiFi3G4GBBrazil;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite2Unknown_0xF4;
						info.devices[info.num_devices++] = KindlePaperWhite2Unknown_0xF9;
					}
				} else if (strcasecmp(optarg, "basic") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleBasic;
					info.devices[info.num_devices++]       = KindleBasicKiwi;
				} else if (strcasecmp(optarg, "voyage") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 6 + (kt_with_unknown_devcodes * 0);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleVoyageWiFi;
					info.devices[info.num_devices++]       = KindleVoyageWiFi3G;
					info.devices[info.num_devices++]       = KindleVoyageWiFi3GEurope;
					info.devices[info.num_devices++]       = KindleVoyageWiFi3GJapan;
					info.devices[info.num_devices++]       = KindleVoyageWiFi3GMexico;
					info.devices[info.num_devices++]       = KindleVoyageWiFi3G_0x4F;
				} else if (strcasecmp(optarg, "paperwhite3") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 12 + (kt_with_unknown_devcodes * 2);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi;
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi3GJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi3GCanada;
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi3G;
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi3GEurope;
					info.devices[info.num_devices++]       = KindlePaperWhite3WiFi3GMexico;
					info.devices[info.num_devices++]       = KindlePaperWhite3WhiteWiFi;
					info.devices[info.num_devices++]       = KindlePaperWhite3WhiteWiFi3GJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite3BlackWiFi32GBJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite3WhiteWiFi32GBJapan;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi3GInternational;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi3GInternationalBis;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePW3WhiteUnknown_0KD;
						info.devices[info.num_devices++] = KindlePW3WhiteUnknown_0KG;
					}
				} else if (strcasecmp(optarg, "oasis") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 5 + (kt_with_unknown_devcodes * 1);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleOasisWiFi;
					info.devices[info.num_devices++]       = KindleOasisWiFi3G;
					info.devices[info.num_devices++]       = KindleOasisWiFi3GEurope;
					info.devices[info.num_devices++]       = KindleOasisWiFi3GInternational;
					info.devices[info.num_devices++]       = KindleOasisWiFi3GChina;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleOasisUnknown_0GS;
					}
				} else if (strcasecmp(optarg, "basic2") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2 + (kt_with_unknown_devcodes * 1);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleBasic2;
					info.devices[info.num_devices++]       = KindleBasic2White;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic2Unknown_0DU;
					}
				} else if (strcasecmp(optarg, "oasis2") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 6 + (kt_with_unknown_devcodes * 9);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleOasis2WiFi8GB;
					info.devices[info.num_devices++]       = KindleOasis2WiFi3G32GB;
					info.devices[info.num_devices++]       = KindleOasis2WiFi32GB;
					info.devices[info.num_devices++]       = KindleOasis2WiFi3G32GBEurope;
					info.devices[info.num_devices++]       = KindleOasis2WiFi32GBChampagne;
					info.devices[info.num_devices++]       = KindleOasis2Unknown_0P6;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LM;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LN;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LP;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LQ;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0P2;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0P7;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S3;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S4;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S7;
					}
				} else if (strcasecmp(optarg, "paperwhite4") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 13 + (kt_with_unknown_devcodes * 8);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi8GB;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi32GB;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi4G32GB;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi4G32GBEurope;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi8GBIndia;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi4G32GBJapan;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi8GBBlue;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi32GBBlue;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi8GBPlum;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi8GBSage;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi32GBIndia;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi32GBPlum;
					info.devices[info.num_devices++]       = KindlePaperWhite4WiFi32GBSage;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T4;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T5;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T7;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TJ;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TK;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TL;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TM;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TN;
					}
				} else if (strcasecmp(optarg, "basic3") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 4 + (kt_with_unknown_devcodes * 2);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleBasic3;
					info.devices[info.num_devices++]       = KindleBasic3White;
					info.devices[info.num_devices++]       = KindleBasic3KidsEdition;
					info.devices[info.num_devices++]       = KindleBasic3White8GB;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic3Unknown_0WG;
						info.devices[info.num_devices++] = KindleBasic3Unknown_0WJ;
					}
				} else if (strcasecmp(optarg, "oasis3") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 6 + (kt_with_unknown_devcodes * 0);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleOasis3WiFi32GBChampagne;
					info.devices[info.num_devices++]       = KindleOasis3WiFi4G32GBJapan;
					info.devices[info.num_devices++]       = KindleOasis3WiFi4G32GB;
					info.devices[info.num_devices++]       = KindleOasis3WiFi32GB;
					info.devices[info.num_devices++]       = KindleOasis3WiFi8GB;
					info.devices[info.num_devices++]       = KindleOasis3WiFi4G32GBIndia;
				} else if (strcasecmp(optarg, "paperwhite5") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 6 + (kt_with_unknown_devcodes * 3);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindlePaperWhite5SignatureEdition;
					info.devices[info.num_devices++]       = KindlePaperWhite5;
					info.devices[info.num_devices++]       = KindlePaperWhite5SE_2BH;
					info.devices[info.num_devices++]       = KindlePaperWhite5_2DK;
					info.devices[info.num_devices++]       = KindlePaperWhite5_21A;
					info.devices[info.num_devices++]       = KindlePaperWhite5SE_219;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_1Q0;
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_1VD;
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_2BJ;
					}
				} else if (strcasecmp(optarg, "basic4") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2 + (kt_with_unknown_devcodes * 5);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleBasic4_2AQ;
					info.devices[info.num_devices++]       = KindleBasic4_2AP;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic4Unknown_22D;
						info.devices[info.num_devices++] = KindleBasic4Unknown_25T;
						info.devices[info.num_devices++] = KindleBasic4Unknown_23A;
						info.devices[info.num_devices++] = KindleBasic4Unknown_1XH;
						info.devices[info.num_devices++] = KindleBasic4Unknown_22C;
					}
				} else if (strcasecmp(optarg, "scribe") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 3 + (kt_with_unknown_devcodes * 5);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleScribe16GB_227;
					info.devices[info.num_devices++]       = KindleScribe64GB_23M;
					info.devices[info.num_devices++]       = KindleScribe_23L;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleScribeUnknown_27J;
						info.devices[info.num_devices++] = KindleScribeUnknown_2BL;
						info.devices[info.num_devices++] = KindleScribeUnknown_263;
						info.devices[info.num_devices++] = KindleScribeUnknown_2BM;
						info.devices[info.num_devices++] = KindleScribeUnknown_270;
					}
				} else if (strcasecmp(optarg, "basic5") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 0 + (kt_with_unknown_devcodes * 7);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L5;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L6;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L4;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L3;
						info.devices[info.num_devices++] = KindleBasic5Unknown_A89;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L2;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3KM;
					}
				} else if (strcasecmp(optarg, "paperwhite6") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 0 + (kt_with_unknown_devcodes * 10);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_349;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_346;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_33X;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_33W;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3HA;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H5;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H3;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H8;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3J5;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3JS;
					}
				}  else if (strcasecmp(optarg, "scribe2") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 0 + (kt_with_unknown_devcodes * 8);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleScribe2Unknown_3V0;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3V1;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X5;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3UV;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X4;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X3;
						info.devices[info.num_devices++] = KindleScribe2Unknown_41E;
						info.devices[info.num_devices++] = KindleScribe2Unknown_41D;
					}
				} else if (strcasecmp(optarg, "colorsoft") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 0 + (kt_with_unknown_devcodes * 10);
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H9;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H4;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3HB;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H6;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H2;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_34X;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H7;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3JT;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3J6;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_456;
					}
				} else if (strcasecmp(optarg, "kindle5") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices =
					    3 + kt_with_unknown_devcodes +           // K5
					    6 +                                      // PW1
					    12 + (kt_with_unknown_devcodes * 2) +    // PW2
					    2 +                                      // KT2
					    6 + (kt_with_unknown_devcodes * 0) +     // KV
					    12 + (kt_with_unknown_devcodes * 2) +    // PW3
					    5 + (kt_with_unknown_devcodes * 1) +     // Oasis
					    2 + (kt_with_unknown_devcodes * 1) +     // KT3
					    6 + (kt_with_unknown_devcodes * 9) +     // Oasis 2
					    13 + (kt_with_unknown_devcodes * 8) +    // PW4
					    4 + (kt_with_unknown_devcodes * 2) +     // KT4
					    6 + (kt_with_unknown_devcodes * 0) +     // KOA3
					    6 + (kt_with_unknown_devcodes * 3) +     // PW5
					    2 + (kt_with_unknown_devcodes * 5) +     // KT5
					    3 + (kt_with_unknown_devcodes * 5) +     // Scribe
					    0 + (kt_with_unknown_devcodes * 7) +     // KT6
					    0 + (kt_with_unknown_devcodes * 10) +    // PW6
						0 + (kt_with_unknown_devcodes * 8) +     // Scribe 2
					    0 + (kt_with_unknown_devcodes * 10);     // CS
					info.devices                     = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					// K5
					info.devices[info.num_devices++] = Kindle5TouchWiFi;
					info.devices[info.num_devices++] = Kindle5TouchWiFi3G;
					info.devices[info.num_devices++] = Kindle5TouchWiFi3GEurope;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = Kindle5TouchUnknown;
					}
					// PW1
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi;
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi3G;
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi3GCanada;
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi3GEurope;
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi3GJapan;
					info.devices[info.num_devices++] = KindlePaperWhiteWiFi3GBrazil;
					// PW2
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFiJapan;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3G;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3GCanada;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3GEurope;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3GRussia;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3GJapan;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi4GBInternational;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3G4GBEurope;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3G4GB;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3G4GBCanada;
					info.devices[info.num_devices++] = KindlePaperWhite2WiFi3G4GBBrazil;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite2Unknown_0xF4;
						info.devices[info.num_devices++] = KindlePaperWhite2Unknown_0xF9;
					}
					// KT2
					info.devices[info.num_devices++] = KindleBasic;
					info.devices[info.num_devices++] = KindleBasicKiwi;
					// KV
					info.devices[info.num_devices++] = KindleVoyageWiFi;
					info.devices[info.num_devices++] = KindleVoyageWiFi3G;
					info.devices[info.num_devices++] = KindleVoyageWiFi3GEurope;
					info.devices[info.num_devices++] = KindleVoyageWiFi3GJapan;
					info.devices[info.num_devices++] = KindleVoyageWiFi3GMexico;
					info.devices[info.num_devices++] = KindleVoyageWiFi3G_0x4F;
					// Black PW3
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi;
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi3GJapan;
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi3GCanada;
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi3G;
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi3GEurope;
					info.devices[info.num_devices++] = KindlePaperWhite3WiFi3GMexico;
					info.devices[info.num_devices++] = KindlePaperWhite3BlackWiFi32GBJapan;
					// White PW3
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi3GJapan;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi32GBJapan;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi3GInternational;
					info.devices[info.num_devices++] = KindlePaperWhite3WhiteWiFi3GInternationalBis;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePW3WhiteUnknown_0KD;
						info.devices[info.num_devices++] = KindlePW3WhiteUnknown_0KG;
					}
					// Oasis
					info.devices[info.num_devices++] = KindleOasisWiFi;
					info.devices[info.num_devices++] = KindleOasisWiFi3G;
					info.devices[info.num_devices++] = KindleOasisWiFi3GEurope;
					info.devices[info.num_devices++] = KindleOasisWiFi3GInternational;
					info.devices[info.num_devices++] = KindleOasisWiFi3GChina;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleOasisUnknown_0GS;
					}
					// KT3
					info.devices[info.num_devices++] = KindleBasic2;
					info.devices[info.num_devices++] = KindleBasic2White;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic2Unknown_0DU;
					}
					// Oasis 2
					info.devices[info.num_devices++] = KindleOasis2WiFi8GB;
					info.devices[info.num_devices++] = KindleOasis2WiFi3G32GB;
					info.devices[info.num_devices++] = KindleOasis2WiFi32GB;
					info.devices[info.num_devices++] = KindleOasis2WiFi3G32GBEurope;
					info.devices[info.num_devices++] = KindleOasis2WiFi32GBChampagne;
					info.devices[info.num_devices++] = KindleOasis2Unknown_0P6;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LM;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LN;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LP;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0LQ;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0P2;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0P7;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S3;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S4;
						info.devices[info.num_devices++] = KindleOasis2Unknown_0S7;
					}
					// PW4
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi8GB;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi32GB;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi4G32GB;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi4G32GBEurope;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi8GBIndia;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi4G32GBJapan;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi8GBBlue;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi32GBBlue;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi8GBPlum;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi8GBSage;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi32GBIndia;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi32GBPlum;
					info.devices[info.num_devices++] = KindlePaperWhite4WiFi32GBSage;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T4;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T5;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0T7;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TJ;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TK;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TL;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TM;
						info.devices[info.num_devices++] = KindlePaperWhite4Unknown_0TN;
					}
					// KT4
					info.devices[info.num_devices++] = KindleBasic3;
					info.devices[info.num_devices++] = KindleBasic3White;
					info.devices[info.num_devices++] = KindleBasic3KidsEdition;
					info.devices[info.num_devices++] = KindleBasic3White8GB;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic3Unknown_0WG;
						info.devices[info.num_devices++] = KindleBasic3Unknown_0WJ;
					}
					// KOA3
					info.devices[info.num_devices++] = KindleOasis3WiFi32GBChampagne;
					info.devices[info.num_devices++] = KindleOasis3WiFi4G32GBJapan;
					info.devices[info.num_devices++] = KindleOasis3WiFi4G32GB;
					info.devices[info.num_devices++] = KindleOasis3WiFi32GB;
					info.devices[info.num_devices++] = KindleOasis3WiFi8GB;
					info.devices[info.num_devices++] = KindleOasis3WiFi4G32GBIndia;
					// PW5
					info.devices[info.num_devices++] = KindlePaperWhite5SignatureEdition;
					info.devices[info.num_devices++] = KindlePaperWhite5;
					info.devices[info.num_devices++] = KindlePaperWhite5SE_2BH;
					info.devices[info.num_devices++] = KindlePaperWhite5_2DK;
					info.devices[info.num_devices++] = KindlePaperWhite5_21A;
					info.devices[info.num_devices++] = KindlePaperWhite5SE_219;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_1Q0;
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_1VD;
						info.devices[info.num_devices++] = KindlePaperWhite5Unknown_2BJ;
					}
					// KT5
					info.devices[info.num_devices++] = KindleBasic4_2AQ;
					info.devices[info.num_devices++] = KindleBasic4_2AP;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic4Unknown_22D;
						info.devices[info.num_devices++] = KindleBasic4Unknown_25T;
						info.devices[info.num_devices++] = KindleBasic4Unknown_23A;
						info.devices[info.num_devices++] = KindleBasic4Unknown_1XH;
						info.devices[info.num_devices++] = KindleBasic4Unknown_22C;
					}
					// Scribe
					info.devices[info.num_devices++] = KindleScribe16GB_227;
					info.devices[info.num_devices++] = KindleScribe64GB_23M;
					info.devices[info.num_devices++] = KindleScribe_23L;
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleScribeUnknown_27J;
						info.devices[info.num_devices++] = KindleScribeUnknown_2BL;
						info.devices[info.num_devices++] = KindleScribeUnknown_263;
						info.devices[info.num_devices++] = KindleScribeUnknown_2BM;
						info.devices[info.num_devices++] = KindleScribeUnknown_270;
					}
					// KT6
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L5;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L6;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L4;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L3;
						info.devices[info.num_devices++] = KindleBasic5Unknown_A89;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3L2;
						info.devices[info.num_devices++] = KindleBasic5Unknown_3KM;
					}
					// PW6
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_349;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_346;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_33X;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_33W;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3HA;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H5;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H3;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3H8;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3J5;
						info.devices[info.num_devices++] = KindlePaperWhite6Unknown_3JS;
					}
					// Scribe 2
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleScribe2Unknown_3V0;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3V1;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X5;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3UV;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X4;
						info.devices[info.num_devices++] = KindleScribe2Unknown_3X3;
						info.devices[info.num_devices++] = KindleScribe2Unknown_41E;
						info.devices[info.num_devices++] = KindleScribe2Unknown_41D;
					}
					// CS
					if (kt_with_unknown_devcodes) {
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H9;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H4;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3HB;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H6;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H2;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_34X;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3H7;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3JT;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_3J6;
						info.devices[info.num_devices++] = KindleColorSoftUnknown_456;
					}
				} else if (kt_with_unknown_devcodes &&
					   (strcasecmp(optarg, "unknown") == 0 || strcasecmp(optarg, "datamined") == 0)) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);    // Meh?
					const unsigned int num_aliased_devices = 7;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x16;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x21;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x07;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x0B;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x0C;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x0D;
					info.devices[info.num_devices++]       = ValidKindleUnknown_0x99;
				} else if (strcasecmp(optarg, "kindle2") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = Kindle2US;
					info.devices[info.num_devices++]       = Kindle2International;
				} else if (strcasecmp(optarg, "kindledx") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 3;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = KindleDXUS;
					info.devices[info.num_devices++]       = KindleDXInternational;
					info.devices[info.num_devices++]       = KindleDXGraphite;
				} else if (strcasecmp(optarg, "kindle3") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 3;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = Kindle3WiFi;
					info.devices[info.num_devices++]       = Kindle3WiFi3G;
					info.devices[info.num_devices++]       = Kindle3WiFi3GEurope;
				} else if (strcasecmp(optarg, "legacy") == 0) {
					memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					const unsigned int num_aliased_devices = 2 + 3 + 3;
					info.devices                           = realloc(info.devices,
                                                               (info.num_devices + num_aliased_devices) * sizeof(Device));
					info.devices[info.num_devices++]       = Kindle2US;
					info.devices[info.num_devices++]       = Kindle2International;
					info.devices[info.num_devices++]       = KindleDXUS;
					info.devices[info.num_devices++]       = KindleDXInternational;
					info.devices[info.num_devices++]       = KindleDXGraphite;
					info.devices[info.num_devices++]       = Kindle3WiFi;
					info.devices[info.num_devices++]       = Kindle3WiFi3G;
					info.devices[info.num_devices++]       = Kindle3WiFi3GEurope;
				} else {
					// We'll only ever set a single device code here
					info.devices = realloc(info.devices, ++info.num_devices * sizeof(Device));
					// K1
					if (strcasecmp(optarg, "k1") == 0) {
						info.devices[info.num_devices - 1] = Kindle1;
					}
					// K2
					else if (strcasecmp(optarg, "k2") == 0) {
						info.devices[info.num_devices - 1] = Kindle2US;
					} else if (strcasecmp(optarg, "k2i") == 0) {
						info.devices[info.num_devices - 1] = Kindle2International;
					}
					// DX
					else if (strcasecmp(optarg, "dx") == 0) {
						info.devices[info.num_devices - 1] = KindleDXUS;
					} else if (strcasecmp(optarg, "dxi") == 0) {
						info.devices[info.num_devices - 1] = KindleDXInternational;
					} else if (strcasecmp(optarg, "dxg") == 0) {
						info.devices[info.num_devices - 1] = KindleDXGraphite;
					}
					// K3
					else if (strcasecmp(optarg, "k3w") == 0) {
						info.devices[info.num_devices - 1] = Kindle3WiFi;
					} else if (strcasecmp(optarg, "k3g") == 0) {
						info.devices[info.num_devices - 1] = Kindle3WiFi3G;
					} else if (strcasecmp(optarg, "k3gb") == 0) {
						info.devices[info.num_devices - 1] = Kindle3WiFi3GEurope;
					}
					// K4
					else if (strcasecmp(optarg, "k4") == 0) {
						info.devices[info.num_devices - 1] = Kindle4NonTouch;
						memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
					} else if (strcasecmp(optarg, "k4b") == 0) {
						info.devices[info.num_devices - 1] = Kindle4NonTouchBlack;
						memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
					}
					// KT
					// NOTE: Magic number switch to 'versionless' update types here...
					//       FW >= 5.6.1 apparently dropped support for these in the UYK menu...
					else if (strcasecmp(optarg, "k5w") == 0) {
						// NOTE: Undocumented, left in because used to generate old data.stgz packages
						info.devices[info.num_devices - 1] = Kindle5TouchWiFi;
						memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
					}
					// N/A
					else if (strcasecmp(optarg, "none") == 0) {
						info.devices[info.num_devices - 1] = KindleUnknown;
						// We *really* mean no devices, so reset num_devices ;).
						info.num_devices                   = 0;
					} else if (strcasecmp(optarg, "auto") == 0 ||
						   strcasecmp(optarg, "current") == 0) {
						// Detect the current Kindle model
						FILE* kindle_usid;
						// NOTE: Newer devices also have /proc/serial?
						if ((kindle_usid = fopen("/proc/usid", "rb")) == NULL) {
							fprintf(
							    stderr,
							    "Cannot open /proc/usid (not running on a Kindle?): %s.\n",
							    strerror(errno));
							goto do_error;
						}
						unsigned char serial_no[SERIAL_NO_LENGTH];
						if (fread(
							serial_no, sizeof(unsigned char), SERIAL_NO_LENGTH, kindle_usid) <
							SERIAL_NO_LENGTH ||
						    ferror(kindle_usid) != 0) {
							fprintf(
							    stderr, "Error reading /proc/usid: %s.\n", strerror(errno));
							fclose(kindle_usid);
							goto do_error;
						}
						fclose(kindle_usid);
						// Get the device code...
						char   device_code[3 + 1] = { 0 };
						Device dev_code           = KindleUnknown;
						// NOTE: If the S/N starts with B or 9,
						//       assume it's an older device with an hexadecimal device code
						if (serial_no[0] == 'B' || serial_no[0] == '9') {
							snprintf(device_code, 2 + 1, "%.*s", 2, serial_no + 2);
							dev_code = (Device) strtoul(device_code, NULL, 16);
							// ... And finally, unless we're feeling adventurous,
							// check if it's really a valid device...
							if (!kt_with_unknown_devcodes &&
							    strcmp(convert_device_id(dev_code), "Unknown") == 0) {
								fprintf(stderr,
									"Unknown device %s (0x%02X) [%.6s].\n",
									device_code,
									dev_code,
									serial_no);
								goto do_error;
							}
						} else {
							// ... try the new device ID scheme if it doesn't...
							snprintf(device_code, 3 + 1, "%.*s", 3, serial_no + 3);
							dev_code = (Device) from_base(device_code, 32);
							// ... And finally, unless we're feeling adventurous,
							// check if it's really a valid device...
							if (!kt_with_unknown_devcodes &&
							    strcmp(convert_device_id(dev_code), "Unknown") == 0) {
								fprintf(stderr,
									"Unknown device %s (0x%03X) [%.6s].\n",
									device_code,
									dev_code,
									serial_no);
								goto do_error;
							}
						}
						// Yay, known valid device code :)
						info.devices[info.num_devices - 1] = dev_code;
						// Roughly guess a decent magic number...
						if (dev_code < Kindle4NonTouch) {
							memcpy(info.magic_number, "FC02", MAGIC_NUMBER_LENGTH);
						} else if (dev_code == Kindle4NonTouch ||
							   dev_code == Kindle4NonTouchBlack) {
							memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
						} else {
							memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
						}
					} else {
						// Check if we passed a device code, be it as a ready-to-use hex value,
						// or a to-be-decoded serial fragment...
						char*  endptr;
						Device dev_code = (Device) strtoul(optarg, &endptr, 16);
						// Check that it even remotely looks like a device code, old or new, first...
						// NOTE: The range is 01 to A89 for now, update as needed!
						if (*endptr != '\0' || dev_code <= 0x00 || dev_code > 0x2909) {
							// That was either an out of range hexadecimal value,
							// or not an hexadecimal value at all...
							if (strcmp(convert_device_id(dev_code), "Unknown") == 0) {
								// ... in which case, try to see if that was
								// a serial fragment following the new device id scheme...
								dev_code = (Device) from_base(optarg, 32);
								// Unless we're feeling adventurous,
								// check if it's a valid device...
								if (!kt_with_unknown_devcodes &&
								    strcmp(convert_device_id(dev_code), "Unknown") == 0) {
									fprintf(stderr,
										"Unknown device %s (0x%03X).\n",
										optarg,
										dev_code);
									goto do_error;
								}
							}
						} else {
							// Okay, that looked like an in-range hex value,
							// make sure it matches an hex-only device id if
							// we're not bypassing device checks...
							if (!kt_with_unknown_devcodes &&
							    strcmp(convert_device_id(dev_code), "Unknown") == 0) {
								fprintf(stderr,
									"Unknown device %s (0x%02X).\n",
									optarg,
									dev_code);
								goto do_error;
							}
						}
						// Yay, known valid device code :)
						info.devices[info.num_devices - 1] = dev_code;
						// Roughly guess a decent magic number...
						if (dev_code < Kindle4NonTouch) {
							memcpy(info.magic_number, "FC02", MAGIC_NUMBER_LENGTH);
						} else if (dev_code == Kindle4NonTouch ||
							   dev_code == Kindle4NonTouchBlack) {
							memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
						} else {
							memcpy(info.magic_number, "FD04", MAGIC_NUMBER_LENGTH);
						}
					}
				}
				break;
			case 'p':
				if (strcasecmp(optarg, "unspecified") == 0) {
					info.platform = Plat_Unspecified;
				} else if (strcasecmp(optarg, "mario") == 0) {
					info.platform = MarioDeprecated;
				} else if (strcasecmp(optarg, "luigi") == 0) {
					info.platform = Luigi;
				} else if (strcasecmp(optarg, "banjo") == 0) {
					info.platform = Banjo;
				} else if (strcasecmp(optarg, "yoshi") == 0) {
					info.platform = Yoshi;
				} else if (strcasecmp(optarg, "yoshime-proto") == 0 ||
					   strcasecmp(optarg, "yoshime-p") == 0) {
					info.platform = YoshimeProto;
				} else if (strcasecmp(optarg, "yoshime") == 0) {
					info.platform = Yoshime;
				} else if (strcasecmp(optarg, "wario") == 0) {
					info.platform = Wario;
				} else if (strcasecmp(optarg, "duet") == 0) {
					info.platform = Duet;
				} else if (strcasecmp(optarg, "heisenberg") == 0) {
					info.platform = Heisenberg;
				} else if (strcasecmp(optarg, "zelda") == 0) {
					info.platform = Zelda;
				} else if (strcasecmp(optarg, "rex") == 0) {
					info.platform = Rex;
				} else if (strcasecmp(optarg, "bellatrix") == 0) {
					info.platform = Bellatrix;
				} else if (strcasecmp(optarg, "bellatrix3") == 0) {
					info.platform = Bellatrix3;
				} else if (strcasecmp(optarg, "bellatrix4") == 0) {
					info.platform = Bellatrix4;
				} else {
					fprintf(stderr, "Unknown platform %s.\n", optarg);
					goto do_error;
				}
				break;
			case 'B':
				if (strcasecmp(optarg, "unspecified") == 0) {
					info.board = Board_Unspecified;
				} else if (strcasecmp(optarg, "tequila") == 0) {
					info.board = Tequila;
				} else if (strcasecmp(optarg, "whitney") == 0) {
					info.board = Whitney;
				} else {
					fprintf(stderr, "Unknown board %s.\n", optarg);
					goto do_error;
				}
				break;
			case 'h':
				info.header_rev = (uint32_t) atoi(optarg);
				break;
			case 'k':
				if (nettle_rsa_privkey_from_pem(optarg, &info.sign_pkey) != 0) {
					fprintf(stderr, "Key '%s' cannot be loaded.\n", optarg);
					goto do_error;
				}
				break;
			case 'b':
				strncpy(info.magic_number, optarg, MAGIC_NUMBER_LENGTH);    // Flawfinder: ignore
				if ((info.version = get_bundle_version(optarg)) == UnknownUpdate) {
					fprintf(stderr, "Invalid bundle version %s.\n", optarg);
					goto do_error;
				}
				break;
			case 's':
				// Handle "min" as a special value
				if (strcasecmp(optarg, "min") == 0) {
					info.source_revision = 0;
				} else {
					info.source_revision = strtoull(optarg, NULL, 0);
				}
				enforce_source_rev = true;
				break;
			case 't':
				// And, arguably more useful, handle "max" as a special value
				if (strcasecmp(optarg, "max") == 0) {
					// NOTE: Given the way we handle commands vs. args, by now, info.version should be accurate.
					if (info.version == OTAUpdateV2 || info.version == RecoveryUpdateV2 ||
					    (info.version == RecoveryUpdate && info.header_rev == 2)) {
						info.target_revision = UINT64_MAX;
					} else {
						info.target_revision = UINT32_MAX;
					}
				} else {
					info.target_revision = strtoull(optarg, NULL, 0);
				}
				enforce_target_rev = true;
				break;
			case '1':
				info.magic_1 = (uint32_t) atoi(optarg);
				break;
			case '2':
				info.magic_2 = (uint32_t) atoi(optarg);
				break;
			case 'm':
				info.minor = (uint32_t) atoi(optarg);
				break;
			case 'c':
				info.certificate_number = (CertificateNumber) atoi(optarg);
				break;
			case 'o':
				info.optional = (uint8_t) atoi(optarg);
				break;
			case 'r':
				info.critical = (uint8_t) atoi(optarg);
				break;
			case 'x':
				// A metastring must contain an '=' character (remember, it's a key=value pair ;))
				if (strchr(optarg, '=') == NULL) {
					fprintf(stderr, "Invalid metastring. Format: key=value, input: %s\n", optarg);
					goto do_error;
				}
				// Flawfinder: ignore
				if (strlen(optarg) > 0xFFFFu) {
					fprintf(stderr,
						"Metastring too long. Max length: %u, input length: %zu\n",
						0xFFFFu,
						strlen(optarg));    // Flawfinder: ignore
					goto do_error;
				}
				info.metastrings = realloc(info.metastrings, ++info.num_meta * sizeof(char*));
				info.metastrings[info.num_meta - 1] = strdup(optarg);
				break;
			case 'X':
				info.metastrings = realloc(info.metastrings,
							   (info.num_meta + num_packaging_metastrings) * sizeof(char*));
				char metabuff[128];
				// Start with PackagedWith
				snprintf(metabuff,
					 sizeof(metabuff),
					 "PackagedWith=KindleTool %s built by %s",
					 KT_VERSION,
					 KT_USERATHOST);
				info.metastrings[info.num_meta++] = strdup(metabuff);
				// Then PackagedBy
#if defined(_WIN32) && !defined(__CYGWIN__)
				DWORD len;
				// Get hostname
				char  nodename[256];
				len = sizeof(nodename);

				if (!GetComputerName(nodename, &len)) {
					snprintf(nodename, sizeof(nodename), "%s", "somewhere");
				}

				// Get username
				char username[100];
				len = sizeof(username);

				if (!GetUserName(username, &len)) {
					snprintf(metabuff, sizeof(metabuff), "PackagedBy=someone@%s", nodename);
				} else {
					snprintf(metabuff, sizeof(metabuff), "PackagedBy=%s@%s", username, nodename);
				}
#else
				// Get hostname
				// NOTE: macOS defaults to the fqdn, and AFAICT, we can't simply wrangle the short one from gethostname()...
				char nodename[HOST_NAME_MAX];
				if (gethostname(nodename, HOST_NAME_MAX) != 0) {
					snprintf(nodename, sizeof(nodename), "%s", "somewhere");
				}
				// Get username
				// NOTE: getlogin() is a cheap-ass way of achieving roughly the same thing.
				struct passwd* pwd;
				if ((pwd = getpwuid(geteuid())) != NULL) {
					snprintf(metabuff, sizeof(metabuff), "PackagedBy=%s@%s", pwd->pw_name, nodename);
				} else {
					snprintf(
					    metabuff, sizeof(metabuff), "PackagedBy=%ld@%s", (long) geteuid(), nodename);
				}
#endif
				info.metastrings[info.num_meta++] = strdup(metabuff);
				// And finally PackagedOn
				// Get UTC time
				time_t     now                    = time(NULL);
				struct tm* gmt;
				gmt = gmtime(&now);
				char sz_time[22];
				strftime(sz_time, sizeof(sz_time), "%Y-%m-%d @ %H:%M:%S", gmt);
				snprintf(metabuff, sizeof(metabuff), "PackagedOn=%s UTC", sz_time);
				info.metastrings[info.num_meta++] = strdup(metabuff);
				break;
			case 'a':
				keep_archive = true;
				break;
			case 'u':
				fake_sign = true;
				break;
			case 'U':
				userdata_only = true;
				break;
			case 'O':
				enforce_ota = true;
				break;
			case 'C':
				legacy = true;
				break;
			case ':':
				fprintf(stderr, "Missing argument for switch '%c'.\n", optopt);
				goto do_error;
				break;
			case '?':
				fprintf(stderr, "Unknown switch '%c'.\n", optopt);
				goto do_error;
				break;
			default:
				fprintf(stderr, "?? Unknown option code 0%o ??\n", (unsigned int) opt);
				goto do_error;
				break;
		}
	}

	// Signed userdata packages are very peculiar, handle them on their own...
	if (userdata_only) {
		// Needs to be a signed package
		if (info.version != UpdateSignature) {
			fprintf(stderr,
				"Invalid update type (%s) for an userdata package.\n",
				convert_bundle_version(info.version));
			goto do_error;
		}
	} else {
		// Did we want to enforce an OTA bundle type?
		if (enforce_ota) {
			// Only makes sense for ota2...
			if (info.version != OTAUpdateV2) {
				fprintf(
				    stderr,
				    "Invalid update type (%s). Enforcing the versioned OTA bundle type only makes sense for OTA V2.\n",
				    convert_bundle_version(info.version));
				goto do_error;
			}
			// We of course need the versioned ota bundle type...
			memcpy(info.magic_number, "FC04", MAGIC_NUMBER_LENGTH);
			// But also a source & target version!
			if (!enforce_source_rev) {
				info.source_revision = 2443670049U;    // FW 5.5.0
			}
			if (!enforce_target_rev) {
				info.target_revision = 4351970007U + 1U;    // FW 5.17.1.0.4 (KT6)
			}
			// NOTE: Don't expect those to be entirely consistent when crossing devices
			//       (f.g., the Touch's FW 5.3.7.3 has a higher OTA build number than the KV's FW 5.5.0)
			// NOTE: The VoiceView packages may sometimes have a slightly higher Target OTA flag, f.g.,
			//       while FW 5.10.1.2 was at 3360340004, the PW4 VoiceView packages targeted 3372830002,
			//       which turned out to be FW 5.10.1.3
			// NOTE: In a fun, new, twist, FW 5.12.2.1.1 appears to prevent same-to-same updates, hence the + 1...
		}
		// Musn't be *only* a sig envelope...
		if (info.version == UpdateSignature) {
			fprintf(stderr,
				"Invalid update type (%s) for an update package.\n",
				convert_bundle_version(info.version));
			goto do_error;
		}
		// Validation (Allow 0 devices in Recovery V2 & FB02 h2, allow multiple devices in OTA V2 & Recovery V2)
		if ((info.num_devices < 1 &&
		     (info.version != RecoveryUpdateV2 && !(info.version == RecoveryUpdate && info.header_rev == 2))) ||
		    ((info.version != OTAUpdateV2 && info.version != RecoveryUpdateV2) && info.num_devices > 1)) {
			fprintf(stderr,
				"Invalid number of supported devices (%hu) for this update type (%s).\n",
				info.num_devices,
				convert_bundle_version(info.version));
			goto do_error;
		}
		if ((info.version != OTAUpdateV2 && info.version != RecoveryUpdateV2 &&
		     !(info.version == RecoveryUpdate && info.header_rev == 2)) &&
		    (info.source_revision > UINT32_MAX || info.target_revision > UINT32_MAX)) {
			fprintf(stderr,
				"Source/target revision for this update type (%s) cannot exceed %u.\n",
				convert_bundle_version(info.version),
				UINT32_MAX);
			goto do_error;
		}
		// When building an ota update with ota2 only devices, don't try to use non ota v1 bundle versions,
		// reset it to FC02, or shit happens.
		if (info.version == OTAUpdate) {
			// OTA V1 only supports one device, we don't need to loop (fix anything newer than a K3GB)
			if (info.devices[0] > Kindle3WiFi3GEurope &&
			    (memcmp(info.magic_number, "FC02", MAGIC_NUMBER_LENGTH) != 0 &&
			     memcmp(info.magic_number, "FD03", MAGIC_NUMBER_LENGTH) != 0)) {
				// FC04 is hardcoded when we set K4 as a device, and FD04 when we ask for a K5 and up, so fix it silently.
				memcpy(info.magic_number, "FC02", MAGIC_NUMBER_LENGTH);
			}
		}
		// Same thing with recovery updates
		if (info.version == RecoveryUpdate) {
			// It's called FB02.2 for a reason...
			// Plus, we can have a null/none device with it, so we avoid the same blowup as the RecoveryV2 check ;).
			if ((info.header_rev == 2 || info.devices[0] > Kindle3WiFi3GEurope) &&
			    (memcmp(info.magic_number, "FB01", MAGIC_NUMBER_LENGTH) != 0 &&
			     memcmp(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH) != 0)) {
				memcpy(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH);
			}
		}
		// Same thing with recovery updates v2
		if (info.version == RecoveryUpdateV2) {
			// Make sure we have a sane magic number...
			// We either don't yet have one set when not specifying any device,
			// or what's set corresponds to OTA update types when specifying anything since the K4...
			if (memcmp(info.magic_number, "FB03", MAGIC_NUMBER_LENGTH) != 0) {
				// NOTE: This effectively prevents us from setting a custom magic number.
				//       Which is not really something you'd want to do in this case anyway...
				memcpy(info.magic_number, "FB03", MAGIC_NUMBER_LENGTH);
			}
		}
		// We need a platform id, board id (& header rev?) for recovery2
		if (info.version == RecoveryUpdateV2) {
			if (strcmp(convert_platform_id(info.platform), "Unknown") == 0) {
				fprintf(stderr,
					"You need to set a platform for this update type (%s).\n",
					convert_bundle_version(info.version));
				goto do_error;
			}
			if (strcmp(convert_board_id(info.board), "Unknown") == 0) {
				fprintf(stderr,
					"You need to set a board for this update type (%s).\n",
					convert_bundle_version(info.version));
				goto do_error;
			}
			// Don't bother for header rev? We don't for other potentially optional flags in recovery, so...
		}
		// We need a platform id & board id for recovery FB02 V2
		if (info.version == RecoveryUpdate) {
			if (memcmp(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH) == 0 && info.header_rev == 2 &&
			    strcmp(convert_platform_id(info.platform), "Unknown") == 0) {
				fprintf(stderr,
					"You need to set a platform for this update type (%s).\n",
					convert_bundle_version(info.version));
				goto do_error;
			}
			if (memcmp(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH) == 0 && info.header_rev == 2 &&
			    strcmp(convert_board_id(info.board), "Unknown") == 0) {
				fprintf(stderr,
					"You need to set a board for this update type (%s).\n",
					convert_bundle_version(info.version));
				goto do_error;
			}
		}
		// Right now, we don't use device at all for FB02.2, so reset it to none to have a consistent recap... FIXME?
		if (info.version == RecoveryUpdate) {
			if (memcmp(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH) == 0 && info.header_rev == 2 &&
			    info.num_devices > 0) {
				info.num_devices               = 0;
				info.devices[info.num_devices] = KindleUnknown;
			}
		}
		// We of course need a full magic number...
		// As magic_number is not NULL terminated, we cannot use strlen,
		// so let one of our helper functions do the job...
		if (get_bundle_version(info.magic_number) == UnknownUpdate) {
			fprintf(stderr,
				"You need to set a valid bundle version for this update type (%s), '%s' is invalid.\n",
				convert_bundle_version(info.version),
				info.magic_number);
			goto do_error;
		}
	}

	// If we don't actually build an archive, legacy mode makes no sense
	if (skip_archive) {
		legacy = false;
	}

	if (optind < argc) {
		// Iterate over non-options (the file(s) we passed)
		while (optind < argc) {
			// The last one will always be our output (but only check if we have at least one input file,
			// we might really want to output to stdout)
			if (optind == argc - 1 && input_index > 0) {
				output_filename = strdup(argv[optind++]);
				// If it's a single dash, output to stdout (like tar cf -)
				if (strcmp(output_filename, "-") == 0) {
					free(output_filename);
					output_filename = NULL;
				}
			} else {
				// Build a list of all our input files/dirs,
				// libarchive will do most of the heavy lifting for us
				// (c.f., http://stackoverflow.com/questions/1182534/#1182649)
				input_list                  = realloc(input_list, ++input_index * sizeof(char*));
				input_list[input_index - 1] = strdup(argv[optind++]);
			}
		}
	} else {
		fprintf(stderr, "No input/output specified.\n");
		goto do_error;
	}

	// While we're at it, check that our output name follows the proper naming scheme when creating a valid update package
	if (output_filename != NULL) {
		// Use libarchive's pattern matching, because it handles ./ in a smart way
		match = archive_match_new();
		entry = archive_entry_new();

		// Handle signed & fake userdata packages...
		if (fake_sign || userdata_only) {
			valid_update_file_pattern = strdup("./data\\.stgz$");
		} else {
			// NOTE: Recovery updates must be lowercase!
			if (info.version == RecoveryUpdate || info.version == RecoveryUpdateV2) {
				valid_update_file_pattern = strdup("./update*\\.bin$");
			} else {
				valid_update_file_pattern = strdup("./[Uu]pdate*\\.bin$");
			}
		}
		if (archive_match_exclude_pattern(match, valid_update_file_pattern) != ARCHIVE_OK) {
			fprintf(stderr, "archive_match_exclude_pattern() failed: %s.\n", archive_error_string(match));
		}
		free(valid_update_file_pattern);

		archive_entry_copy_pathname(entry, output_filename);

		r = archive_match_path_excluded(match, entry);
		if (r != 1) {
			if (r < 0) {
				fprintf(
				    stderr, "archive_match_path_excluded() failed: %s.\n", archive_error_string(match));
			}
			fprintf(
			    stderr,
			    "Your output file '%s' needs to follow the proper naming scheme (%s) in order to be picked up by the Kindle.\n",
			    output_filename,
			    (fake_sign || userdata_only) ? "data.stgz" : "update*.bin");
#if defined(_WIN32) && !defined(__CYGWIN__)
			fprintf(
			    stderr,
			    "As an added quirk, on Windows, make sure you're using UNIX-style forward slashes ('/') in your output file path, instead of Windows-style backward slashes ('\\'), and do NOT leave any trailing slashes.\n");
#endif
			archive_entry_free(entry);
			archive_match_free(match);
			goto do_error;
		}

		// Cleanup
		archive_entry_free(entry);
		archive_match_free(match);

		// Check to see if we can write to our output file
		// (do it now instead of earlier, this way the pattern matching has been done,
		// and we potentially avoid fopen squishing a file we meant as input, not output)
		if ((output = fopen(output_filename, "wb")) == NULL) {
			fprintf(
			    stderr, "Cannot create output package file '%s': %s.\n", output_filename, strerror(errno));
			goto do_error;
		}
	} else {
		// If we're really outputting to stdout, fix the output filename
		output_filename = strdup("standard output");
	}

	// If we only provided a single input file, and it's a tarball, assume it's properly packaged,
	// and just sign/munge it. (Restore backwards compatibilty with ixtab's tools, among other things)
	if (input_index == 1) {
		if (IS_TGZ(input_list[0]) || IS_TARBALL(input_list[0])) {
			// NOTE: There's no real check besides the file extension...
			skip_archive     = true;
			// Use it as our tarball...
			tarball_filename = strdup(input_list[0]);
		}
	}

	// Don't try to build an unsigned package if we didn't feed a single proper tarball
	if (fake_sign && !skip_archive) {
		fprintf(stderr, "You need to feed me a single tarball to build an unsigned package.\n");
		goto do_error;
	}

	// Same thing when building a signed userdata package
	if (userdata_only && !skip_archive) {
		fprintf(stderr, "You need to feed me a single tarball to build a signed userdata package.\n");
		goto do_error;
	}

	// If we need to build a tarball, do it in a tempfile
	if (!skip_archive) {
		// We need a proper mkstemp template
		char tartmpfile[PATH_MAX];
		snprintf(tartmpfile, PATH_MAX, "%s/%s", kt_tempdir, "kindletool_create_tarball_XXXXXX");
		tarball_filename = strdup(tartmpfile);
		tarball_fd       = mkstemp(tarball_filename);
		if (tarball_fd == -1) {
			fprintf(stderr, "Couldn't open temporary tarball file: %s.\n", strerror(errno));
			goto do_error;
		}
	}

	// Recap (to stderr, in order not to mess stuff up if we output to stdout) what we're building
	// Again, a signed userdata package is the ugly duckling...
	if (userdata_only) {
		fprintf(stderr,
			"Building userdata package '%s' directly from '%s' (signed with cert %u).\n",
			output_filename,
			tarball_filename,
			(uint32_t) info.certificate_number);
	} else {
		fprintf(stderr,
			"Building %s%s%s (%.*s) update package '%s'%s%s%s%s for",
			(legacy ? "(in legacy mode) " : ""),
			(fake_sign ? "fake " : ""),
			(convert_bundle_version(info.version)),
			MAGIC_NUMBER_LENGTH,
			info.magic_number,
			output_filename,
			(skip_archive ? " directly from " : ""),
			(skip_archive ? "'" : ""),
			(skip_archive ? tarball_filename : ""),
			(skip_archive ? "'" : ""));
		// If we have specific device IDs, list them
		if (info.num_devices > 0) {
			fprintf(stderr, " %hu device%s:\n", info.num_devices, (info.num_devices > 1 ? "s" : ""));
			// Loop over devices
			for (i = 0; i < info.num_devices; i++) {
				fprintf(stderr, "\t%s", convert_device_id(info.devices[i]));
				if (i != info.num_devices - 1) {
					fprintf(stderr, "\n");
				}
			}
			fprintf(stderr, "\n");
		} else {
			fprintf(stderr, " no specific device\n");
		}
		// Don't print settings not applicable to our update type...
		switch (info.version) {
			case OTAUpdateV2:
				if (info.target_revision == UINT64_MAX) {
					fprintf(
					    stderr,
					    "With the following flags: Min. OTA: %llu, Target OTA: MAX, Critical: %hhu, Cert: %u & %hu Metadata strings%s",
					    (long long unsigned int) info.source_revision,
					    info.critical,
					    (uint32_t) info.certificate_number,
					    info.num_meta,
					    (info.num_meta ? " (" : ".\n"));
				} else {
					fprintf(
					    stderr,
					    "With the following flags: Min. OTA: %llu, Target OTA: %llu, Critical: %hhu, Cert: %u & %hu Metadata strings%s",
					    (long long unsigned int) info.source_revision,
					    (long long unsigned int) info.target_revision,
					    info.critical,
					    (uint32_t) info.certificate_number,
					    info.num_meta,
					    (info.num_meta ? " (" : ".\n"));
				}
				// Loop over meta
				for (i = 0; i < info.num_meta; i++) {
					fprintf(stderr, "%s", info.metastrings[i]);
					if (i != info.num_meta - 1) {
						fprintf(stderr, "; ");
					} else {
						fprintf(stderr, ").\n");
					}
				}
				break;
			case OTAUpdate:
				if (info.target_revision == UINT32_MAX) {
					fprintf(
					    stderr,
					    "With the following flags: Min. OTA: %llu, Target OTA: MAX, Optional: %hhu.\n",
					    (long long unsigned int) info.source_revision,
					    info.optional);
				} else {
					fprintf(
					    stderr,
					    "With the following flags: Min. OTA: %llu, Target OTA: %llu, Optional: %hhu.\n",
					    (long long unsigned int) info.source_revision,
					    (long long unsigned int) info.target_revision,
					    info.optional);
				}
				break;
			case RecoveryUpdate:
				fprintf(stderr,
					"With the following flags: Minor: %u, Magic 1: %u, Magic 2: %u",
					info.minor,
					info.magic_1,
					info.magic_2);
				if (memcmp(info.magic_number, "FB02", MAGIC_NUMBER_LENGTH) == 0 && info.header_rev == 2) {
					if (info.target_revision == UINT64_MAX) {
						fprintf(stderr,
							", Header Rev: %u, Target OTA: MAX, Platform: %s, Board: %s.\n",
							info.header_rev,
							convert_platform_id(info.platform),
							convert_board_id(info.board));
					} else {
						fprintf(stderr,
							", Header Rev: %u, Target OTA: %llu, Platform: %s, Board: %s.\n",
							info.header_rev,
							(long long unsigned int) info.target_revision,
							convert_platform_id(info.platform),
							convert_board_id(info.board));
					}
				} else {
					fprintf(stderr, ".\n");
				}
				break;
			case RecoveryUpdateV2:
				fprintf(stderr, "With the following flags:");
				if (info.target_revision == UINT64_MAX) {
					fprintf(stderr, " Target OTA: MAX");
				} else {
					fprintf(
					    stderr, " Target OTA: %llu", (long long unsigned int) info.target_revision);
				}
				fprintf(
				    stderr,
				    ", Minor: %u, Magic 1: %u, Magic 2: %u, Header Rev: %u, Cert: %u, Platform: %s, Board: %s.\n",
				    info.minor,
				    info.magic_1,
				    info.magic_2,
				    info.header_rev,
				    (uint32_t) info.certificate_number,
				    convert_platform_id(info.platform),
				    convert_board_id(info.board));
				break;
			case UnknownUpdate:
			default:
				fprintf(stderr, "\n\n!!!!\nUnknown update type, we shouldn't ever hit this!\n!!!!\n");
				break;
		}
	}

	// Create our package archive, sigfile & bundlefile included
	if (!skip_archive) {
		if (kindle_create_package_archive(
			tarball_fd, input_list, input_index, &info.sign_pkey, legacy, real_blocksize) != 0) {
			fprintf(stderr, "Failed to create intermediate archive '%s'.\n", tarball_filename);
			// Delete the borked files
			close(tarball_fd);
			unlink(tarball_filename);
			goto do_error;
		}
		// We opened it, we need to close it ;)
		close(tarball_fd);
	}

	// And finally, build our package :)
	if ((input = fopen(tarball_filename, "rb")) == NULL) {
		fprintf(stderr, "Cannot read input tarball '%s': %s.\n", tarball_filename, strerror(errno));
		goto do_error;
	}
	if (kindle_create(&info, input, output, fake_sign) < 0) {
		fprintf(stderr, "Cannot write update to output.\n");
		goto do_error;
	}

	// Cleanup
	for (ui = 0; ui < input_index; ui++) {
		free(input_list[ui]);
	}
	free(input_list);
	free(info.devices);
	for (i = 0; i < info.num_meta; i++) {
		free(info.metastrings[i]);
	}
	free(info.metastrings);
	rsa_private_key_clear(&info.sign_pkey);
	fclose(input);
	if (output != stdout) {
		fclose(output);
	}
	free(output_filename);
	// Remove tarball, unless we asked to keep it, or we used an existing tarball as sole input
	if (!keep_archive && !skip_archive) {
		unlink(tarball_filename);
	}
	free(tarball_filename);

	return 0;

do_error:
	if (input_index > 0) {
		for (ui = 0; ui < input_index; ui++) {
			free(input_list[ui]);
		}
		free(input_list);
	}
	free(output_filename);
	free(info.devices);
	for (i = 0; i < info.num_meta; i++) {
		free(info.metastrings[i]);
	}
	free(info.metastrings);
	rsa_private_key_clear(&info.sign_pkey);
	if (input != NULL) {
		fclose(input);
	}
	if (output != NULL && output != stdout) {
		fclose(output);
	}
	free(tarball_filename);
	return -1;
}
