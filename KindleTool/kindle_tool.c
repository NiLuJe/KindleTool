/*
**  KindleTool, kindle_tool.c
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

#include "kindle_main.h"
#include "kindle_table.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
// NOTE: Handle the rest of the Win32 tempfiles mess in a quick'n dirty way...
// Namely: - We couldn't use MinGW's mkstemp until 5.0 came out
//           (the implementation in 4.0.1 unlinks on close, which is unexpected)
//         - MSVCRT's tmpfile() creates files in the root drive,
//           which, as we've already mentioned, is a recipe for disaster...
// Whip crude hacks around both of these issues while staying oblivious to the wchar_t potential mess...

// Inspired from gnulib's tmpfile implementation (http://git.savannah.gnu.org/gitweb/?p=gnulib.git;a=blob;f=lib/tmpfile.c)
FILE*
    kt_win_tmpfile(void)
{
	char template[PATH_MAX];
	snprintf(template, PATH_MAX, "%s/%s", kt_tempdir, "kindletool_tmpfile_XXXXXX");
	int fd = -1;

	// Now, because the CRT's _mktemp is terrible (PID based), duplicate MinGW's mkstemp logic...
	// c.f., https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-crt/misc/mkstemp.c
	size_t index, len;

	/* These are the (62) characters used in temporary filenames. */
	static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	/* The last six characters of template must be "XXXXXX" */
	// Flawfinder: ignore
	if (template == NULL || (len = strlen(template)) < 6U || memcmp(template + (len - 6U), "XXXXXX", 6U)) {
		errno = EINVAL;
		fprintf(stderr, "Couldn't create temporary file template: %s.\n", strerror(errno));
		return NULL;
	}

	/* User may supply more than six trailing Xs */
	for (index = len - 6U; index > 0U && template[index - 1U] == 'X'; index--)
		;

	/*
	Like OpenBSD, mkstemp() will try at least 2 ** 31 combinations before
	giving up.
	*/
	for (int i = 0; i >= 0; i++) {
		for (size_t j = index; j < len; j++) {
			template[j] = letters[rand() % 62];
		}
		// NOTE: And we can't use mkstemp directly, because we *DO* want O_TEMPORARY here...
		fd = _sopen(
		    template, _O_RDWR | _O_CREAT | _O_EXCL | _O_TEMPORARY | _O_BINARY, _SH_DENYRW, _S_IREAD | _S_IWRITE);
		if (fd != -1) {
			// Success!
			break;
		}
		if (fd == -1 && errno != EEXIST) {
			// Failure, we'll handle cleanup outside...
			break;
		}
	}

	// And we're back!
	if (fd == -1) {
		fprintf(stderr, "Couldn't open temporary file: %s.\n", strerror(errno));
		return NULL;
	}
	FILE* fp = _fdopen(fd, "w+b");
	if (fp != NULL) {
		return fp;
	} else {
		// We need to close the fd ourselves in case of error, since our own code expects a FP, not an fd...
		// Which means we have to fudge errno to keep the one from fdopen...
		int saved_errno = errno;
		_close(fd);
		errno = saved_errno;
	}
	return NULL;
}
#endif

void
    md(unsigned char* bytes, size_t length)
{
	for (size_t i = 0; i < length; ++i) {
		bytes[i] = (unsigned char) ptog[bytes[i]];
	}
}

void
    dm(unsigned char* bytes, size_t length)
{
	for (size_t i = 0; i < length; ++i) {
		bytes[i] = (unsigned char) gtop[bytes[i]];
	}
}

int
    munger(FILE* input, FILE* output, size_t length, const bool fake_sign)
{
	unsigned char bytes[BUFFER_SIZE];
	size_t        bytes_read;
	size_t        bytes_written;

	while ((bytes_read = fread(
		    bytes, sizeof(unsigned char), (length < BUFFER_SIZE && length > 0 ? length : BUFFER_SIZE), input)) >
	       0) {
		// Don't munge if we asked for a fake package
		if (!fake_sign) {
			md(bytes, bytes_read);
		}
		bytes_written = fwrite(bytes, sizeof(unsigned char), bytes_read, output);
		if (ferror(output) != 0) {
			fprintf(stderr, "Error munging, cannot write to output: %s.\n", strerror(errno));
			return -1;
		} else if (bytes_written < bytes_read) {
			fprintf(stderr,
				"Error munging, read %zu bytes but only wrote %zu bytes.\n",
				bytes_read,
				bytes_written);
			return -1;
		}
		length -= bytes_read;
	}
	if (ferror(input) != 0) {
		fprintf(stderr, "Error munging, cannot read input: %s.\n", strerror(errno));
		return -1;
	}

	return 0;
}

int
    demunger(FILE* input, FILE* output, size_t length, const bool fake_sign)
{
	unsigned char bytes[BUFFER_SIZE];
	size_t        bytes_read;
	size_t        bytes_written;

	while ((bytes_read = fread(
		    bytes, sizeof(unsigned char), (length < BUFFER_SIZE && length > 0 ? length : BUFFER_SIZE), input)) >
	       0) {
		// Don't demunge if we supplied a fake package
		if (!fake_sign) {
			dm(bytes, bytes_read);
		}
		bytes_written = fwrite(bytes, sizeof(unsigned char), bytes_read, output);
		if (ferror(output) != 0) {
			fprintf(stderr, "Error demunging, cannot write to output: %s.\n", strerror(errno));
			return -1;
		} else if (bytes_written < bytes_read) {
			fprintf(stderr,
				"Error demunging, read %zu bytes but only wrote %zu bytes.\n",
				bytes_read,
				bytes_written);
			return -1;
		}
		length -= bytes_read;
	}
	if (ferror(input) != 0) {
		fprintf(stderr, "Error demunging, cannot read input: %s.\n", strerror(errno));
		return -1;
	}

	return 0;
}

