/*
 * sha.c: routines to compute SHA-1/256/384/512 digests
 *
 * Ref: NIST FIPS PUB 180-2 Secure Hash Standard
 *
 * Copyright (C) 2003 Mark Shelor, All Rights Reserved
 *
 * Version: 2.4
 * Sat Nov 22 17:10:22 MST 2003
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sha.h"
#include "sha64bit.h"
#include "endian.h"

#define SHR(x, n)	( (x) >> (n) )
#define ROTR(x, n)	( ( (x) >> (n) ) | ( (x) << (32 - (n)) ) )
#define ROTL(x, n)	( ( (x) << (n) ) | ( (x) >> (32 - (n)) ) )

#define Ch(x, y, z)	( (z) ^ ( (x) & ( (y) ^ (z) ) ) )
#define Parity(x, y, z)	( (x) ^ (y) ^ (z) )
#define Maj(x, y, z)	( ( (x) & (y) ) | ( (z) & ( (x) | (y) ) ) )

#define SIGMA0(x)	( ROTR(x,  2) ^ ROTR(x, 13) ^ ROTR(x, 22) )
#define SIGMA1(x)	( ROTR(x,  6) ^ ROTR(x, 11) ^ ROTR(x, 25) )
#define sigma0(x)	( ROTR(x,  7) ^ ROTR(x, 18) ^  SHR(x,  3) )
#define sigma1(x)	( ROTR(x, 17) ^ ROTR(x, 19) ^  SHR(x, 10) )

#define SETBIT(str, pos)	str[(pos) >> 3] |=  (0x01 << (7 - (pos) % 8))
#define CLRBIT(str, pos)	str[(pos) >> 3] &= ~(0x01 << (7 - (pos) % 8))
#define BYTECNT(bitcnt)		(1 + (((bitcnt) - 1) >> 3))

#define K11	0x5a827999UL
#define K12	0x6ed9eba1UL
#define K13	0x8f1bbcdcUL
#define K14	0xca62c1d6UL

static unsigned long K256[64] =
{
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static unsigned long H01[5] =
{
	0x67452301UL, 0xefcdab89UL, 0x98badcfeUL,
	0x10325476UL, 0xc3d2e1f0UL
};

static unsigned long H0256[8] =
{
	0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
	0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL
};

static void ul2mem(mem, ul)		/* endian-neutral */
unsigned char *mem;
unsigned long ul;
{
	int i;

	for (i = 0; i < 4; i++)
		*mem++ = SHR(ul, 24 - i * 8) & 0xff;
}

static unsigned long W[64];

