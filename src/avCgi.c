/*
 avCgi.c - Common Gateway Interface functions for arvos directory service.

 Copyright (C) 2016   Tamiko Thiel and Peter Graf

 This file is part of ARVOS-APP - AR Viewer Open Source.
 ARVOS-APP is free software.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For more information on the ARVOS-APP, Tamiko Thiel or Peter Graf,
 please see: http://www.arvos-app.com/.

 $Log: avCgi.c,v $
 Revision 1.6  2018/03/11 00:34:52  peter
 Integration with pbl cgi code.

 Revision 1.5  2016/12/03 00:03:54  peter
 Cleanup after tests

 */

/*
 * Make sure "strings <exe> | grep Id | sort -u" shows the source file versions
 */
char * avCgi_c_id = "$Id: avCgi.c,v 1.6 2018/03/11 00:34:52 peter Exp $";

#ifdef WIN32

// define _CRT_RAND_S prior to inclusion statement.
#define _CRT_RAND_S

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#else

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#endif

#include <memory.h>
#include <fcntl.h>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include "arvosCgi.h"

/*****************************************************************************/
/* #defines                                                                  */
/*****************************************************************************/
#define AV_MAX_DIGEST_LEN                   32

/*****************************************************************************/
/* Variables                                                                 */
/*****************************************************************************/

sqlite3 * avSqliteDb = NULL;

char * pblCgiValueIncrement = "i++";

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

/**
 * Wrapper for sqlite3_exec
 */
void avSqlExec(char * statement, int (*callback)(void*, int, char**, char**), void * parameter)
{
	if (!statement)
	{
		pblCgiExitOnError("Out of memory\n");
	}

	char * message = NULL;

	if (SQLITE_OK != sqlite3_exec(avSqliteDb, statement, callback, parameter, &message))
	{
		if (message)
		{
			if (strstr(message, "callback requested query abort"))
			{
				return;
			}
			pblCgiExitOnError("Failed to execute SQL statement \"%s\", message: %s\n", statement, message);
			sqlite3_free(message);
		}
	}
}

/**
 * SqLite callback that expects an int * as parameter and counts the calls made back
 */
int avCallbackCounter(void * ptr, int nColums, char ** values, char ** headers)
{
	int * counter = (int *) ptr;
	if (counter)
	{
		*counter += 1;
	}
	return 0;
}

/**
 * SqLite callback that expects a single cell value and duplicates it to the pointer
 */
int avCallbackCellValue(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums != 1)
	{
		pblCgiExitOnError("SQLite callback avCallbackCellValue called with %d columns\n", nColums);
	}
	char ** cellValue = (char **) ptr;
	if (cellValue)
	{
		*cellValue = pblCgiStrDup(values[0]);
	}
	return 0;
}

/**
 * SqLite callback that expects column values of a single row and sets them to the pointer map
 */
int avCallbackRowValues(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums < 1)
	{
		pblCgiExitOnError("SQLite callback avCallbackRowValues called with %d columns\n", nColums);
	}
	PblMap ** mapPtr = (PblMap **) ptr;
	if (mapPtr)
	{
		for (int i = 0; i < nColums; i++)
		{
			if (pblCgiStrEquals("VALS", headers[i]))
			{
				avDataStrToMap(*mapPtr, values[i]);
			}
			else
			{
				pblCgiSetValueToMap(headers[i], values[i], -1, *mapPtr);
			}
		}
	}
	return 0;
}

/**
 * SqLite callback that expects values of a single column in multiple rows and sets them to the pointer map
 */
int avCallbackColumnValues(void * ptr, int nColums, char ** values, char ** headers)
{
	if (nColums != 1)
	{
		pblCgiExitOnError("SQLite callback avCallbackColumnValues called with %d columns\n", nColums);
	}
	PblMap ** mapPtr = (PblMap **) ptr;
	if (mapPtr)
	{
		char * key = pblCgiSprintf("%d", pblMapSize(*mapPtr));
		pblCgiSetValueToMap(key, values[0], -1, *mapPtr);
		PBL_FREE(key);
	}
	return 0;
}