const char*
    convert_device_id(Device dev)
{
	switch (dev) {
		case Kindle1:
			return "Kindle 1";
		case Kindle2US:
			return "Kindle 2 US";
		case Kindle2International:
			return "Kindle 2 International";
		case KindleDXUS:
			return "Kindle DX US";
		case KindleDXInternational:
			return "Kindle DX International";
		case KindleDXGraphite:
			return "Kindle DX Graphite";
		case Kindle3WiFi:
			return "Kindle 3 WiFi";
		case Kindle3WiFi3G:
			return "Kindle 3 WiFi+3G";
		case Kindle3WiFi3GEurope:
			return "Kindle 3 WiFi+3G Europe";
		case Kindle4NonTouch:
			return "Silver Kindle 4 Non-Touch (2011)";
		case Kindle5TouchWiFi:
			return "Kindle 5 Touch WiFi";
		case Kindle5TouchWiFi3G:
			return "Kindle 5 Touch WiFi+3G";
		case Kindle5TouchWiFi3GEurope:
			return "Kindle 5 Touch WiFi+3G Europe";
		case Kindle5TouchUnknown:
			return "Kindle 5 Touch (Unknown Variant)";
		case Kindle4NonTouchBlack:
			return "Black Kindle 4 Non-Touch (2012)";
		case KindlePaperWhiteWiFi:
			return "Kindle PaperWhite WiFi";
		case KindlePaperWhiteWiFi3G:
			return "Kindle PaperWhite WiFi+3G";
		case KindlePaperWhiteWiFi3GCanada:
			return "Kindle PaperWhite WiFi+3G Canada";
		case KindlePaperWhiteWiFi3GEurope:
			return "Kindle PaperWhite WiFi+3G Europe";
		case KindlePaperWhiteWiFi3GJapan:
			return "Kindle PaperWhite WiFi+3G Japan";
		case KindlePaperWhiteWiFi3GBrazil:
			return "Kindle PaperWhite WiFi+3G Brazil";
		case KindlePaperWhite2WiFi:
			return "Kindle PaperWhite 2 (2013) WiFi";
		case KindlePaperWhite2WiFiJapan:
			return "Kindle PaperWhite 2 (2013) WiFi Japan";
		case KindlePaperWhite2WiFi3G:
			return "Kindle PaperWhite 2 (2013) WiFi+3G";
		case KindlePaperWhite2WiFi3GCanada:
			return "Kindle PaperWhite 2 (2013) WiFi+3G Canada";
		case KindlePaperWhite2WiFi3GEurope:
			return "Kindle PaperWhite 2 (2013) WiFi+3G Europe";
		case KindlePaperWhite2WiFi3GRussia:
			return "Kindle PaperWhite 2 (2013) WiFi+3G Russia";
		case KindlePaperWhite2WiFi3GJapan:
			return "Kindle PaperWhite 2 (2013) WiFi+3G Japan";
		case KindlePaperWhite2WiFi4GBInternational:
			return "Kindle PaperWhite 2 (2013) WiFi (4GB) International";
		case KindlePaperWhite2WiFi3G4GBEurope:
			return "Kindle PaperWhite 2 (2013) WiFi+3G (4GB) Europe";
		case KindlePaperWhite2Unknown_0xF4:
			return "Kindle PaperWhite 2 (2013) (Unknown Variant 0xF4)";
		case KindlePaperWhite2Unknown_0xF9:
			return "Kindle PaperWhite 2 (2013) (Unknown Variant 0xF9)";
		case KindlePaperWhite2WiFi3G4GB:
			return "Kindle PaperWhite 2 (2013) WiFi+3G (4GB)";
		case KindlePaperWhite2WiFi3G4GBBrazil:
			return "Kindle PaperWhite 2 (2013) WiFi+3G (4GB) Brazil";
		case KindlePaperWhite2WiFi3G4GBCanada:
			return "Kindle PaperWhite 2 (2013) WiFi+3G (4GB) Canada";
		case KindleBasic:
			return "Kindle Basic (2014)";
		case KindleVoyageWiFi:
			return "Kindle Voyage WiFi";
		case ValidKindleUnknown_0x16:
			return "Unknown Kindle (0x16)";
		case ValidKindleUnknown_0x21:
			return "Unknown Kindle (0x21)";
		case KindleVoyageWiFi3G:
			return "Kindle Voyage WiFi+3G";
		case KindleVoyageWiFi3GJapan:
			return "Kindle Voyage WiFi+3G Japan";
		case KindleVoyageWiFi3G_0x4F:
			return "Kindle Voyage WiFi+3G (Variant 0x4F)";
		case KindleVoyageWiFi3GMexico:
			return "Kindle Voyage WiFi+3G Mexico";
		case KindleVoyageWiFi3GEurope:
			return "Kindle Voyage WiFi+3G Europe";
		case ValidKindleUnknown_0x07:
			return "Unknown Kindle (0x07)";
		case ValidKindleUnknown_0x0B:
			return "Unknown Kindle (0x0B)";
		case ValidKindleUnknown_0x0C:
			return "Unknown Kindle (0x0C)";
		case ValidKindleUnknown_0x0D:
			return "Unknown Kindle (0x0D)";
		case ValidKindleUnknown_0x99:
			return "Unknown Kindle (0x99)";
		case KindleBasicKiwi:
			return "Kindle Basic (2014) Australia";
		case KindlePaperWhite3WiFi:
			return "Kindle PaperWhite 3 (2015) WiFi";
		case KindlePaperWhite3WiFi3G:
			return "Kindle PaperWhite 3 (2015) WiFi+3G";
		case KindlePaperWhite3WiFi3GMexico:
			return "Kindle PaperWhite 3 (2015) WiFi+3G Mexico";
		case KindlePaperWhite3WiFi3GEurope:
			return "Kindle PaperWhite 3 (2015) WiFi+3G Europe";
		case KindlePaperWhite3WiFi3GCanada:
			return "Kindle PaperWhite 3 (2015) WiFi+3G Canada";
		case KindlePaperWhite3WiFi3GJapan:
			return "Kindle PaperWhite 3 (2015) WiFi+3G Japan";
		case KindlePaperWhite3WhiteWiFi:
			return "White Kindle PaperWhite 3 (2016) WiFi";
		case KindlePaperWhite3WhiteWiFi3GJapan:
			return "White Kindle PaperWhite 3 (2016) WiFi+3G Japan";
		case KindlePW3WhiteUnknown_0KD:
			return "White Kindle PaperWhite 3 (Unknown Variant 0KD)";
		case KindlePaperWhite3WhiteWiFi3GInternational:
			return "White Kindle PaperWhite 3 (2016) WiFi+3G International";
		case KindlePaperWhite3WhiteWiFi3GInternationalBis:
			return "White Kindle PaperWhite 3 (2016) WiFi+3G International (Bis)";
		case KindlePW3WhiteUnknown_0KG:
			return "White Kindle PaperWhite 3 (Unknown Variant 0KG)";
		case KindlePaperWhite3BlackWiFi32GBJapan:
			return "Kindle PaperWhite 3 (2016) WiFi (32GB) Japan";
		case KindlePaperWhite3WhiteWiFi32GBJapan:
			return "White Kindle PaperWhite 3 (2016) WiFi (32GB) Japan";
		case KindlePW3Unknown_TTT:
			return "Kindle PaperWhite 3 (2016) (Unknown Variant TTT)";
		case KindleOasisWiFi:
			return "Kindle Oasis WiFi";
		case KindleOasisWiFi3G:
			return "Kindle Oasis WiFi+3G";
		case KindleOasisWiFi3GInternational:
			return "Kindle Oasis WiFi+3G International";
		case KindleOasisUnknown_0GS:
			return "Kindle Oasis (Unknown Variant 0GS)";
		case KindleOasisWiFi3GChina:
			return "Kindle Oasis WiFi+3G China";
		case KindleOasisWiFi3GEurope:
			return "Kindle Oasis WiFi+3G Europe";
		case KindleBasic2White:
			return "White Kindle Basic 2 (2016)";
		case KindleBasic2:
			return "Kindle Basic 2 (2016)";
		case KindleBasic2Unknown_0DU:
			return "Kindle Basic 2 (2016) (Unknown Variant 0DU)";
		case KindleOasis2Unknown_0LM:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0LM)";
		case KindleOasis2Unknown_0LN:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0LN)";
		case KindleOasis2Unknown_0LP:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0LP)";
		case KindleOasis2Unknown_0LQ:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0LQ)";
		case KindleOasis2WiFi32GBChampagne:
			return "Champagne Kindle Oasis 2 (2017) WiFi (32GB)";
		case KindleOasis2Unknown_0P2:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0P2)";
		case KindleOasis2Unknown_0P6:
			return "Kindle Oasis 2 (2017) WiFi+3G (32GB) (Variant 0P6)";
		case KindleOasis2Unknown_0P7:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0P7)";
		case KindleOasis2WiFi8GB:
			return "Kindle Oasis 2 (2017) WiFi (8GB)";
		case KindleOasis2WiFi3G32GB:
			return "Kindle Oasis 2 (2017) WiFi+3G (32GB)";
		case KindleOasis2WiFi3G32GBEurope:
			return "Kindle Oasis 2 (2017) WiFi+3G (32GB) Europe";
		case KindleOasis2Unknown_0S3:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0S3)";
		case KindleOasis2Unknown_0S4:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0S4)";
		case KindleOasis2Unknown_0S7:
			return "Kindle Oasis 2 (2017) (Unknown Variant 0S7)";
		case KindleOasis2WiFi32GB:
			return "Kindle Oasis 2 (2017) WiFi (32GB)";
		case KindlePaperWhite4WiFi8GB:
			return "Kindle PaperWhite 4 (2018) WiFi (8GB)";
		case KindlePaperWhite4WiFi4G32GB:
			return "Kindle PaperWhite 4 (2018) WiFi+4G (32GB)";
		case KindlePaperWhite4WiFi4G32GBEurope:
			return "Kindle PaperWhite 4 (2018) WiFi+4G (32GB) Europe";
		case KindlePaperWhite4WiFi4G32GBJapan:
			return "Kindle PaperWhite 4 (2018) WiFi+4G (32GB) Japan";
		case KindlePaperWhite4Unknown_0T4:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0T4)";
		case KindlePaperWhite4Unknown_0T5:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0T5)";
		case KindlePaperWhite4WiFi32GB:
			return "Kindle PaperWhite 4 (2018) WiFi (32GB)";
		case KindlePaperWhite4Unknown_0T7:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0T7)";
		case KindlePaperWhite4Unknown_0TJ:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0TJ)";
		case KindlePaperWhite4Unknown_0TK:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0TK)";
		case KindlePaperWhite4Unknown_0TL:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0TL)";
		case KindlePaperWhite4Unknown_0TM:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0TM)";
		case KindlePaperWhite4Unknown_0TN:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0TN)";
		case KindlePaperWhite4WiFi8GBIndia:
			return "Kindle PaperWhite 4 (2018) WiFi (8GB) India";
		case KindlePaperWhite4WiFi32GBIndia:
			return "Kindle PaperWhite 4 (2018) WiFi (32GB) India";
		case KindlePaperWhite4WiFi32GBBlue:
			return "Twilight Blue Kindle PaperWhite 4 (2018) WiFi (32GB)";
		case KindlePaperWhite4WiFi32GBPlum:
			return "Plum Kindle PaperWhite 4 (2018) WiFi (32GB)";
		case KindlePaperWhite4WiFi32GBSage:
			return "Sage Kindle PaperWhite 4 (2018) WiFi (32GB)";
		case KindlePaperWhite4WiFi8GBBlue:
			return "Twilight Blue Kindle PaperWhite 4 (2018) WiFi (8GB)";
		case KindlePaperWhite4WiFi8GBPlum:
			return "Plum Kindle PaperWhite 4 (2018) WiFi (8GB)";
		case KindlePaperWhite4WiFi8GBSage:
			return "Sage Kindle PaperWhite 4 (2018) WiFi (8GB)";
		case KindlePW4Unknown_0PL:
			return "Kindle PaperWhite 4 (2018) (Unknown Variant 0PL)";
		case KindleBasic3:
			return "Kindle Basic 3 (2019)";
		case KindleBasic3White8GB:
			return "White Kindle Basic 3 (2019) (8GB)";
		case KindleBasic3Unknown_0WG:
			return "Kindle Basic 3 (2019) (Unknown Variant 0WG)";
		case KindleBasic3White:
			return "White Kindle Basic 3 (2019)";
		case KindleBasic3Unknown_0WJ:
			return "Kindle Basic 3 (2019) (Unknown Variant 0WJ)";
		case KindleBasic3KidsEdition:
			return "Kindle Basic 3 (2019) Kids Edition";
		case KindleOasis3WiFi32GBChampagne:
			return "Champagne Kindle Oasis 3 (2019) WiFi (32GB)";
		case KindleOasis3WiFi4G32GBJapan:
			return "Kindle Oasis 3 (2019) WiFi+4G (32GB) Japan";
		case KindleOasis3WiFi4G32GBIndia:
			return "Kindle Oasis 3 (2019) WiFi+4G (32GB) India";
		case KindleOasis3WiFi4G32GB:
			return "Kindle Oasis 3 (2019) WiFi+4G (32GB)";
		case KindleOasis3WiFi32GB:
			return "Kindle Oasis 3 (2019) WiFi (32GB)";
		case KindleOasis3WiFi8GB:
			return "Kindle Oasis 3 (2019) WiFi (8GB)";
		case KindlePaperWhite5SignatureEdition:
			return "Kindle PaperWhite 5 Signature Edition (2021)";
		case KindlePaperWhite5Unknown_1Q0:
			return "Kindle PaperWhite 5 (2021) (Unknown Variant 1Q0)";
		case KindlePaperWhite5:
			return "Kindle PaperWhite 5 (2021)";
		case KindlePaperWhite5Unknown_1VD:
			return "Kindle PaperWhite 5 (2021) (Unknown Variant 1VD)";
		case KindlePaperWhite5SE_219:
			return "Kindle PaperWhite 5 Signature Edition (2021) (Variant 219)";
		case KindlePaperWhite5_21A:
			return "Kindle PaperWhite 5 (2021) (Variant 21A)";
		case KindlePaperWhite5SE_2BH:
			return "Kindle PaperWhite 5 Signature Edition (2021) (Variant 2BH)";
		case KindlePaperWhite5Unknown_2BJ:
			return "Kindle PaperWhite 5 (2021) (Unknown Variant 2BJ)";
		case KindlePaperWhite5_2DK:
			return "Kindle PaperWhite 5 (2021) (Variant 2DK)";
		case KindleBasic4Unknown_22D:
			return "Kindle Basic 4 (2022) (Unknown Variant 22D)";
		case KindleBasic4Unknown_25T:
			return "Kindle Basic 4 (2022) (Unknown Variant 25T)";
		case KindleBasic4Unknown_23A:
			return "Kindle Basic 4 (2022) (Unknown Variant 23A)";
		case KindleBasic4_2AQ:
			return "Kindle Basic 4 (2022) (Variant 2AQ)";
		case KindleBasic4_2AP:
			return "Kindle Basic 4 (2022) (Variant 2AP)";
		case KindleBasic4Unknown_1XH:
			return "Kindle Basic 4 (2022) (Unknown Variant 1XH)";
		case KindleBasic4Unknown_22C:
			return "Kindle Basic 4 (2022) (Unknown Variant 22C)";
		case KindleScribeUnknown_27J:
			return "Kindle Scribe (Unknown Variant 27J)";
		case KindleScribeUnknown_2BL:
			return "Kindle Scribe (Unknown Variant 2BL)";
		case KindleScribeUnknown_263:
			return "Kindle Scribe (Unknown Variant 263)";
		case KindleScribe16GB_227:
			return "Kindle Scribe (16GB) (Variant 227)";
		case KindleScribeUnknown_2BM:
			return "Kindle Scribe (Unknown Variant 2BM)";
		case KindleScribe_23L:
			return "Kindle Scribe (Variant 23L)";
		case KindleScribe64GB_23M:
			return "Kindle Scribe (64GB) (Variant 23M)";
		case KindleScribeUnknown_270:
			return "Kindle Scribe (Unknown Variant 270)";
		case KindleBasic5Unknown_3L5:
			return "Kindle Basic 5 (2024) (Unknown Variant 3L5)";
		case KindleBasic5Unknown_3L6:
			return "Kindle Basic 5 (2024) (Unknown Variant 3L6)";
		case KindleBasic5Unknown_3L4:
			return "Kindle Basic 5 (2024) (Unknown Variant 3L4)";
		case KindleBasic5Unknown_3L3:
			return "Kindle Basic 5 (2024) (Unknown Variant 3L3)";
		case KindleBasic5Unknown_A89:
			return "Kindle Basic 5 (2024) (Unknown Variant A89)";
		case KindleBasic5Unknown_3L2:
			return "Kindle Basic 5 (2024) (Unknown Variant 3L2)";
		case KindleBasic5Unknown_3KM:
			return "Kindle Basic 5 (2024) (Unknown Variant 3KM)";
		case KindlePaperWhite6Unknown_349:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 349)";
		case KindlePaperWhite6Unknown_346:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 346)";
		case KindlePaperWhite6Unknown_33X:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 33X)";
		case KindlePaperWhite6Unknown_33W:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 33W)";
		case KindlePaperWhite6Unknown_3HA:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3HA)";
		case KindlePaperWhite6Unknown_3H5:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3H5)";
		case KindlePaperWhite6Unknown_3H3:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3H3)";
		case KindlePaperWhite6Unknown_3H8:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3H8)";
		case KindlePaperWhite6Unknown_3J5:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3J5)";
		case KindlePaperWhite6Unknown_3JS:
			return "Kindle PaperWhite 6 (2024) (Unknown Variant 3JS)";
		case KindleScribe2Unknown_3V0:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3V0)";
		case KindleScribe2Unknown_3V1:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3V1)";
		case KindleScribe2Unknown_3X5:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3X5)";
		case KindleScribe2Unknown_3UV:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3UV)";
		case KindleScribe2Unknown_3X4:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3X4)";
		case KindleScribe2Unknown_3X3:
			return "Kindle Scribe 2 (2024) (Unknown Variant 3X3)";
		case KindleScribe2Unknown_41E:
			return "Kindle Scribe 2 (2024) (Unknown Variant 41E)";
		case KindleScribe2Unknown_41D:
			return "Kindle Scribe 2 (2024) (Unknown Variant 41D)";
		case KindleColorSoftUnknown_3H9:
			return "Kindle ColorSoft (2024) (Unknown Variant 3H9)";
		case KindleColorSoftUnknown_3H4:
			return "Kindle ColorSoft (2024) (Unknown Variant 3H4)";
		case KindleColorSoftUnknown_3HB:
			return "Kindle ColorSoft (2024) (Unknown Variant 3HB)";
		case KindleColorSoftUnknown_3H6:
			return "Kindle ColorSoft (2024) (Unknown Variant 3H6)";
		case KindleColorSoftUnknown_3H2:
			return "Kindle ColorSoft (2024) (Unknown Variant 3H2)";
		case KindleColorSoftUnknown_34X:
			return "Kindle ColorSoft (2024) (Unknown Variant 34X)";
		case KindleColorSoftUnknown_3H7:
			return "Kindle ColorSoft (2024) (Unknown Variant 3H7)";
		case KindleColorSoftUnknown_3JT:
			return "Kindle ColorSoft (2024) (Unknown Variant 3JT)";
		case KindleColorSoftUnknown_3J6:
			return "Kindle ColorSoft (2024) (Unknown Variant 3J6)";
		case KindleColorSoftUnknown_456:
			return "Kindle ColorSoft (2024) (Unknown Variant 456)";
		case KindleUnknown:
		default:
			return "Unknown";
	}
}