static void sha1(p, block)
SHA *p;
unsigned char *block;
{
	int t;
	unsigned long a, b, c, d, e;
	unsigned long *q = W;

	if (sha_big_endian)
		memcpy(W, block, 64);
	else for (t = 0; t < 16; t++, q++) {
		*q = *block++; *q = (*q << 8) + *block++;
		*q = (*q << 8) + *block++; *q = (*q << 8) + *block++;
	}

/*
 * Use SHA-1 alternate method from FIPS PUB 180-2 (ref. 6.1.3)
 *
 * To improve performance, unroll the loop and consolidate assignments
 * by changing the roles of variables "a" through "e" at each step.
 * Note that the variable "T" is no longer needed.
 *
 */

#define MIX(a,b,c,d,e,f,k,w)	e+=ROTL(a,5)+f(b,c,d)+k+w;b=ROTL(b,30)

#define ASG(s)	\
	(W[s&15]=ROTL(W[(s+13)&15]^W[(s+8)&15]^W[(s+2)&15]^W[s&15],1))

	a = p->H[0]; b = p->H[1]; c = p->H[2]; d = p->H[3]; e = p->H[4];
	MIX(a, b, c, d, e, Ch, K11, W[0]);
	MIX(e, a, b, c, d, Ch, K11, W[1]);
	MIX(d, e, a, b, c, Ch, K11, W[2]);
	MIX(c, d, e, a, b, Ch, K11, W[3]);
	MIX(b, c, d, e, a, Ch, K11, W[4]);
	MIX(a, b, c, d, e, Ch, K11, W[5]);
	MIX(e, a, b, c, d, Ch, K11, W[6]);
	MIX(d, e, a, b, c, Ch, K11, W[7]);
	MIX(c, d, e, a, b, Ch, K11, W[8]);
	MIX(b, c, d, e, a, Ch, K11, W[9]);
	MIX(a, b, c, d, e, Ch, K11, W[10]);
	MIX(e, a, b, c, d, Ch, K11, W[11]);
	MIX(d, e, a, b, c, Ch, K11, W[12]);
	MIX(c, d, e, a, b, Ch, K11, W[13]);
	MIX(b, c, d, e, a, Ch, K11, W[14]);
	MIX(a, b, c, d, e, Ch, K11, W[15]);
	MIX(e, a, b, c, d, Ch, K11, ASG(0));
	MIX(d, e, a, b, c, Ch, K11, ASG(1));
	MIX(c, d, e, a, b, Ch, K11, ASG(2));
	MIX(b, c, d, e, a, Ch, K11, ASG(3));
	MIX(a, b, c, d, e, Parity, K12, ASG(4));
	MIX(e, a, b, c, d, Parity, K12, ASG(5));
	MIX(d, e, a, b, c, Parity, K12, ASG(6));
	MIX(c, d, e, a, b, Parity, K12, ASG(7));
	MIX(b, c, d, e, a, Parity, K12, ASG(8));
	MIX(a, b, c, d, e, Parity, K12, ASG(9));
	MIX(e, a, b, c, d, Parity, K12, ASG(10));
	MIX(d, e, a, b, c, Parity, K12, ASG(11));
	MIX(c, d, e, a, b, Parity, K12, ASG(12));
	MIX(b, c, d, e, a, Parity, K12, ASG(13));
	MIX(a, b, c, d, e, Parity, K12, ASG(14));
	MIX(e, a, b, c, d, Parity, K12, ASG(15));
	MIX(d, e, a, b, c, Parity, K12, ASG(0));
	MIX(c, d, e, a, b, Parity, K12, ASG(1));
	MIX(b, c, d, e, a, Parity, K12, ASG(2));
	MIX(a, b, c, d, e, Parity, K12, ASG(3));
	MIX(e, a, b, c, d, Parity, K12, ASG(4));
	MIX(d, e, a, b, c, Parity, K12, ASG(5));
	MIX(c, d, e, a, b, Parity, K12, ASG(6));
	MIX(b, c, d, e, a, Parity, K12, ASG(7));
	MIX(a, b, c, d, e, Maj, K13, ASG(8));
	MIX(e, a, b, c, d, Maj, K13, ASG(9));
	MIX(d, e, a, b, c, Maj, K13, ASG(10));
	MIX(c, d, e, a, b, Maj, K13, ASG(11));
	MIX(b, c, d, e, a, Maj, K13, ASG(12));
	MIX(a, b, c, d, e, Maj, K13, ASG(13));
	MIX(e, a, b, c, d, Maj, K13, ASG(14));
	MIX(d, e, a, b, c, Maj, K13, ASG(15));
	MIX(c, d, e, a, b, Maj, K13, ASG(0));
	MIX(b, c, d, e, a, Maj, K13, ASG(1));
	MIX(a, b, c, d, e, Maj, K13, ASG(2));
	MIX(e, a, b, c, d, Maj, K13, ASG(3));
	MIX(d, e, a, b, c, Maj, K13, ASG(4));
	MIX(c, d, e, a, b, Maj, K13, ASG(5));
	MIX(b, c, d, e, a, Maj, K13, ASG(6));
	MIX(a, b, c, d, e, Maj, K13, ASG(7));
	MIX(e, a, b, c, d, Maj, K13, ASG(8));
	MIX(d, e, a, b, c, Maj, K13, ASG(9));
	MIX(c, d, e, a, b, Maj, K13, ASG(10));
	MIX(b, c, d, e, a, Maj, K13, ASG(11));
	MIX(a, b, c, d, e, Parity, K14, ASG(12));
	MIX(e, a, b, c, d, Parity, K14, ASG(13));
	MIX(d, e, a, b, c, Parity, K14, ASG(14));
	MIX(c, d, e, a, b, Parity, K14, ASG(15));
	MIX(b, c, d, e, a, Parity, K14, ASG(0));
	MIX(a, b, c, d, e, Parity, K14, ASG(1));
	MIX(e, a, b, c, d, Parity, K14, ASG(2));
	MIX(d, e, a, b, c, Parity, K14, ASG(3));
	MIX(c, d, e, a, b, Parity, K14, ASG(4));
	MIX(b, c, d, e, a, Parity, K14, ASG(5));
	MIX(a, b, c, d, e, Parity, K14, ASG(6));
	MIX(e, a, b, c, d, Parity, K14, ASG(7));
	MIX(d, e, a, b, c, Parity, K14, ASG(8));
	MIX(c, d, e, a, b, Parity, K14, ASG(9));
	MIX(b, c, d, e, a, Parity, K14, ASG(10));
	MIX(a, b, c, d, e, Parity, K14, ASG(11));
	MIX(e, a, b, c, d, Parity, K14, ASG(12));
	MIX(d, e, a, b, c, Parity, K14, ASG(13));
	MIX(c, d, e, a, b, Parity, K14, ASG(14));
	MIX(b, c, d, e, a, Parity, K14, ASG(15));
	p->H[0] += a; p->H[1] += b; p->H[2] += c; p->H[3] += d; p->H[4] += e;
}

