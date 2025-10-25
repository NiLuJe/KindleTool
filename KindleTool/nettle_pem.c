/*
**  KindleTool, nettle_pem.c
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

#include "kindle_tool.h"

// This was pretty much just lifted straight off from nettle's tools/pkcs1-conv.c,
// Copyright (C) 2005, 2009 Niels Möller, Magnus Holmgren
// Copyright (C) 2014 Niels Möller
// with a very few tweaks to better suit our needs...
enum object_type
{
	RSA_PRIVATE_KEY = 0x200,
	RSA_PUBLIC_KEY,
	DSA_PRIVATE_KEY,
	GENERAL_PUBLIC_KEY,
};

/* Return 1 on success, 0 on error, -1 on eof */
static int
    read_line(struct nettle_buffer* buffer, FILE* f)
{
	int c;

	// Flawfinder: ignore
	while ((c = getc(f)) != EOF) {
		if (!NETTLE_BUFFER_PUTC(buffer, (uint8_t) c)) {
			return 0;
		}

		if (c == '\n') {
			return 1;
		}
	}
	if (ferror(f)) {
		fprintf(stderr, "Read failed: %s.\n", strerror(errno));
		return 0;
	} else {
		return -1;
	}
}

static int
    read_file(struct nettle_buffer* buffer, FILE* f)
{
	int c;

	// Flawfinder: ignore
	while ((c = getc(f)) != EOF) {
		if (!NETTLE_BUFFER_PUTC(buffer, (uint8_t) c)) {
			return 0;
		}
	}

	if (ferror(f)) {
		fprintf(stderr, "Read failed: %s.\n", strerror(errno));
		return 0;
	} else {
		return 1;
	}
}

static const uint8_t pem_start_pattern[11] __attribute__((nonstring)) = "-----BEGIN ";

static const uint8_t pem_end_pattern[9] __attribute__((nonstring)) = "-----END ";

static const uint8_t pem_trailer_pattern[5] __attribute__((nonstring)) = "-----";

static const char pem_ws[33] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,   /* \t, \n, \v, \f, \r */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /* SPC */
};

#define PEM_IS_SPACE(c) ((c) < sizeof(pem_ws) && pem_ws[(c)])

/* Returns 1 on match, otherwise 0. */
static int
    match_pem_start(size_t length, const uint8_t* line, size_t* marker_start, size_t* marker_length)
{
	while (length > 0 && PEM_IS_SPACE(line[length - 1])) {
		length--;
	}

	if (length > (sizeof(pem_start_pattern) + sizeof(pem_trailer_pattern)) &&
	    memcmp(line, pem_start_pattern, sizeof(pem_start_pattern)) == 0 &&
	    memcmp(line + length - sizeof(pem_trailer_pattern), pem_trailer_pattern, sizeof(pem_trailer_pattern)) == 0) {
		*marker_start  = 11;
		*marker_length = length - (sizeof(pem_start_pattern) + sizeof(pem_trailer_pattern));

		return 1;
	} else {
		return 0;
	}
}

/* Returns 1 on match, -1 if the line is of the right form except for
   the marker, otherwise 0. */
static int
    match_pem_end(size_t length, const uint8_t* line, size_t marker_length, const uint8_t* marker)
{
	while (length > 0 && PEM_IS_SPACE(line[length - 1])) {
		length--;
	}

	if (length > (sizeof(pem_end_pattern) + sizeof(pem_trailer_pattern)) &&
	    memcmp(line, pem_end_pattern, sizeof(pem_end_pattern)) == 0 &&
	    memcmp(line + length - sizeof(pem_trailer_pattern), pem_trailer_pattern, sizeof(pem_trailer_pattern)) == 0) {
		/* Right form. Check marker */
		if (length == marker_length + (sizeof(pem_end_pattern) + sizeof(pem_trailer_pattern)) &&
		    memcmp(line + sizeof(pem_end_pattern), marker, marker_length) == 0) {
			return 1;
		} else {
			return -1;
		}
	} else {
		return 0;
	}
}

struct pem_info
{
	/* The FOO part in "-----BEGIN FOO-----" */
	size_t marker_start;
	size_t marker_length;
	size_t data_start;
	size_t data_length;
};

static int
    read_pem(struct nettle_buffer* buffer, FILE* f, struct pem_info* info)
{
	/* Find start line */
	for (;;) {
		int res;

		nettle_buffer_reset(buffer);

		res = read_line(buffer, f);
		if (res != 1) {
			return res;
		}

		if (match_pem_start(buffer->size, buffer->contents, &info->marker_start, &info->marker_length)) {
			break;
		}
	}

	/* NUL-terminate the marker. Don't care to check for embedded NULs. */
	buffer->contents[info->marker_start + info->marker_length] = 0;

	info->data_start = buffer->size;

	for (;;) {
		size_t line_start = buffer->size;

		if (read_line(buffer, f) != 1) {
			return 0;
		}

		switch (match_pem_end(buffer->size - line_start,
				      buffer->contents + line_start,
				      info->marker_length,
				      buffer->contents + info->marker_start)) {
			case 0:
				break;
			case -1:
				fprintf(stderr, "PEM END line doesn't match BEGIN.\n");
				return 0;
			case 1:
				/* Return base 64 data; let caller do the decoding */
				info->data_length = line_start - info->data_start;
				return 1;
		}
	}
}

