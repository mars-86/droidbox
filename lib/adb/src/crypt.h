#ifndef __ANDROID_DEBUG_BRIDGE_CRYPT_INCLUDED_H__
#define __ANDROID_DEBUG_BRIDGE_CRYPT_INCLUDED_H__

#include <stdint.h>

/**
 * struct RSAPublicKey - holder for a public key
 *
 * An RSA public key consists of a modulus (typically called N), the inverse
 * and R^2, where R is 2^(# key bits).
 */

/* Borrowed this from myncript https://github.com/topjohnwu/mincrypt/blob/master/include/mincrypt/rsa.h */

#define RSANUMBYTES 256 /* 2048 bit key length */
#define RSANUMWORDS (RSANUMBYTES / sizeof(uint32_t))

typedef struct RSAPublicKey {
    int len; /* Length of n[] in number of uint32_t */
    uint32_t n0inv; /* -1 / n[0] mod 2^32 */
    uint32_t n[RSANUMWORDS]; /* modulus as little endian array */
    uint32_t rr[RSANUMWORDS]; /* R^2 as little endian array */
    int exponent; /* 3 or 65537 */
} RSAPublicKey;

int rsa_public_key(const char* public_key, RSAPublicKey* rsapub);

#endif // __ANDROID_DEBUG_BRIDGE_CRYPT_INCLUDED_H__