void avInit(struct timeval * startTime, char * traceFilePath, char * databasePath)
{
	pblCgiStartTime = *startTime;

	//
	// sqlite link object sqlite3.o was created with command:
	// CFLAGS="-Os -DSQLITE_THREADSAFE=0" ./configure; make
	//

	char * filePath = pblCgiStrCat(databasePath, "arvos.sqlite");
	if ( SQLITE_OK != sqlite3_open(filePath, &avSqliteDb))
	{
		if (!avSqliteDb)
		{
			pblCgiExitOnError("Can't open SQLite database '%s': Out of memory\n", filePath);
		}
		pblCgiExitOnError("Can't open SQLite database '%s': %s\n", filePath, sqlite3_errmsg(avSqliteDb));
		sqlite3_close(avSqliteDb);
	}

	char * sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='session';";
	int count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE session ( ID INTEGER PRIMARY KEY, COK TEXT UNIQUE, TLA TEXT, AUT TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			pblCgiExitOnError("Failed to create session table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='author';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE author ( ID INTEGER PRIMARY KEY, NAM TEXT UNIQUE, EML TEXT, TAC TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			pblCgiExitOnError("Failed to create author table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='channel';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create =
				"CREATE TABLE channel ( ID INTEGER PRIMARY KEY, CHN TEXT UNIQUE, AUT TEXT, DES TEXT, DEV TEXT, VALS TEXT );";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			pblCgiExitOnError("Failed to create channel table '%s'\n", create);
		}
	}

	sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='location';";
	count = 0;

	avSqlExec(sql, avCallbackCounter, &count);

	if (count == 0)
	{
		char * create = "CREATE TABLE location ( ID INTEGER PRIMARY KEY, CHN TEXT, POS TEXT, VALS TEXT ); "
				"CREATE INDEX location_POS_index ON location(POS);";

		avSqlExec(create, NULL, NULL);
		avSqlExec(sql, avCallbackCounter, &count);

		if (count == 0)
		{
			pblCgiExitOnError("Failed to create location table '%s'\n", create);
		}
	}

	if (traceFilePath && *traceFilePath)
	{
		FILE * stream;

#ifdef WIN32

		errno_t err = fopen_s(&stream, filePath, "r");
		if (err != 0)
		{
			return;
		}

#else

		if (!(stream = fopen(traceFilePath, "r")))
		{
			return;
		}

#endif

		fclose(stream);

		pblCgiTraceFile = pblCgiFopen(traceFilePath, "a");
		fputs("\n", pblCgiTraceFile);
		fputs("\n", pblCgiTraceFile);
		PBL_CGI_TRACE("----------------------------------------> Started");

		extern char **environ;
		char ** envp = environ;

		while (envp && *envp)
		{
			PBL_CGI_TRACE("ENV %s", *envp++);
		}
	}
}

static char * avCookieKey = AV_COOKIE;
static char * avCookieTag = AV_COOKIE "=";

/**
 * Get the cookie, if any is given.
 */
char * avGetCoockie()
{
	return pblCgiGetCoockie(avCookieKey, avCookieTag);
}

#ifndef ARVOS_CRYPTOLOGIC_RANDOM

static int avRandomFirst = 1;

unsigned char * avRandomBytes(unsigned char * buffer, size_t length)
{
	if (avRandomFirst)
	{
		avRandomFirst = 0;

		struct timeval now;
		gettimeofday(&now, NULL);
		srand(now.tv_sec ^ now.tv_usec ^ getpid());
	}
	if (!rand() % 10)
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		srand(rand() ^ now.tv_sec ^ now.tv_usec ^ getpid());
	}

	for (unsigned int i = 0; i < length; i++)
	{
		buffer[i] = rand() % 0xff;
	}
	return buffer;
}

#else

/**
 * Fill the buffer with random bytes.
 */