static inline int
    base64_decode_in_place(struct base64_decode_ctx* ctx, size_t* dst_length, size_t length, uint8_t* data)
{
	return base64_decode_update(ctx, dst_length, data, length, (const char*) data);
}

static int
    decode_base64(struct nettle_buffer* buffer, size_t start, size_t* length)
{
	struct base64_decode_ctx ctx;

	base64_decode_init(&ctx);

	/* Decode in place */
	if (base64_decode_in_place(&ctx, length, *length, buffer->contents + start) && base64_decode_final(&ctx)) {
		return 1;
	} else {
		fprintf(stderr, "Invalid base64 data.\n");
		return 0;
	}
}

static int
    convert_rsa_private_key(struct nettle_buffer*   buffer,
			    size_t                  length,
			    const uint8_t*          data,
			    struct rsa_private_key* rsa_pkey)
{
	struct rsa_public_key pub;
	int                   res;

	// NOTE: Unlike rsa_keypair_from_sexp, we *HAVE* to init the pubkey too, or everything blows up,
	//       the from_der codepath expects it to be setup...
	rsa_public_key_init(&pub);

	if (rsa_keypair_from_der(&pub, rsa_pkey, 0, length, data)) {
		nettle_buffer_reset(buffer);
		res = 1;
	} else {
		fprintf(stderr, "Invalid PKCS#1 private key.\n");
		res = 0;
	}

	rsa_public_key_clear(&pub);

	return res;
}

// NOTE: Destroys contents of buffer
//       Returns 1 on success, 0 on error, and -1 for unsupported algorithms.
static int
    convert_type(struct nettle_buffer*   buffer,
		 enum object_type        type,
		 size_t                  length,
		 const uint8_t*          data,
		 struct rsa_private_key* rsa_pkey)
{
	int res;

	switch (type) {
		default:
			fprintf(stderr, "Unsupported key type!\n");
			return -1;

		case RSA_PRIVATE_KEY:
			res = convert_rsa_private_key(buffer, length, data, rsa_pkey);
			break;
	}

	return res;
}

static int
    load_pem(struct nettle_buffer* buffer, FILE* f, struct rsa_private_key* rsa_pkey, enum object_type type, int base64)
{
	if (type) {
		read_file(buffer, f);
		if (base64 && !decode_base64(buffer, 0, &buffer->size)) {
			return 0;
		}

		if (convert_type(buffer, type, buffer->size, buffer->contents, rsa_pkey) != 1) {
			return 0;
		}

		return 1;
	} else {
		/* PEM processing */
		for (;;) {
			struct pem_info info;
			const uint8_t*  marker;

			nettle_buffer_reset(buffer);
			switch (read_pem(buffer, f, &info)) {
				default:
					return 0;
				case 1:
					break;
				case -1:
					/* EOF */
					return 1;
			}

			if (!decode_base64(buffer, info.data_start, &info.data_length)) {
				fprintf(stderr, "decode_base64 failed!\n");
				return 0;
			}

			marker = buffer->contents + info.marker_start;

			type = 0;
			switch (info.marker_length) {
				case 10:
					if (memcmp(marker, "PUBLIC KEY", 10) == 0) {
						type = GENERAL_PUBLIC_KEY;
					}
					break;
				case 14:
					if (memcmp(marker, "RSA PUBLIC KEY", 14) == 0) {
						type = RSA_PUBLIC_KEY;
					}
					break;
				case 15:
					if (memcmp(marker, "RSA PRIVATE KEY", 15) == 0) {
						type = RSA_PRIVATE_KEY;
					} else if (memcmp(marker, "DSA PRIVATE KEY", 15) == 0) {
						type = DSA_PRIVATE_KEY;
					}
					break;
			}

			if (!type) {
				fprintf(stderr, "Ignoring unsupported object type `%s'.\n", marker);
			} else if (convert_type(
				       buffer, type, info.data_length, buffer->contents + info.data_start, rsa_pkey) !=
				   1) {
				fprintf(stderr, "convert_type failed!\n");
				return 0;
			}
		}
	}
}

int
    nettle_rsa_privkey_from_pem(const char* pem_filename, struct rsa_private_key* rsa_pkey)
{
	struct nettle_buffer buffer;
	enum object_type     type   = 0;
	int                  base64 = 0;

	nettle_buffer_init_realloc(&buffer, NULL, nettle_xrealloc);

	const char* mode = (type || base64) ? "r" : "rb";

	FILE* f = fopen(pem_filename, mode);
	if (!f) {
		fprintf(stderr, "Failed to open `%s': %s.\n", pem_filename, strerror(errno));
		return EXIT_FAILURE;
	}

	if (!load_pem(&buffer, f, rsa_pkey, type, base64)) {
		fprintf(stderr, "load_pem failed!\n");
		return EXIT_FAILURE;
	}

	fclose(f);
	nettle_buffer_clear(&buffer);

	return EXIT_SUCCESS;
}
