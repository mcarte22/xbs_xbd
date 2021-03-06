/*
 * crypto_hash/try.c version 20090118
 * D. J. Bernstein
 * Jens Graef (128b-modifications)
 * Public domain.
 */

#include <stdlib.h>
#include "crypto_hash.h"

extern unsigned char *alignedcalloc(unsigned long long);

const char *primitiveimplementation = crypto_hash_IMPLEMENTATION;

#define MAXTEST_BYTES (10000 + crypto_hash_BYTES)
#define CHECKSUM_BYTES 128
#define CHECKSUM_PAD_BYTES 2048 
#define TUNE_BYTES 1536

static unsigned char *h;
static unsigned char *h2;
static unsigned char *m;
static unsigned char *m2;

void preallocate(void)
{
}

void allocate(void)
{
  h = alignedcalloc(crypto_hash_BYTES);
  h2 = alignedcalloc(crypto_hash_BYTES);
  m = alignedcalloc(MAXTEST_BYTES);
  m2 = alignedcalloc(MAXTEST_BYTES);
}

void predoit(void)
{
}

void doit(void)
{
  crypto_hash(h,m,TUNE_BYTES);
}

char checksum[crypto_hash_BYTES * 2 + 1];

const char *checksum_compute(void)
{
  long long i;
  long long j;

  for (i = 0;i < CHECKSUM_BYTES;++i) {
    long long hlen = crypto_hash_BYTES;
    long long mlen = i;

    /* fill area before start und end with random */
    for (j = -16;j < 0;++j) h[j] = random();
    for (j = hlen;j < hlen + 16;++j) h[j] = random();
    /* copy to h2 */
    for (j = -16;j < hlen + 16;++j) h2[j] = h[j];
    /* fill area before start und end with random */
    for (j = -16;j < 0;++j) m[j] = random();
    for (j = mlen;j < mlen + 16;++j) m[j] = random();
    /* copy to h1 */
    for (j = -16;j < mlen + 16;++j) m2[j] = m[j];


    if (crypto_hash(h,m,mlen) != 0) return "crypto_hash returns nonzero";
    /* check original message */
    for (j = -16;j < mlen + 16;++j) if (m2[j] != m[j]) return "crypto_hash writes to input";
    /* check area before start and end */
    for (j = -16;j < 0;++j) if (h2[j] != h[j]) return "crypto_hash writes before output";
    for (j = hlen;j < hlen + 16;++j) if (h2[j] != h[j]) return "crypto_hash writes after output";
    /* crypt into itself */
    if (crypto_hash(m2,m2,mlen) != 0) return "crypto_hash returns nonzero";
    /* compare */
    for (j = 0;j < hlen;++j) if (m2[j] != h[j]) return "crypto_hash does not handle overlap";


    /* xor the message with the hash */
    for (j = 0;j < mlen;++j) m[j] ^= h[j % hlen];
    /* take the first byte of the hast as new next byte for message */
    m[mlen] = h[0];
  }
  
  /* padding */
  for(i=CHECKSUM_BYTES; i<CHECKSUM_PAD_BYTES;i++)
  {
    j=i-CHECKSUM_BYTES;

    /** convert to bcd */
    unsigned char high=(j%100)/10;
    unsigned char low=j%10;
    m[i]=high*16+low;
  } 

  /* hash it */
  if (crypto_hash(h,m,CHECKSUM_PAD_BYTES) != 0) return "crypto_hash returns nonzero";

  for (i = 0;i < crypto_hash_BYTES;++i) {
    checksum[2 * i] = "0123456789abcdef"[15 & (h[i] >> 4)];
    checksum[2 * i + 1] = "0123456789abcdef"[15 & h[i]];
  }
  checksum[2 * i] = 0;
  return 0;
}