unsigned char * avRandomBytes(unsigned char * buffer, size_t length)
{
	static char * tag = "avRandomBytes";

#ifdef WIN32

	unsigned int number;
	unsigned char * ptr = (unsigned char *) &number;

	for (unsigned int i = 0; i < length;)
	{
		if (rand_s(&number))
		{
			pblCgiExitOnError("%s: rand_s failed, errno=%d\n", tag, errno);
		}

		for (unsigned int j = 0; j < 4 && i < length; i++, j++)
		{
			buffer[i] = ptr[j];
		}
	}

#else

	int randomData = open("/dev/random", O_RDONLY);

	size_t randomDataLen = 0;
	while (randomDataLen < length)
	{
		ssize_t result = read(randomData, buffer + randomDataLen, length - randomDataLen);
		if (result < 0)
		{
			pblCgiExitOnError("%s: unable to read /dev/random, errno=%d\n", tag,
					errno);
		}
		randomDataLen += result;
	}
	close(randomData);

#endif
	return buffer;
}
#endif

/*
 * Sha256 implementation, as defined by NIST
 */
#define Sha256_min( x, y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )

#define Sha256_S(x,n) ( ((x)>>(n)) | ((x)<<(32-(n))) )
#define Sha256_R(x,n) ( (x)>>(n) )

#define Sha256_Ch(x,y,z)  ( ((x) & (y)) ^ (~(x) & (z)) )
#define Sha256_Maj(x,y,z) ( ( (x) & (y) ) ^ ( (x) & (z) ) ^ ( (y) & (z) ) )

#define Sha256_SIG0(x) ( Sha256_S(x, 2) ^ Sha256_S(x,13) ^ Sha256_S(x,22) )
#define Sha256_SIG1(x) ( Sha256_S(x, 6) ^ Sha256_S(x,11) ^ Sha256_S(x,25) )
#define Sha256_sig0(x) ( Sha256_S(x, 7) ^ Sha256_S(x,18) ^ Sha256_R(x, 3) )
#define Sha256_sig1(x) ( Sha256_S(x,17) ^ Sha256_S(x,19) ^ Sha256_R(x,10) )

static unsigned int K[] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
		0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138,
		0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70,
		0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa,
		0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

#define Sha256_H1      0x6a09e667
#define Sha256_H2      0xbb67ae85
#define Sha256_H3      0x3c6ef372
#define Sha256_H4      0xa54ff53a
#define Sha256_H5      0x510e527f
#define Sha256_H6      0x9b05688c
#define Sha256_H7      0x1f83d9ab
#define Sha256_H8      0x5be0cd19

static unsigned int H[8] = { Sha256_H1, Sha256_H2, Sha256_H3, Sha256_H4, Sha256_H5, Sha256_H6, Sha256_H7,
Sha256_H8 };

/* convert to big endian where needed */

static void convert_to_bigendian(void* data, int len)
{
	/* test endianness */
	unsigned int test_value = 0x01;
	unsigned char* test_as_bytes = (unsigned char*) &test_value;
	int little_endian = test_as_bytes[0];

	unsigned int temp;
	unsigned char* temp_as_bytes = (unsigned char*) &temp;
	unsigned int* data_as_words = (unsigned int*) data;
	unsigned char* data_as_bytes;
	int i;

	if (little_endian)
	{
		len /= 4;
		for (i = 0; i < len; i++)
		{
			temp = data_as_words[i];
			data_as_bytes = (unsigned char*) &(data_as_words[i]);

			data_as_bytes[0] = temp_as_bytes[3];
			data_as_bytes[1] = temp_as_bytes[2];
			data_as_bytes[2] = temp_as_bytes[1];
			data_as_bytes[3] = temp_as_bytes[0];
		}
	}
	/* on big endian machines do nothing as the CPU representation
	 automatically does the right thing for SHA1 */
}

/*
 * sha256 context
 */
typedef struct avSha256_ctx_s
{
	unsigned int H[8];
	unsigned int hbits;
	unsigned int lbits;
	unsigned char M[64];
	unsigned char mlen;

} avSha256_ctx_t;