static void sha256(p, block)
SHA *p;
unsigned char *block;
{
	int t;
	unsigned long a, b, c, d, e, f, g, h, T1, T2;
	unsigned long *q = W;

	if (sha_big_endian)
		memcpy(W, block, 64);
	else for (t = 0; t < 16; t++, q++) {
		*q = *block++; *q = (*q << 8) + *block++;
		*q = (*q << 8) + *block++; *q = (*q << 8) + *block++;
	}
	for (t = 16; t < 64; t++)
		W[t] = sigma1(W[t-2]) + W[t-7] + sigma0(W[t-15]) + W[t-16];
	a = p->H[0]; b = p->H[1]; c = p->H[2]; d = p->H[3];
	e = p->H[4]; f = p->H[5]; g = p->H[6]; h = p->H[7];
	for (t = 0; t < 64; t++) {
		T1 = h + SIGMA1(e) + Ch(e, f, g) + K256[t] + W[t];
		T2 = SIGMA0(a) + Maj(a, b, c);
		h = g; g = f; f = e; e = d + T1;
		d = c; c = b; b = a; a = T1 + T2;
	}
	p->H[0] += a; p->H[1] += b; p->H[2] += c; p->H[3] += d;
	p->H[4] += e; p->H[5] += f; p->H[6] += g; p->H[7] += h;
}

#include "sha64bit.c"

static void pad(s)
SHA *s;
{
	unsigned int lenpos, lhpos, llpos;

	lenpos = s->blocksize == SHA1_BLOCK_BITS ? 448 : 896;
	lhpos = s->blocksize == SHA1_BLOCK_BITS ? 56 : 120;
	llpos = s->blocksize == SHA1_BLOCK_BITS ? 60 : 124;
	SETBIT(s->block, s->blockcnt), s->blockcnt++;
	while (s->blockcnt > lenpos)
		if (s->blockcnt == s->blocksize)
			s->sha(s, s->block), s->blockcnt = 0;
		else
			CLRBIT(s->block, s->blockcnt), s->blockcnt++;
	while (s->blockcnt < lenpos)
		CLRBIT(s->block, s->blockcnt), s->blockcnt++;
	if (s->blocksize != SHA1_BLOCK_BITS) {
		ul2mem(s->block + 112, s->lenhh);
		ul2mem(s->block + 116, s->lenhl);
	}
	ul2mem(s->block + lhpos, s->lenlh);
	ul2mem(s->block + llpos, s->lenll);
	s->sha(s, s->block), s->blockcnt = 0;
}

static void digcpy(s)
SHA *s;
{
	unsigned int i;

	if (s->blocksize == SHA1_BLOCK_BITS)
		for (i = 0; i < 16; i++)
			ul2mem(s->digest + i * 4, s->H[i]);
	else
		digcpy64(s);
}