const char*
    convert_platform_id(Platform plat)
{
	switch (plat) {
		case Plat_Unspecified:
			return "Unspecified";
		case MarioDeprecated:
			return "Mario (Deprecated)";
		case Luigi:
			return "Luigi";
		case Banjo:
			return "Banjo";
		case Yoshi:
			return "Yoshi";
		case YoshimeProto:
			return "Yoshime (Prototype)";
		case Yoshime:
			return "Yoshime (Yoshime3)";
		case Wario:
			return "Wario";
		case Duet:
			return "Duet";
		case Heisenberg:
			return "Heisenberg";
		case Zelda:
			return "Zelda";
		case Rex:
			return "Rex";
		case Bellatrix:
			return "Bellatrix";
		case Bellatrix3:
			return "Bellatrix3";
		case Bellatrix4:
			return "Bellatrix4";
		default:
			return "Unknown";
	}
}

const char*
    convert_board_id(Board board)
{
	switch (board) {
		case Board_Unspecified:
			return "Unspecified";
		case Tequila:
			return "Tequila";
		case Whitney:
			return "Whitney";
		default:
			return "Unknown";
	}
}

BundleVersion
    get_bundle_version(const char magic_number[MAGIC_NUMBER_LENGTH])
{
	if (!memcmp(magic_number, "FB02", MAGIC_NUMBER_LENGTH) || !memcmp(magic_number, "FB01", MAGIC_NUMBER_LENGTH)) {
		return RecoveryUpdate;
	} else if (!memcmp(magic_number, "FB03", MAGIC_NUMBER_LENGTH)) {
		return RecoveryUpdateV2;
	} else if (!memcmp(magic_number, "FC02", MAGIC_NUMBER_LENGTH) ||
		   !memcmp(magic_number, "FD03", MAGIC_NUMBER_LENGTH)) {
		return OTAUpdate;
	} else if (!memcmp(magic_number, "FC04", MAGIC_NUMBER_LENGTH) ||
		   !memcmp(magic_number, "FD04", MAGIC_NUMBER_LENGTH) ||
		   !memcmp(magic_number, "FL01", MAGIC_NUMBER_LENGTH)) {
		return OTAUpdateV2;
	} else if (!memcmp(magic_number, "SP01", MAGIC_NUMBER_LENGTH)) {
		return UpdateSignature;
	} else if (!memcmp(magic_number, "\x1F\x8B\x08\x00", MAGIC_NUMBER_LENGTH)) {    // GZIP magic number
		return UserDataPackage;
	} else if (!memcmp(magic_number, "\x50\x4B\x03\x04", MAGIC_NUMBER_LENGTH)) {    // ZIP magic number
		return AndroidUpdate;
	} else if (!memcmp(magic_number, "CB01", MAGIC_NUMBER_LENGTH)) {
		return ComponentUpdate;
	} else {
		return UnknownUpdate;
	}
}