static void avSha256_init(avSha256_ctx_t* ctx)
{
	memcpy(ctx->H, H, 8 * sizeof(unsigned int));
	ctx->lbits = 0;
	ctx->hbits = 0;
	ctx->mlen = 0;
}

static void Sha256_transform(avSha256_ctx_t* ctx)
{
	int j;
	unsigned int A = ctx->H[0];
	unsigned int B = ctx->H[1];
	unsigned int C = ctx->H[2];
	unsigned int D = ctx->H[3];
	unsigned int E = ctx->H[4];
	unsigned int F = ctx->H[5];
	unsigned int G = ctx->H[6];
	unsigned int H = ctx->H[7];
	unsigned int T1, T2;
	unsigned int W[64];

	memcpy(W, ctx->M, 64);

	for (j = 16; j < 64; j++)
	{
		W[j] = Sha256_sig1(W[j - 2]) + W[j - 7] + Sha256_sig0(W[j - 15]) + W[j - 16];
	}

	for (j = 0; j < 64; j++)
	{
		T1 = H + Sha256_SIG1(E) + Sha256_Ch(E, F, G) + K[j] + W[j];
		T2 = Sha256_SIG0(A) + Sha256_Maj(A, B, C);
		H = G;
		G = F;
		F = E;
		E = D + T1;
		D = C;
		C = B;
		B = A;
		A = T1 + T2;
	}

	ctx->H[0] += A;
	ctx->H[1] += B;
	ctx->H[2] += C;
	ctx->H[3] += D;
	ctx->H[4] += E;
	ctx->H[5] += F;
	ctx->H[6] += G;
	ctx->H[7] += H;
}

static void avSha256_update(avSha256_ctx_t* ctx, const void* vdata, unsigned int data_len)
{
	const unsigned char* data = vdata;
	unsigned int use;
	unsigned int low_bits;

	/* convert data_len to bits and add to the 64 bit word formed by lbits
	 and hbits */

	ctx->hbits += data_len >> 29;
	low_bits = data_len << 3;
	ctx->lbits += low_bits;
	if (ctx->lbits < low_bits)
	{
		ctx->hbits++;
	}

	/* deal with first block */

	use = (unsigned int) (Sha256_min((unsigned int )(64 - ctx->mlen), data_len));
	memcpy(ctx->M + ctx->mlen, data, use);
	ctx->mlen += (unsigned char) use;
	data_len -= use;
	data += use;

	while (ctx->mlen == 64)
	{
		convert_to_bigendian((unsigned int*) ctx->M, 64);
		Sha256_transform(ctx);
		use = Sha256_min(64, data_len);
		memcpy(ctx->M, data, use);
		ctx->mlen = (unsigned char) use;
		data_len -= use;
		data += use;
	}
}

static void avSha256_final(avSha256_ctx_t* ctx)
{
	if (ctx->mlen < 56)
	{
		ctx->M[ctx->mlen] = 0x80;
		ctx->mlen++;
		memset(ctx->M + ctx->mlen, 0x00, 56 - ctx->mlen);
		convert_to_bigendian(ctx->M, 56);
	}
	else
	{
		ctx->M[ctx->mlen] = 0x80;
		ctx->mlen++;
		memset(ctx->M + ctx->mlen, 0x00, 64 - ctx->mlen);
		convert_to_bigendian(ctx->M, 64);
		Sha256_transform(ctx);
		memset(ctx->M, 0x00, 56);
	}

	memcpy(ctx->M + 56, (void*) (&(ctx->hbits)), 8);
	Sha256_transform(ctx);
}

void avSha256_digest(avSha256_ctx_t* ctx, unsigned char* digest)
{
	if (digest)
	{
		memcpy(digest, ctx->H, 8 * sizeof(unsigned int));
		convert_to_bigendian(digest, 8 * sizeof(unsigned int));
	}
}

/**
 * Return the sha256 digest of a given buffer of bytes.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return AV_DIGEST_LEN bytes of static binary data
 */
