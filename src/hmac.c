/*
 * hmac.c: routines to compute HMAC-SHA-1/224/256/384/512 digests
 *
 * Ref: FIPS PUB 198 The Keyed-Hash Message Authentication Code
 *
 * Copyright (C) 2003 Mark Shelor, All Rights Reserved
 *
 * Version: 4.2.1
 * Sat Jan 24 00:56:54 MST 2004
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hmac.h"
#include "sha.h"

HMAC *hmacopen(alg, key, keylen)
int alg;
unsigned char *key;
unsigned int keylen;
{
	int i;
	HMAC *h;

	SHA_newz(0, h, 1, HMAC);
	if (h == NULL)
		return(NULL);
	if ((h->isha = shaopen(alg)) == NULL) {
		SHA_free(h);
		return(NULL);
	}
	if ((h->osha = shaopen(alg)) == NULL) {
		shaclose(h->isha);
		SHA_free(h);
		return(NULL);
	}
	if (keylen <= sizeof(h->key))
		memcpy(h->key, key, keylen);
	else {
		if ((h->ksha = shaopen(alg)) == NULL) {
			shaclose(h->isha);
			shaclose(h->osha);
			SHA_free(h);
			return(NULL);
		}
		shawrite(key, keylen * 8, h->ksha);
		shafinish(h->ksha);
		memcpy(h->key, shadigest(h->ksha), h->ksha->digestlen);
		shaclose(h->ksha);
	}
	for (i = 0; i < 64; i++)
		h->key[i] ^= 0x5c;
	shawrite(h->key, 512, h->osha);
	for (i = 0; i < 64; i++)
		h->key[i] ^= (0x5c ^ 0x36);
	shawrite(h->key, 512, h->isha);
	memset(h->key, 0, sizeof(h->key));
	return(h);
}

unsigned long hmacwrite(bitstr, bitcnt, h)
unsigned char *bitstr;
unsigned long bitcnt;
HMAC *h;
{
	return(shawrite(bitstr, bitcnt, h->isha));
}

void hmacfinish(h)
HMAC *h;
{
	shafinish(h->isha);
	shawrite(shadigest(h->isha), h->isha->digestlen * 8, h->osha);
	shaclose(h->isha);
	shafinish(h->osha);
}

unsigned char *hmacdigest(h)
HMAC *h;
{
	return(shadigest(h->osha));
}

char *hmachex(h)
HMAC *h;
{
	return(shahex(h->osha));
}

char *hmacbase64(h)
HMAC *h;
{
	return(shabase64(h->osha));
}

int hmacclose(h)
HMAC *h;
{
	shaclose(h->osha);
	if (h != NULL) {
		memset(h, 0, sizeof(HMAC));
		SHA_free(h);
	}
	return(0);
}