int
    md5_sum(FILE* input, char output_string[BASE16_ENCODE_LENGTH(MD5_DIGEST_SIZE)])
{
	unsigned char  bytes[BUFFER_SIZE];
	size_t         bytes_read;
	struct md5_ctx md5;
	uint8_t        digest[MD5_DIGEST_SIZE];

	md5_init(&md5);
	while ((bytes_read = fread(bytes, sizeof(unsigned char), BUFFER_SIZE, input)) > 0) {
		md5_update(&md5, bytes_read, bytes);
	}
	if (ferror(input) != 0) {
		fprintf(stderr, "Error reading input file: %s.\n", strerror(errno));
		return -1;
	}
	md5_digest(&md5, MD5_DIGEST_SIZE, digest);
	// And build the hex checksum the nettle way ;)
	base16_encode_update(output_string, MD5_DIGEST_SIZE, digest);

	return 0;
}

int
    sha256_sum(FILE* input, char output_string[BASE16_ENCODE_LENGTH(SHA256_DIGEST_SIZE)])
{
	unsigned char     bytes[BUFFER_SIZE];
	size_t            bytes_read;
	struct sha256_ctx sha256;
	uint8_t           digest[SHA256_DIGEST_SIZE];

	sha256_init(&sha256);
	while ((bytes_read = fread(bytes, sizeof(unsigned char), BUFFER_SIZE, input)) > 0) {
		sha256_update(&sha256, bytes_read, bytes);
	}
	if (ferror(input) != 0) {
		fprintf(stderr, "Error reading input file: %s.\n", strerror(errno));
		return -1;
	}
	sha256_digest(&sha256, SHA256_DIGEST_SIZE, digest);
	// And build the hex checksum the nettle way ;)
	base16_encode_update(output_string, SHA256_DIGEST_SIZE, digest);

	return 0;
}