unsigned char * avSha256(unsigned char * buffer, size_t length)
{
	static char * tag = "avSha256";

	unsigned char * digest = pbl_malloc(tag, AV_MAX_DIGEST_LEN);
	if (!digest)
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}
	avSha256_ctx_t context;

	avSha256_init(&context);
	avSha256_update(&context, buffer, length);
	avSha256_final(&context);
	avSha256_digest(&context, digest);

	return (digest);
}

/**
 * Return the sha256 digest of a given buffer of bytes as hex string.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return The digest as a malloced hex string.
 */
char * avSha256AsHexString(unsigned char * buffer, size_t length)
{
	unsigned char * digest = avSha256(buffer, length);
	char * string = pblCgiStrToHexFromBuffer(digest, AV_MAX_DIGEST_LEN);
	PBL_FREE(digest);
	return string;
}

static char * avReplaceLowerThan(char * string, char *ptr2)
{
	static char * tag = "avReplaceLowerThan";
	char * ptr = string;

	PblStringBuilder * stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	for (int i = 0; i < 1000000; i++)
	{
		size_t length = ptr2 - ptr;
		if (length > 0)
		{
			if (pblStringBuilderAppendStrN(stringBuilder, length, ptr) == ((size_t) -1))
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}

		if (pblStringBuilderAppendStr(stringBuilder, "&lt;") == ((size_t) -1))
		{
			pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
		ptr = ptr2 + 1;
		ptr2 = strstr(ptr, "<");
		if (!ptr2)
		{
			if (pblStringBuilderAppendStr(stringBuilder, ptr) == ((size_t) -1))
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}

			char * result = pblStringBuilderToString(stringBuilder);
			if (!result)
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			pblStringBuilderFree(stringBuilder);
			return result;
		}
	}
	pblCgiExitOnError("%s: There are more than 1000000 '<' characters to be replaced in one string.\n", tag);
	return NULL;
}