SHA *shaopen(alg)
int alg;
{
	SHA *s;

	if ((s = (SHA *) calloc(1, sizeof(SHA))) == NULL)
		return(NULL);
	s->alg = alg;
	if (alg == SHA1) {
		s->sha = sha1;
		memcpy(s->H, H01, sizeof(H01));
		s->blocksize = SHA1_BLOCK_BITS;
		s->digestlen = SHA1_DIGEST_BITS >> 3;
	}
	else if (alg == SHA256) {
		s->sha = sha256;
		memcpy(s->H, H0256, sizeof(H0256));
		s->blocksize = SHA256_BLOCK_BITS;
		s->digestlen = SHA256_DIGEST_BITS >> 3;
	}
	else if (!sha_384_512) {
		free(s);
		return(NULL);
	}
	else if (alg == SHA384) {
		s->sha = sha512;
		memcpy(s->H, H0384, sizeof(H0384));
		s->blocksize = SHA384_BLOCK_BITS;
		s->digestlen = SHA384_DIGEST_BITS >> 3;
	}
	else if (alg == SHA512) {
		s->sha = sha512;
		memcpy(s->H, H0512, sizeof(H0512));
		s->blocksize = SHA512_BLOCK_BITS;
		s->digestlen = SHA512_DIGEST_BITS >> 3;
	}
	else {
		free(s);
		return(NULL);
	}
	return(s);
}

static unsigned long shadirect(bitstr, bitcnt, s)
unsigned char *bitstr;
unsigned long bitcnt;
SHA *s;
{
	unsigned long savecnt = bitcnt;

	while (bitcnt >= s->blocksize) {
		s->sha(s, bitstr);
		bitstr += (s->blocksize >> 3);
		bitcnt -= s->blocksize;
	}
	if (bitcnt > 0) {
		memcpy(s->block, bitstr, BYTECNT(bitcnt));
		s->blockcnt = bitcnt;
	}
	return(savecnt);
}

static unsigned long shabytes(bitstr, bitcnt, s)
unsigned char *bitstr;
unsigned long bitcnt;
SHA *s;
{
	unsigned int offset;
	unsigned int numbits;
	unsigned long savecnt = bitcnt;

	offset = s->blockcnt >> 3;
	if (s->blockcnt + bitcnt >= s->blocksize) {
		numbits = s->blocksize - s->blockcnt;
		memcpy(s->block+offset, bitstr, numbits>>3);
		bitcnt -= numbits;
		bitstr += (numbits >> 3);
		s->sha(s, s->block), s->blockcnt = 0;
		shadirect(bitstr, bitcnt, s);
	}
	else {
		memcpy(s->block+offset, bitstr, BYTECNT(bitcnt));
		s->blockcnt += bitcnt;
	}
	return(savecnt);
}

static unsigned long shabits(bitstr, bitcnt, s)
unsigned char *bitstr;
unsigned long bitcnt;
SHA *s;
{
	unsigned int i;
	unsigned int gap;
	unsigned long numbits;
	static unsigned char buf[1<<12];
	unsigned int bufsize = sizeof(buf);
	unsigned long bufbits = bufsize << 3;
	unsigned int numbytes = BYTECNT(bitcnt);
	unsigned long savecnt = bitcnt;

	gap = 8 - s->blockcnt % 8;
	s->block[s->blockcnt>>3] &= ~0 << gap;
	s->block[s->blockcnt>>3] |= *bitstr >> (8 - gap);
	s->blockcnt += bitcnt < gap ? bitcnt : gap;
	if (bitcnt < gap)
		return(savecnt);
	if (s->blockcnt == s->blocksize)
		s->sha(s, s->block), s->blockcnt = 0;
	if ((bitcnt -= gap) == 0)
		return(savecnt);
	while (numbytes > bufsize) {
		for (i = 0; i < bufsize; i++)
			buf[i] = bitstr[i] << gap | bitstr[i+1] >> (8-gap);
		numbits = bitcnt < bufbits ? bitcnt : bufbits;
		shabytes(buf, numbits, s);
		bitcnt -= numbits, bitstr += bufsize, numbytes -= bufsize;
	}
	for (i = 0; i < numbytes - 1; i++)
		buf[i] = bitstr[i] << gap | bitstr[i+1] >> (8-gap);
	buf[numbytes-1] = bitstr[numbytes-1] << gap;
	shabytes(buf, bitcnt, s);
	return(savecnt);
}