static int
    kindle_print_help(const char* prog_name)
{
	printf(
	    "Usage:\n"
	    "  %s md [ <input> ] [ <output> ]\n"
	    "    Obfuscates data using Amazon's update algorithm.\n"
	    "    If no input is provided, input from stdin\n"
	    "    If no output is provided, output to stdout\n"
	    "    \n"
	    "  %s dm [ <input> ] [ <output> ]\n"
	    "    Deobfuscates data using Amazon's update algorithm.\n"
	    "    If no input is provided, input from stdin\n"
	    "    If no output is provided, output to stdout\n"
	    "    \n"
	    "  %s convert [options] <input>...\n"
	    "    Converts a Kindle update package to a gzipped tar archive file, and delete input.\n"
	    "    \n"
	    "    Options:\n"
	    "      -c, --stdout                Write to standard output, keeping original files unchanged.\n"
	    "      -i, --info                  Just print the package information, no conversion done.\n"
	    "      -s, --sig                   OTA V2, Recovery V2 & Recovery FB02 with header rev 2 updates only. Extract the payload signature.\n"
	    "      -k, --keep                  Don't delete the input package.\n"
	    "      -u, --unsigned              Assume input is an unsigned & mangled userdata package.\n"
	    "      -w, --unwrap                Just unwrap the package, if it's wrapped in an UpdateSignature header (especially useful for userdata packages).\n"
	    "      \n"
	    "  %s extract [options] <input> <output>\n"
	    "    Extracts a Kindle update package to a directory.\n"
	    "    \n"
	    "    Options:\n"
	    "      -u, --unsigned              Assume input is an unsigned & mangled userdata package.\n"
	    "      \n"
	    "  %s create <type> <devices> [options] <dir|file>... [ <output> ]\n"
	    "    Creates a Kindle update package.\n"
	    "    You should be able to throw a mix of files & directories as input without trouble.\n"
	    "    Just keep in mind that by default, if you feed it absolute paths, it will archive absolute paths, which usually isn't what you want!\n"
	    "    If input is a single gzipped tarball (\".tgz\" or \".tar.gz\") file, we assume it is properly packaged (bundlefile & sigfile), and will only convert it to an update.\n"
	    "    Output should be a file with the extension \".bin\", if it is not provided, or if it's a single dash, outputs to standard output.\n"
	    "    In case of OTA updates, all files with the extension \".ffs\" or \".sh\" will be treated as update scripts.\n"
	    "    \n"
	    "    Type:\n"
	    "      ota                         OTA V1 update package. Works on Kindle 3 and older.\n"
	    "      ota2                        OTA V2 signed update package. Works on Kindle 4 and newer.\n"
	    "      recovery                    Recovery package for restoring partitions.\n"
	    "      recovery2                   Recovery V2 package for restoring partitions. Works on FW >= 5.2 (PaperWhite) and newer.\n"
	    "      sig                         Signature envelope. Use this to build a signed userdata package with the -U switch (FW >= 5.1 only, but device agnostic).\n"
	    "    \n"
	    "    Devices:\n"
	    "      OTA V1 & Recovery packages only support one device. OTA V2 & Recovery V2 packages can support multiple devices.\n"
	    "      \n"
	    "      -d, --device k1             Kindle 1\n"
	    "      -d, --device k2             Kindle 2 US\n"
	    "      -d, --device k2i            Kindle 2 International\n"
	    "      -d, --device dx             Kindle DX US\n"
	    "      -d, --device dxi            Kindle DX International\n"
	    "      -d, --device dxg            Kindle DX Graphite\n"
	    "      -d, --device k3w            Kindle 3 WiFi\n"
	    "      -d, --device k3g            Kindle 3 WiFi+3G\n"
	    "      -d, --device k3gb           Kindle 3 WiFi+3G Europe\n"
	    "      -d, --device k4             Silver Kindle 4 (Non-Touch) (2011)\n"
	    "      -d, --device k4b            Black Kindle 4 (Non-Touch) (2012)\n"
	    "      -d, --device kindle2        Alias for k2 + k2i\n"
	    "      -d, --device kindledx       Alias for dx + dxi + dxg\n"
	    "      -d, --device kindle3        Alias for k3w + k3g + k3gb\n"
	    "      -d, --device legacy         Alias for kindle2 + kindledx + kindle3\n"
	    "      -d, --device kindle4        Alias for k4 + k4b\n"
	    "      -d, --device touch          Includes all known Kindle Touch variants\n"
	    "      -d, --device paperwhite     Includes all known Kindle PaperWhite 1 variants\n"
	    "      -d, --device paperwhite2    Includes all known Kindle PaperWhite 2 variants\n"
	    "      -d, --device basic          Includes all known Kindle Basic 1 variants\n"
	    "      -d, --device voyage         Includes all known Kindle Voyage variants\n"
	    "      -d, --device paperwhite3    Includes all known Kindle PaperWhite 3 variants\n"
	    "      -d, --device oasis          Includes all known Kindle Oasis 1 variants\n"
	    "      -d, --device basic2         Includes all known Kindle Basic 2 variants\n"
	    "      -d, --device oasis2         Includes all known Kindle Oasis 2 variants\n"
	    "      -d, --device paperwhite4    Includes all known Kindle PaperWhite 4 variants\n"
	    "      -d, --device basic3         Includes all known Kindle Basic 3 variants\n"
	    "      -d, --device oasis3         Includes all known Kindle Oasis 3 variants\n"
	    "      -d, --device paperwhite5    Includes all known Kindle PaperWhite 5 variants\n"
	    "      -d, --device basic4         Includes all known Kindle Basic 4 variants\n"
	    "      -d, --device scribe         Includes all known Kindle Scribe variants\n"
	    "      -d, --device basic5         Includes all known Kindle Basic 5 variants\n"
	    "      -d, --device paperwhite6    Includes all known Kindle PaperWhite 6 variants\n"
		"      -d, --device scribe2        Includes all known Kindle Scribe 2 variants\n"
	    "      -d, --device colorsoft      Includes all known Kindle ColorSoft variants\n"
		"      -d, --device kindle5        Alias for touch + paperwhite + paperwhite2 + basic + voyage + paperwhite3 + oasis + basic2 + oasis2 + paperwhite4 + basic3 + oasis3 + paperwhite5 + basic4 + scribe + basic5 + paperwhite6 + scribe2 + colorsoft\n"
	    "      -d, --device none           No specific device (Recovery V2 & Recovery FB02 with header rev 2 only, default).\n"
	    "      -d, --device auto           The current device (Obviously, has to be run from a Kindle).\n"
	    "      \n"
	    "    Platforms:\n"
	    "      Recovery V2 & recovery FB02 with header rev 2 updates only. Use a single platform per package.\n"
	    "      \n"
	    "      -p, --platform unspecified  Don't target a specific platform.\n"
	    "      -p, --platform mario        Mario (mostly devices shipped on FW 1.x?) [Deprecated].\n"
	    "      -p, --platform luigi        Luigi (mostly devices shipped on FW 2.x?).\n"
	    "      -p, --platform banjo        Banjo (devices shipped on FW 3.x?).\n"
	    "      -p, --platform yoshi        Yoshi (mostly devices shipped on FW <= 5.1).\n"
	    "      -p, --platform yoshime-p    Yoshime (Prototype).\n"
	    "      -p, --platform yoshime      Yoshime (Also known as Yoshime3, mostly devices shipped on FW >= 5.2).\n"
	    "      -p, --platform wario        Wario (mostly devices shipped on FW >= 5.4).\n"
	    "      -p, --platform duet         Duet (mostly devices shipped on FW >= 5.7).\n"
	    "      -p, --platform heisenberg   Heisenberg (mostly devices shipped on FW >= 5.8).\n"
	    "      -p, --platform zelda        Zelda (mostly devices shipped on FW >= 5.9).\n"
	    "      -p, --platform rex          Rex (mostly devices shipped on FW >= 5.10).\n"
	    "      -p, --platform bellatrix    Bellatrix (mostly devices shipped on FW >= 5.14).\n"
	    "      -p, --platform bellatrix3   Bellatrix3 (mostly devices shipped on FW >= 5.16).\n"
	    "      -p, --platform bellatrix4   Bellatrix4 (mostly devices shipped on FW >= 5.18).\n"
	    "      \n"
	    "    Boards:\n"
	    "      Recovery V2 & Recovery FB02 with header rev 2 updates only. Use a single board per package.\n"
	    "      \n"
	    "      -B, --board unspecified     Don't target a specific board, skip the device check.\n"
	    "      -B, --board tequila         Tequila (Kindle 4)\n"
	    "      -B, --board whitney         Whitney (Kindle Touch)\n"
	    "      \n"
	    "    Options:\n"
	    "      All the following options are optional and advanced.\n"
	    "      -k, --key <file>            PEM file containing RSA private key to sign update. Default is popular jailbreak key.\n"
	    "      -b, --bundle <type>         Manually specify package magic number. May override the value dictated by \"type\", if it makes sense. Valid bundle versions:\n"
	    "                                    FB01, FB02 = recovery; FB03 = recovery2; FC02, FD03 = ota; FC04, FD04, FL01 = ota2; SP01 = sig\n"
	    "      -s, --srcrev <ulong|uint>   OTA updates only. Source revision. OTA V1 uses uint, OTA V2 uses ulong.\n"
	    "                                    Lowest version of device that package supports. Default is 0.\n"
	    "                                    Also acccepts min for 0.\n"
	    "      -t, --tgtrev <ulong|uint>   OTA, Recovery V2 & Recovery FB02 with header rev 2 updates only. Target revision. OTA V1 & Recovery V1H2 uses uint, OTA V2 & Recovery V2 uses ulong.\n"
	    "                                    Highest version of device that package supports. Default is ulong/uint max value.\n"
	    "                                    Also acccepts max for the appropriate maximum value for the chosen update package type.\n"
	    "      -h, --hdrrev <uint>         Recovery V2 & Recovery FB02 updates only. Header Revision. Default is 0.\n"
	    "      -1, --magic1 <uint>         Recovery updates only. Magic number 1. Default is 0.\n"
	    "      -2, --magic2 <uint>         Recovery updates only. Magic number 2. Default is 0.\n"
	    "      -m, --minor <uint>          Recovery updates only. Minor number. Default is 0.\n"
	    "      -c, --cert <ushort>         OTA V2 & Recovery V2 updates only. The number of the certificate to use (found in /etc/uks on device). Default is 0.\n"
	    "                                    0 = pubdevkey01.pem, 1 = pubprodkey01.pem, 2 = pubprodkey02.pem\n"
	    "      -o, --opt <uchar>           OTA V1 updates only. One byte optional data expressed as a number. Default is 0.\n"
	    "      -r, --crit <uchar>          OTA V2 updates only. One byte optional data expressed as a number. Default is 0.\n"
	    "      -x, --meta <str>            OTA V2 updates only. An optional string to add. Multiple \"--meta\" options supported.\n"
	    "                                    Format of metastring must be: key=value\n"
	    "      -X, --packaging             OTA V2 updates only. Adds PackagedWith, PackagedBy & PackagedOn metastrings, storing packaging metadata.\n"
	    "      -a, --archive               Keep the intermediate archive.\n"
	    "      -u, --unsigned              Build an unsigned & mangled userdata package.\n"
	    "      -U, --userdata              Build an userdata package (can only be used with the sig update type).\n"
	    "      -O, --ota                   Build a versioned OTA bundle (can only be used with the ota2 update type).\n"
	    "      -C, --legacy                Emulate the behaviour of yifanlu's KindleTool regarding directories. By default, we behave like tar:\n"
	    "                                    every path passed on the commandline is stored as-is in the archive. This switch changes that, and store paths\n"
	    "                                    relative to the path passed on the commandline, like if we had chdir'ed into it.\n"
	    "      \n"
	    "  %s info <serialno>\n"
	    "    Get the default root password.\n"
	    "    Unless you changed your password manually, the first password shown will be the right one.\n"
	    "    (The Kindle defaults to DES hashed passwords, which are truncated to 8 characters).\n"
	    "    If you're looking for the recovery MMC export password, that's the second one.\n"
	    "    \n"
	    "  %s version\n"
	    "    Show some info about this KindleTool build.\n"
	    "    \n"
	    "  %s help\n"
	    "    Show this help screen.\n"
	    "    \n"
	    "Notices:\n"
	    "  1.  If the variable KT_WITH_UNKNOWN_DEVCODES is set in your environment (no matter the value), some device checks will be relaxed with the create command.\n"
	    "  2.  If the variable KT_PKG_METADATA_DUMP is set in your environment, convert will dump header info in a shell-friendly format in the file this variable points to.\n"
	    "  3.  Updates with meta-strings will probably fail to run when passed to 'Update Your Kindle'.\n"
	    "  4.  Currently, even though OTA V2 supports updates that run on multiple devices, it is not possible to create an update package that will run on both FW 4.x (Kindle 4) and FW 5.x (Basically everything since the Kindle Touch).\n",
	    prog_name,
	    prog_name,
	    prog_name,
	    prog_name,
	    prog_name,
	    prog_name,
	    prog_name,
	    prog_name);
	return 0;
}