static char * avReplaceVariable(char * string, int iteration)
{
	static char * tag = "avReplaceVariable";
	static char * startPattern = "<!--?";
	static char * startPattern2 = "<?";

	int startPatternLength;
	char * endPattern;
	int endPatternLength;

	char * ptr = strchr(string, '<');
	if (!ptr)
	{
		return string;
	}

	size_t length = ptr - string;

	char * ptr2 = strstr(ptr, startPattern);
	if (!ptr2)
	{
		ptr2 = strstr(ptr, startPattern2);
		if (!ptr2)
		{
			return string;
		}
		else
		{
			startPatternLength = 2;
			endPattern = ">";
			endPatternLength = 1;
		}
	}
	else
	{
		char * ptr3 = strstr(ptr, startPattern2);
		if (ptr3 && ptr3 < ptr2)
		{
			ptr2 = ptr3;
			startPatternLength = 2;
			endPattern = ">";
			endPatternLength = 1;
		}
		else
		{
			startPatternLength = 5;
			endPattern = "-->";
			endPatternLength = 3;
		}
	}

	PblStringBuilder * stringBuilder = pblStringBuilderNew();
	if (!stringBuilder)
	{
		pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
	}

	if (length > 0)
	{
		if (pblStringBuilderAppendStrN(stringBuilder, length, string) == ((size_t) -1))
		{
			pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
		}
	}

	char * result;

	for (int i = 0; i < 1000000; i++)
	{
		length = ptr2 - ptr;
		if (length > 0)
		{
			if (pblStringBuilderAppendStrN(stringBuilder, length, ptr) == ((size_t) -1))
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
		}

		ptr = ptr2 + startPatternLength;
		ptr2 = strstr(ptr, endPattern);
		if (!ptr2)
		{
			result = pblStringBuilderToString(stringBuilder);
			if (!result)
			{
				pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
			}
			pblStringBuilderFree(stringBuilder);
			return result;
		}

		char * key = pblCgiStrRangeDup(ptr, ptr2);
		char * value = NULL;
		if (iteration >= 0)
		{
			value = pblCgiValueForIteration(key, iteration);
		}
		if (!value)
		{
			value = pblCgiValue(key);
		}
		PBL_FREE(key);

		if (value && *value)
		{
			char * pointer = strstr(value, "<");
			if (!pointer)
			{
				if (pblStringBuilderAppendStr(stringBuilder, value) == ((size_t) -1))
				{
					pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
			}
			else
			{
				value = avReplaceLowerThan(value, pointer);
				if (value && *value)
				{
					if (pblStringBuilderAppendStr(stringBuilder, value) == ((size_t) -1))
					{
						pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
					}
				}
				PBL_FREE(value);
			}
		}
		ptr = ptr2 + endPatternLength;
		ptr2 = strstr(ptr, startPattern);
		if (!ptr2)
		{
			ptr2 = strstr(ptr, startPattern2);
			if (!ptr2)
			{
				if (pblStringBuilderAppendStr(stringBuilder, ptr) == ((size_t) -1))
				{
					pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
				result = pblStringBuilderToString(stringBuilder);
				if (!result)
				{
					pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
				pblStringBuilderFree(stringBuilder);
				return result;
			}
			else
			{
				startPatternLength = 2;
				endPattern = ">";
				endPatternLength = 1;
			}
		}
		else
		{
			char * ptr3 = strstr(ptr, startPattern2);
			if (ptr3 && ptr3 < ptr2)
			{
				ptr2 = ptr3;
				startPatternLength = 2;
				endPattern = ">";
				endPatternLength = 1;
			}
			else
			{
				startPatternLength = 5;
				endPattern = "-->";
				endPatternLength = 3;
			}
		}
	}
	pblCgiExitOnError("%s: There are more than 1000000 variables defined in one string.\n", tag);
	return NULL;
}

static char * avPrintStr(char * string, FILE * outStream, int iteration);

static char * avSkip(char * string, char * skipKey, FILE * outStream, int iteration)
{
	string = avReplaceVariable(string, -1);

	for (;;)
	{
		char * ptr = strstr(string, "<!--#ENDIF");
		if (!ptr)
		{
			return skipKey;
		}
		ptr += 10;

		char *ptr2 = strstr(ptr, "-->");
		if (ptr2)
		{
			char * key = pblCgiStrRangeDup(ptr, ptr2);
			if (!strcmp(skipKey, key))
			{
				PBL_FREE(skipKey);
				PBL_FREE(key);
				return avPrintStr(ptr2 + 3, outStream, iteration);
			}

			PBL_FREE(key);
			string = ptr2 + 3;
			continue;
		}
		break;
	}
	return skipKey;
}

static char * avPrintStr(char * string, FILE * outStream, int iteration)
{
	string = avReplaceVariable(string, iteration);

	for (;;)
	{
		char * ptr = strstr(string, "<!--#");
		if (!ptr)
		{
			fputs(string, outStream);
			return NULL;
		}

		while (string < ptr)
		{
			fputc(*string++, outStream);
		}

		if (!memcmp(string, "<!--#IFDEF", 10))
		{
			ptr += 10;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * key = pblCgiStrRangeDup(ptr, ptr2);
				if (!pblCgiValue(key) && !pblCgiValueForIteration(key, iteration))
				{
					return avSkip(ptr2 + 1, key, outStream, iteration);
				}
				PBL_FREE(key);
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		if (!memcmp(string, "<!--#IFNDEF", 11))
		{
			ptr += 11;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				char * key = pblCgiStrRangeDup(ptr, ptr2);
				if (pblCgiValue(key) || pblCgiValueForIteration(key, iteration))
				{
					return avSkip(ptr2 + 1, key, outStream, iteration);
				}
				PBL_FREE(key);
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		if (!memcmp(string, "<!--#ENDIF", 10))
		{
			ptr += 7;

			char *ptr2 = strstr(ptr, "-->");
			if (ptr2)
			{
				string = ptr2 + 3;
				continue;
			}
			return NULL;
		}
		fputs(string, outStream);
		break;
	}
	return NULL;
}

/**
 * Get the values from the data of this string as a map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map as malloced memory.
 */
PblMap * avDataValues(char * buffer)
{
	return avDataStrToMap(NULL, buffer);
}

/**
 * Update values from the data of the map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map after the update as malloced memory.
 */
PblMap * avUpdateData(PblMap * map, char ** keys, char ** values)
{
	static char * tag = "avUpdateData";

	if (!map)
	{
		map = pblCgiNewMap();
	}

	for (int i = 0; keys[i]; i++)
	{
		char * value = values[i];
		if (value == pblCgiValueIncrement)
		{
			value = pblMapGetStr(map, keys[i]);
			if (value && *value)
			{
				int n = atoi(value);
				int length = strlen(value) + 1;
				value = pbl_malloc0(tag, length + 2);
				if (!value)
				{
					pblCgiExitOnError("%s: pbl_errno = %d, message='%s'\n", tag, pbl_errno, pbl_errstr);
				}
				snprintf(value, length + 1, "%d", ++n);
				if (pblMapAddStrStr(map, keys[i], value) < 0)
				{
					pblCgiExitOnError("%s: Failed to save string '%s', value '%s' in the map, pbl_errno %d\n", tag, keys[i],
							value, pbl_errno);
				}
				PBL_FREE(value);
			}
			else if (pblMapAddStrStr(map, keys[i], "1") < 0)
			{
				pblCgiExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keys[i], pbl_errno);
			}
		}
		else if (value)
		{
			char * ptr = value;
			while ((ptr = strchr(ptr, '\t')))
			{
				*ptr++ = ' ';
			}

			if (pblMapAddStrStr(map, keys[i], value) < 0)
			{
				pblCgiExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keys[i], pbl_errno);
			}
		}
		else
		{
			pblMapUnmapStr(map, keys[i]);
		}
	}
	return map;
}

/**
 * Write values of the map to a string.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * string: The string as malloced memory.
 */
char * avMapToDataStr(PblMap * map)
{
	static char * tag = "avMapToDataStr";

	PblStringBuilder * stringBuilder = pblMapStrStrToStringBuilder(map, "\t", "=");
	if (!stringBuilder)
	{
		pblCgiExitOnError("%s: Failed to convert map to a string builder, pbl_errno %d\n", tag, pbl_errno);
	}

	char * data = pblStringBuilderToString(stringBuilder);
	if (!data)
	{
		pblCgiExitOnError("%s: Failed to convert string builder to a string, pbl_errno %d\n", tag, pbl_errno);
	}
	pblStringBuilderFree(stringBuilder);

	return data;
}

/**
 * Get the values from the data of this string to a map.
 *
 * If an error occurs, the program exits with an error message.
 *
 * @return char * map: The map as malloced memory.
 */
PblMap * avDataStrToMap(PblMap * map, char * buffer)
{
	static char * tag = "avDataStrToMap";

	if (!map)
	{
		map = pblCgiNewMap();
	}

	char * listItem;
	char * keyValuePair[2 + 1];
	PblList * keyValuePairs = pblCgiStrSplitToList(buffer, "\t");

	while (!pblListIsEmpty(keyValuePairs))
	{
		listItem = (char *)pblListPop(keyValuePairs);
		pblCgiStrSplit(listItem, "=", 2, keyValuePair);
		if (keyValuePair[0] && keyValuePair[1] && !keyValuePair[2])
		{
			if (pblMapAddStrStr(map, keyValuePair[0], keyValuePair[1]) < 0)
			{
				pblCgiExitOnError("%s: Failed to save string '%s' in the map, pbl_errno %d\n", tag, keyValuePair[0],
						pbl_errno);
			}
		}

		PBL_FREE(keyValuePair[0]);
		PBL_FREE(keyValuePair[1]);
		PBL_FREE(listItem);
	}
	pblListFree(keyValuePairs);

	return map;
}
