#ifndef CODA_ZLIB_MANGLE_H
#define CODA_ZLIB_MANGLE_H

/*
 * This header file mangles symbols exported from the zlib library.
 * This is needed on some platforms because of nameresolving conflicts when
 * CODA is used as a module in an application that has its own version of zlib.
 * (this problem was seen with the CODA IDL interface on Linux).
 * Even though name mangling is not needed for every platform or CODA
 * interface, we always perform the mangling for consitency reasons.
 */

#define adler32 coda_adler32
#define adler32_combine coda_adler32_combine
#define adler32_combine64 coda_adler32_combine64
#define crc32 coda_crc32
#define crc32_combine coda_crc32_combine
#define crc32_combine64 coda_crc32_combine64
#define get_crc_table coda_get_crc_table
#define inflate coda_inflate
#define inflate_copyright coda_inflate_copyright
#define inflate_fast coda_inflate_fast
#define inflate_table coda_inflate_table
#define inflateCopy coda_inflateCopy
#define inflateEnd coda_inflateEnd
#define inflateGetDictionary coda_inflateGetDictionary
#define inflateGetHeader coda_inflateGetHeader
#define inflateInit_ coda_inflateInit_
#define inflateInit2_ coda_inflateInit2_
#define inflateMark coda_inflateMark
#define inflatePrime coda_inflatePrime
#define inflateReset coda_inflateReset
#define inflateReset2 coda_inflateReset2
#define inflateResetKeep coda_inflateResetKeep
#define inflateSetDictionary coda_inflateSetDictionary
#define inflateSync coda_inflateSync
#define inflateSyncPoint coda_inflateSyncPoint
#define inflateUndermine coda_inflateUndermine
#define z_errmsg coda_z_errmsg
#define zcalloc coda_zcalloc
#define zcfree coda_zcfree
#define zError coda_zError
#define zlibCompileFlags coda_zlibCompileFlags
#define zlibVersion coda_zlibVersion

#endif