static int
    kindle_print_version(const char* prog_name)
{
	printf("%s (KindleTool) %s built by %s with ", prog_name, KT_VERSION, KT_USERATHOST);
#ifdef __clang__
	printf("Clang %s ", __clang_version__);
#else
	printf("GCC %s ", __VERSION__);
#endif
	printf("on %s @ %s against %s ", __DATE__, __TIME__, ARCHIVE_VERSION_STRING);
	printf(
	    "& nettle %s\n",
	    NETTLE_VERSION);    // NOTE: This is completely custom, I couldn't find a way to get this info at buildtime in a saner way...
	return 0;
}

static int
    kindle_obfuscate_main(int argc, char* argv[])
{
	FILE* input;
	FILE* output;
	input  = stdin;
	output = stdout;

	// Skip command
	argv++;
	argc--;
	if (argc > 1) {
		if ((output = fopen(argv[1], "wb")) == NULL) {
			fprintf(stderr, "Cannot open output for writing: %s.\n", strerror(errno));
			return -1;
		}
	}
	if (argc > 0) {
		if ((input = fopen(argv[0], "rb")) == NULL) {
			fprintf(stderr, "Cannot open input for reading: %s.\n", strerror(errno));
			fclose(output);
			return -1;
		}
	}
	if (munger(input, output, 0, false) < 0) {
		fprintf(stderr, "Cannot obfuscate.\n");
		fclose(input);
		fclose(output);
		return -1;
	}
	fclose(input);
	fclose(output);
	return 0;
}