unsigned long shawrite(bitstr, bitcnt, s)
unsigned char *bitstr;
unsigned long bitcnt;
SHA *s;
{
	if (bitcnt == 0)
		return(0);
	s->lenll += bitcnt;
	if (s->lenll < bitcnt) {
		s->lenlh++;
		if (s->lenlh == 0) {
			s->lenhl++;
			if (s->lenhl == 0)
				s->lenhh++;
		}
	}
	if (s->blockcnt == 0)
		return(shadirect(bitstr, bitcnt, s));
	else if (s->blockcnt % 8 == 0)
		return(shabytes(bitstr, bitcnt, s));
	else
		return(shabits(bitstr, bitcnt, s));
}

void shafinish(s)
SHA *s;
{
	pad(s);
}

unsigned char *shadigest(s)
SHA *s;
{
	digcpy(s);
	return(s->digest);
}

char *shahex(s)
SHA *s;
{
	int i;

	digcpy(s);
	s->hex[0] = '\0';
	for (i = 0; i < s->digestlen; i++)
		sprintf(s->hex+i*2, "%02x", s->digest[i]);
	return(s->hex);
}

static char map[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *enc3bytes(b, n)
unsigned char *b;
int n;
{
	unsigned char in[3] = {0, 0, 0};
	static char out[5];

	out[0] = '\0';
	if (n < 1 || n > 3)
		return(out);
	memcpy(in, b, n);
	out[0] = map[in[0] >> 2];
	out[1] = map[((in[0] & 0x03) << 4) | (in[1] >> 4)];
	out[2] = map[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
	out[3] = map[in[2] & 0x3f];
	out[n+1] = '\0';
	return(out);
}

char *shabase64(s)
SHA *s;
{
	int n;
	unsigned char *q;

	digcpy(s);
	s->base64[0] = '\0';
	for (n = s->digestlen, q = s->digest; n > 3; n -= 3, q += 3)
		strcat(s->base64, enc3bytes(q, 3));
	strcat(s->base64, enc3bytes(q, n));
	return(s->base64);
}

SHA *shadup(s)
SHA *s;
{
	SHA *p;

	if ((p = (SHA *) malloc(sizeof(SHA))) == NULL)
		return(NULL);
	memcpy(p, s, sizeof(SHA));
	return(p);
}

int shadump(file, s)
char *file;
SHA *s;
{
	int i;
	FILE *f;
	unsigned char *p;

	if (file == NULL || strlen(file) == 0)
		f = stdout;
	else if ((f = fopen(file, "w")) == NULL)
		return(0);
	fprintf(f, "alg:%d\n", s->alg);
	fprintf(f, "H");
	p = shadigest(s);
	if (s->alg <= SHA256) for (i = 0; i < 8; i++, p += 4)
		fprintf(f, ":%02x%02x%02x%02x", p[0],p[1],p[2],p[3]);
	else for (i = 0; i < 8; i++, p += 8)
		fprintf(f, ":%02x%02x%02x%02x%02x%02x%02x%02x",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
	fprintf(f, "\n");
	fprintf(f, "block");
	for (i = 0; i < sizeof(s->block); i++)
		fprintf(f, ":%02x", s->block[i]);
	fprintf(f, "\n");
	fprintf(f, "blockcnt:%u\n", s->blockcnt);
	fprintf(f, "lenhh:%lu\n", s->lenhh);
	fprintf(f, "lenhl:%lu\n", s->lenhl);
	fprintf(f, "lenlh:%lu\n", s->lenlh);
	fprintf(f, "lenll:%lu\n", s->lenll);
	if (f != stdout)
		fclose(f);
	return(1);
}

static int match(f, tag)
FILE *f;
char *tag;
{
	static char line[1<<10];

	while (fgets(line, sizeof(line), f) != NULL)
		if (line[0] == '#' || isspace(line[0]))
			continue;
		else
			return(strcmp(strtok(line, ":\n"), tag) == 0);
	return(0);
}

#define TYPE_C 1
#define TYPE_I 2
#define TYPE_L 3
#define TYPE_LL 4

static int loadval(f, tag, type, pval, num, base)
FILE *f;
char *tag;
int type;
void *pval;
int num;
int base;
{
	char *p;

	if (!match(f, tag))
		return(0);
	while (num-- > 0) {
		if ((p = strtok(NULL, ":\n")) == NULL)
			return(0);
		if (type == TYPE_C)
			*((unsigned char *) pval)++ = strtoul(p, NULL, base);
		else if (type == TYPE_I)
			*((unsigned int *) pval)++ = strtoul(p, NULL, base);
		else if (type == TYPE_L)
			*((unsigned long *) pval)++ = strtoul(p, NULL, base);
		else if (type == TYPE_LL)
			loadull(pval, p);
		else
			return(0);
	}
	return(1);
}

static SHA *closeall(f, s)
FILE *f;
SHA *s;
{
	if (f != NULL && f != stdin)
		fclose(f);
	if (s != NULL)
		shaclose(s);
	return(NULL);
}

SHA *shaload(file)
char *file;
{
	int alg;
	SHA *s;
	FILE *f;

	if (file == NULL || strlen(file) == 0)
		f = stdin;
	else if ((f = fopen(file, "r")) == NULL)
		return(NULL);
	if (!loadval(f, "alg", TYPE_I, &alg, 1, 10))
		return(closeall(f, NULL));
	if ((s = shaopen(alg)) == NULL)
		return(closeall(f, NULL));
	if (!loadval(f, "H", alg<=SHA256 ? TYPE_L : TYPE_LL, s->H, 8, 16))
		return(closeall(f, s));
	if (!loadval(f, "block", TYPE_C, s->block, s->blocksize>>3, 16))
		return(closeall(f, s));
	if (!loadval(f, "blockcnt", TYPE_I, &s->blockcnt, 1, 10))
		return(closeall(f, s));
	if (!loadval(f, "lenhh", TYPE_L, &s->lenhh, 1, 10))
		return(closeall(f, s));
	if (!loadval(f, "lenhl", TYPE_L, &s->lenhl, 1, 10))
		return(closeall(f, s));
	if (!loadval(f, "lenlh", TYPE_L, &s->lenlh, 1, 10))
		return(closeall(f, s));
	if (!loadval(f, "lenll", TYPE_L, &s->lenll, 1, 10))
		return(closeall(f, s));
	if (f != stdin)
		fclose(f);
	return(s);
}

int shaclose(s)
SHA *s;
{
	memset(s, 0, sizeof(SHA));
	free(s);
	return(0);
}

static unsigned char *shacomp(alg, fmt, bitstr, bitcnt)
int alg;
int fmt;
unsigned char *bitstr;
unsigned long bitcnt;
{
	SHA *s;
	static unsigned char digest[SHA_MAX_HEX_LEN+1];
	unsigned char *ret = digest;

	if ((s = shaopen(alg)) == NULL)
		return(NULL);
	shawrite(bitstr, bitcnt, s);
	shafinish(s);
	if (fmt == SHA_FMT_RAW)
		memcpy(digest, shadigest(s), s->digestlen);
	else if (fmt == SHA_FMT_HEX)
		strcpy((char *) digest, shahex(s));
	else if (fmt == SHA_FMT_BASE64)
		strcpy((char *) digest, shabase64(s));
	else
		ret = NULL;
	shaclose(s);
	return(ret);
}

#define SHA_DIRECT(type, name, alg, fmt) 			\
type name(bitstr, bitcnt)					\
unsigned char *bitstr;						\
unsigned long bitcnt;						\
{								\
	return((type) shacomp(alg, fmt, bitstr, bitcnt));	\
}

SHA_DIRECT(unsigned char *, sha1digest, SHA1, SHA_FMT_RAW)
SHA_DIRECT(char *, sha1hex, SHA1, SHA_FMT_HEX)
SHA_DIRECT(char *, sha1base64, SHA1, SHA_FMT_BASE64)

SHA_DIRECT(unsigned char *, sha256digest, SHA256, SHA_FMT_RAW)
SHA_DIRECT(char *, sha256hex, SHA256, SHA_FMT_HEX)
SHA_DIRECT(char *, sha256base64, SHA256, SHA_FMT_BASE64)

SHA_DIRECT(unsigned char *, sha384digest, SHA384, SHA_FMT_RAW)
SHA_DIRECT(char *, sha384hex, SHA384, SHA_FMT_HEX)
SHA_DIRECT(char *, sha384base64, SHA384, SHA_FMT_BASE64)

SHA_DIRECT(unsigned char *, sha512digest, SHA512, SHA_FMT_RAW)
SHA_DIRECT(char *, sha512hex, SHA512, SHA_FMT_HEX)
SHA_DIRECT(char *, sha512base64, SHA512, SHA_FMT_BASE64)