static int
    kindle_deobfuscate_main(int argc, char* argv[])
{
	FILE* input;
	FILE* output;
	input  = stdin;
	output = stdout;

	// Skip command
	argv++;
	argc--;
	if (argc > 1) {
		if ((output = fopen(argv[1], "wb")) == NULL) {
			fprintf(stderr, "Cannot open output for writing: %s.\n", strerror(errno));
			return -1;
		}
	}
	if (argc > 0) {
		if ((input = fopen(argv[0], "rb")) == NULL) {
			fprintf(stderr, "Cannot open input for reading: %s.\n", strerror(errno));
			fclose(output);
			return -1;
		}
	}
	if (demunger(input, output, 0, false) < 0) {
		fprintf(stderr, "Cannot deobfuscate.\n");
		fclose(input);
		fclose(output);
		return -1;
	}
	fclose(input);
	fclose(output);
	return 0;
}

static int
    kindle_info_main(int argc, char* argv[])
{
	char           serial_no[SERIAL_NO_LENGTH + 1] = { 0 };    // Leave an extra space for the LF...
	struct md5_ctx md5;
	uint8_t        digest[MD5_DIGEST_SIZE];
	char           hash[BASE16_ENCODE_LENGTH(MD5_DIGEST_SIZE)];
	char           device_code[3 + 1] = { 0 };
	Device         device;
	unsigned int   i;

	// Skip command
	argv++;
	argc--;
	if (argc < 1) {
		fprintf(stderr, "Missing argument. You must pass a serial number.\n");
		return -1;
	}
	// Don't manipulate argv directly, make a copy of it first...
	strncpy(serial_no, argv[0], SERIAL_NO_LENGTH);    // Flawfinder: ignore
	// Flawfinder: ignore
	if (strlen(serial_no) < SERIAL_NO_LENGTH || strlen(argv[0]) > SERIAL_NO_LENGTH) {
		fprintf(stderr,
			"Serial number must be composed of exactly 16 characters (without spaces). For example: %s\n",
			"B0NNXXXXXXXXXXXX");
		return -1;
	}
	// Make it fully uppercase
	for (i = 0; i < SERIAL_NO_LENGTH; i++) {
		if (islower((int) serial_no[i])) {
			serial_no[i] = (char) toupper((int) serial_no[i]);
		}
	}
	// We need to terminate the string with a LF, no matter the system (probably to match the procfs usid format)...
	serial_no[SERIAL_NO_LENGTH] = '\xA';
	// The root password is based on the MD5 hash of the S/N, so, hash it first.
	md5_init(&md5);
	md5_update(&md5, SERIAL_NO_LENGTH + 1, (uint8_t*) serial_no);
	md5_digest(&md5, MD5_DIGEST_SIZE, digest);
	base16_encode_update(hash, MD5_DIGEST_SIZE, digest);

	// And finally, do the device dance...
	// NOTE: If the S/N starts with B or 9, assume it's an older device with an hexadecimal device code
	if (serial_no[0] == 'B' || serial_no[0] == '9') {
		// NOTE: Slice the bracketed section out of the S/N: B0[17]NNNNNNNNNNNN
		snprintf(device_code, 2 + 1, "%.*s", 2, serial_no + 2);
		// It's in hex, easy peasy.
		device = (Device) strtoul(device_code, NULL, 16);
		if (strcmp(convert_device_id(device), "Unknown") == 0) {
			fprintf(stderr, "Unknown device %s (0x%02X).\n", device_code, device);
			return -1;
		}
	} else {
		// Otherwise, assume it's the new base32-ish format (so far, all of those S/N start with a 'G').
		// In use since the PW3.
		// NOTE: Slice the bracketed section out of the S/N: (G09[0G1]NNNNNNNNNN)
		snprintf(device_code, 3 + 1, "%.*s", 3, serial_no + 3);
		// (these ones are encoded in a slightly custom base 32)
		device = (Device) from_base(device_code, 32);
		if (strcmp(convert_device_id(device), "Unknown") == 0) {
			fprintf(stderr, "Unknown device %s (0x%03X).\n", device_code, device);
			return -1;
		}
	}
	// Handle the Wario (>= PW2) passwords while we're at it... Thanks to npoland for this one ;).
	// NOTE: Remember to check if this is still sane w/ kindle_model_sort.py when new stuff comes out!
	if (device == KindleVoyageWiFi || device == KindlePaperWhite2WiFi4GBInternational ||
	    device >= KindleVoyageWiFi3GJapan) {
		fprintf(stderr, "Platform is Wario or newer [%s]\n", convert_device_id(device));
		fprintf(stderr,
			"Root PW            %s%.*s\nRecovery PW        %s%.*s\n",
			"fiona",
			3,
			&hash[13],
			"fiona",
			4,
			&hash[13]);
	} else {
		fprintf(stderr, "Platform is pre Wario [%s]\n", convert_device_id(device));
		fprintf(stderr,
			"Root PW            %s%.*s\nRecovery PW        %s%.*s\n",
			"fiona",
			3,
			&hash[7],
			"fiona",
			4,
			&hash[7]);
	}
	// Default root passwords are DES hashed, so we only care about the first 8 chars. On the other hand,
	// the recovery MMC export option expects a 9 chars password, so, provide both...
	return 0;
}

int
    main(int argc, char* argv[])
{
	const char* prog_name;
	const char* cmd;

	// Do we want to use unknown devcodes?
	// Very lame test, we only check if the var actually exists, we don't check the value...
	if (getenv("KT_WITH_UNKNOWN_DEVCODES") == NULL) {
		kt_with_unknown_devcodes = 0;
	} else {
		kt_with_unknown_devcodes = 1;
	}

	// Do we want a metadata dump in a shell-friendly format?
	kt_pkg_metadata_dump = getenv("KT_PKG_METADATA_DUMP");

	// Try to use a sane temp directory, and remember it
#if defined(_WIN32) && !defined(__CYGWIN__)
	// Seed rand() so it isn't so utterly awful
	srand((unsigned int) time(NULL));

	// NOTE: Not dealing with the whole TCHAR/WCHAR mess, so, lalalalala, here be dragons!
	char  win_tmpdir[PATH_MAX];
	DWORD ret;
	ret = GetTempPath(PATH_MAX, win_tmpdir);
	if (ret > 0 && ret < PATH_MAX) {
		// NOTE: We need to strip trailing '\', or GetFileAttributes will fail...
		//       c.f., git for windows's compat/mingw.c
		while (ret > 0 && win_tmpdir[ret - 1U] == '\\') {
			win_tmpdir[--ret] = '\0';
		}
		strcpy(kt_tempdir, win_tmpdir);
	}
#else
	const char* posix_tmpdir = getenv("TMPDIR");
	// Flawfinder: ignore
	if (posix_tmpdir != NULL && strlen(posix_tmpdir) < PATH_MAX) {
		strcpy(kt_tempdir, posix_tmpdir);
	}
#endif
	// If we don't have a platform-specific tempdir, use the fallback...
	if (!*kt_tempdir) {
		strcpy(kt_tempdir, KT_TMPDIR);
	} else {
		// Check that our supposedly sane platform-specific tempdir actually exists, and that we can write to it...
		struct stat st;
		if (stat(kt_tempdir, &st) == -1) {
			// ... couldn't stat it (doesn't exist, couldn't be searched):
			//     use our fallback directory and hope for the best
			strcpy(kt_tempdir, KT_TMPDIR);
		} else if (!S_ISDIR(st.st_mode)) {
			// ... it's not a directory: use our fallback directory and hope for the best
			strcpy(kt_tempdir, KT_TMPDIR);
		} else if (access(kt_tempdir, R_OK | W_OK | X_OK) == -1) {
			// NOTE: We might want to use euidaccess or eaccess instead, but those are GNU extensions.
			// ... we can't write into that directory: use our fallback directory and hope for the best
			strcpy(kt_tempdir, KT_TMPDIR);
		}
	}

	prog_name = argv[0];
	// Discard program name for easier parsing
	argv++;
	argc--;

	if (argc > 0) {
		if (strncmp(argv[0], "--", 2) == 0) {
			// Allow our commands to be passed in longform
			argv[0] += 2;
		}
	} else {
		// No command was given, print help and die
		fprintf(stderr, "No command was specified!\n\n");
		kindle_print_help(prog_name);
		exit(1);
	}
	cmd = argv[0];

#if defined(_WIN32) && !defined(__CYGWIN__)
	// Set binary mode properly on MingW, MSVCRT craps out when freopen'ing NULL ;)
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#else
	if (freopen(NULL, "rb", stdin) == NULL) {
		fprintf(stderr, "Cannot set stdin to binary mode: %s.\n", strerror(errno));
		return -1;
	}
	if (freopen(NULL, "wb", stdout) == NULL) {
		fprintf(stderr, "Cannot set stdout to binary mode: %s.\n", strerror(errno));
		return -1;
	}
#endif

	if (strncmp(cmd, "md", 2) == 0) {
		return kindle_obfuscate_main(argc, argv);
	} else if (strncmp(cmd, "dm", 2) == 0) {
		return kindle_deobfuscate_main(argc, argv);
	} else if (strncmp(cmd, "convert", 7) == 0) {
		return kindle_convert_main(argc, argv);
	} else if (strncmp(cmd, "extract", 7) == 0) {
		return kindle_extract_main(argc, argv);
	} else if (strncmp(cmd, "create", 6) == 0) {
		return kindle_create_main(argc, argv);
	} else if (strncmp(cmd, "info", 4) == 0) {
		return kindle_info_main(argc, argv);
	} else if (strncmp(cmd, "version", 7) == 0) {
		return kindle_print_version(prog_name);
	} else if (strncmp(cmd, "help", 4) == 0 || strncmp(cmd, "-help", 5) == 0 || strncmp(cmd, "-h", 2) == 0 ||
		   strncmp(cmd, "-?", 2) == 0 || strncmp(cmd, "/?", 2) == 0 || strncmp(cmd, "/h", 2) == 0 ||
		   strncmp(cmd, "/help", 2) == 0) {
		return kindle_print_help(prog_name);
	} else {
		fprintf(stderr, "Unknown command '%s'!\n\n", cmd);
		kindle_print_help(prog_name);
		exit(1);
	}

	return 1;
}
