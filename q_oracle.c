/******************************************
 * Name: q_oracle.c
 *
 * Author: Tan Teik Guan
 *
 * Copyright (c) 2020 Tan Teik Guan.
 * All rights reserved.
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Tan Teik Guan. The name of
 * Tan Teik Guan may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *****************************************/
#include <math.h>
#include <stdlib.h>
#include "q_emul.h"
#include "q_oracle.h"

unsigned long (* OracleList[MAX_ORACLE])(unsigned long *);
unsigned long * (* FunctionList[MAX_ORACLE])(int, unsigned long *, unsigned long *);
int setupDone = 0;

// internal functions
/*************************** HEADER FILES ***************************/
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
// #include "sha256.h"  // included below
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16- bit machines

typedef struct {
        BYTE data[64];
        WORD datalen;
        unsigned long long bitlen;
        WORD state[8];
} SHA256_CTX;

#define ENCRYPTION_MODE 1
#define DECRYPTION_MODE 0

typedef struct {
	unsigned char k[8];
	unsigned char c[4];
	unsigned char d[4];
} key_set;

typedef u_int32_t uint32_t;
typedef u_int8_t uint8_t;

static void sha256_init(SHA256_CTX *ctx);
static void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len);
static void sha256_final(SHA256_CTX *ctx, BYTE hash[]);
static void generate_sub_keys(unsigned char* main_key, key_set* key_sets);
static void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode);

static void subkeys(uint32_t k1[4], uint32_t k2[4], const uint32_t k[4]);
static void chaskey(uint8_t *tag, uint32_t taglen, const uint8_t *m, const uint32_t mlen, const uint32_t k[4], const uint32_t k1[4], const uint32_t k2[4]);

void qOracle_setup(void)
{

	if (setupDone)
		return;
	OracleList[ORACLE_MODEXP] = &Oracle_ModularExponentiation;
	OracleList[ORACLE_EVENMAN_MODEXP] = &Oracle_EvenMansour_ModExp;
	OracleList[ORACLE_SHA2256] = &Oracle_SHA256;
	OracleList[ORACLE_EVENMAN_SHA2256] = &Oracle_EvenMansour_SHA256;
	OracleList[ORACLE_CHASKEY12] = &Oracle_ChasKey12;

	FunctionList[FUNCTION_GAUSS_ELI] = &Function_Gaussian_Elimination_Binary;

	setupDone = 1;
}


unsigned long Oracle_ModularExponentiation(unsigned long * params)
{
	// param 1 = base 
	// param 2 = modulus 
	// value is used as the exponent  

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];

	unsigned long base,mod;
	unsigned long Yvalue;
	unsigned long i;

	Yvalue = value;
	value >>= numYQubits;
	base = params[2];
	mod = params[3];

	unsigned long ret = 1;

	while (value > 0)
	{
		if (value & 0x1)
			ret = (ret * base) % mod;
		base = (base * base) % mod;
		value >>= 1;
	}

	for (i=1;i<value;i++)
	{
		// to optimize
		ret = (ret * base) % mod;
	}

	//ret = (unsigned long) pow(base,value);
	//ret = ret % mod;

	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;

	ret = ret ^ Yvalue;
	ret += value;		
        return ret;
}

unsigned long Oracle_EvenMansour_ModExp(unsigned long * params)
{
	// param 1 = base 
	// param 2 = modulus 
	// param 3 = k1
	// param 4 = k2
	// value is used as the exponent  

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];
	unsigned long Yvalue;
	unsigned int base,mod,k1,k2;
	unsigned long i;

	Yvalue = value;
	value >>= numYQubits;
	base = params[2];
	mod = params[3];
	k1 = params[4];
	k2 = params[5];

	value = value ^ k1;
	
	unsigned long ret = 1;
	while (value > 0)
	{
		if (value & 0x1)
			ret = (ret * base) % mod;
		base = (base * base) % mod;
		value >>= 1;
	}

	ret = ret ^ k2;
	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;
	
	ret = ret ^ Yvalue;
	ret += value;		
	
	return ret;
}


unsigned long Oracle_SHA256(unsigned long * params)
{
	// param 1 = mask 

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];
	unsigned long mask = params[2];
	unsigned long Yvalue;
	unsigned long ret;
	unsigned char shahash[SHA256_BLOCK_SIZE];
	SHA256_CTX shactx;
	unsigned char HashStr[SHA256_BLOCK_SIZE];
	
	Yvalue = value;
	value >>= numYQubits;
	mask ^= value;
	memset(HashStr,0,sizeof(HashStr));
	memcpy(HashStr,&mask,sizeof(unsigned long));
	HashStr[SHA256_BLOCK_SIZE-1] = (unsigned char) numYQubits;

	sha256_init(&shactx);
	sha256_update(&shactx,HashStr,sizeof(HashStr));
	sha256_final(&shactx,shahash);

	memcpy(&ret,shahash,sizeof(unsigned long));
	ret &= (0xFFFFFFFFFFFFFFFF >> ((sizeof(unsigned long)*8)-numYQubits));

	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;

	ret = ret ^ Yvalue;
	ret += value;		
	
	return ret;
}

unsigned long Oracle_EvenMansour_SHA256(unsigned long * params)
{
	// param 1 = mask 
	// param 2 = k1
	// param 3 = k2

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];
	unsigned long mask = params[2];
	unsigned long Yvalue;
	unsigned long k1 = params[3];
	unsigned long k2 = params[4];
	unsigned long ret;
	unsigned char shahash[SHA256_BLOCK_SIZE];
	SHA256_CTX shactx;
	unsigned char HashStr[SHA256_BLOCK_SIZE];
	
	Yvalue = value;
	value >>= numYQubits;

	value = value ^ k1;
	mask ^= value;
	memset(HashStr,0,sizeof(HashStr));
	memcpy(HashStr,&mask,sizeof(unsigned long));
	HashStr[SHA256_BLOCK_SIZE-1] = (unsigned char) numYQubits;

	sha256_init(&shactx);
	sha256_update(&shactx,HashStr,sizeof(HashStr));
	sha256_final(&shactx,shahash);

	memcpy(&ret,shahash,sizeof(unsigned long));
	ret &= (0xFFFFFFFFFFFFFFFF >> ((sizeof(unsigned long)*8)-numYQubits));

	ret = ret ^ k2;

	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;

	ret = ret ^ Yvalue;
	ret += value;		
	
	return ret;
}

unsigned long Oracle_ChasKey12(unsigned long * params)
{
	// param 1 = keyl  
	// param 2 = keyr  
	// param 3 = messagel

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];
	unsigned long keyl = params[2];
	unsigned long keyr = params[3];
	unsigned long messagel = params[4];
	unsigned long Yvalue;
	unsigned long ret;
	uint32_t k[4],k1[4],k2[4];
	uint8_t m[16];
	uint8_t tag[16];
	
	Yvalue = value;
	value >>= numYQubits;

	memcpy(m,(unsigned char *) &messagel,sizeof(unsigned long));
	memcpy(&m[8],(unsigned char *) &value,sizeof(unsigned long));
	memcpy(k,(unsigned char *) &keyl,sizeof(keyl));
	memcpy(&k[2],(unsigned char *) &keyr,sizeof(keyr));

	subkeys(k1,k2,k);
	chaskey(tag,sizeof(tag),m,sizeof(m),(uint32_t *)k,k1,k2);

	memcpy(&ret,tag,sizeof(unsigned long)); 
	ret &= (0xFFFFFFFFFFFFFFFF >> ((sizeof(unsigned long)*8)-numYQubits));

	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;

	ret = ret ^ Yvalue;
	ret += value;		
	
	return ret;
}

unsigned long Oracle_DES64(unsigned long * params)
{
	// param 1 = key 
	// param 2 = operation {0 = encrypt, 1 = decrypt, default = encrypt}

	unsigned long numYQubits = params[0];
	unsigned long value = params[1];
	unsigned long key = params[2];
	unsigned long operation = params[3];
	unsigned long Yvalue;
	unsigned long ret;
	unsigned char deskey[8];
	unsigned char inblock[8];
	unsigned char outblock[8];
	key_set key_sets[17];

	memset(deskey,0,sizeof(deskey));
	memset(inblock,0,sizeof(inblock));
	memset(outblock,0,sizeof(outblock));
	memset(key_sets,0,sizeof(key_sets));

	memcpy(deskey,(unsigned char *) &key,sizeof(deskey));
	if (operation != 1)
		operation = 0;

	Yvalue = value;
	value >>= numYQubits;

	generate_sub_keys(deskey,key_sets);

	memcpy(inblock,(unsigned char *)&value,sizeof(inblock));
	process_message(inblock,outblock,key_sets,operation);

	memcpy(&ret,outblock,sizeof(unsigned long));
	ret &= (0xFFFFFFFFFFFFFFFF >> ((sizeof(unsigned long)*8)-numYQubits));

	value = params[1] >> numYQubits;
	value <<= numYQubits;
	Yvalue -= value;

	ret = ret ^ Yvalue;
	ret += value;		
	
	return ret;
}

unsigned long * Function_Gaussian_Elimination_Binary(int numBits, unsigned long * functionParams, unsigned long * qubitValues)
{
	int i,j;
	int found, rowDone, numSolutions;
	unsigned long tempL1,tempL2;
	unsigned long solutionMask;
	unsigned long * solutionList;

/*
printf("qubitvalues : ");
for (i=0;i<qubitValues[0];i++)
	printf(" [%d] %lu ", i, qubitValues[i+1]);
printf("\n");
*/
	if (qubitValues)
	{
		// look for all zeros
		j = 0;
		for (i=0;i<qubitValues[0];i++)
		{	
			if (qubitValues[i+1] != 0)
				j = qubitValues[i+1];
		}
		if (j==0) 
		{
			qubitValues[0] = 1;
			qubitValues[1] = 0;
			return qubitValues;
		}

		rowDone = 0;
		for (j = 0; j < numBits; j++)
		{
			tempL1 = 1 << j;
			found = 0;
			for (i=rowDone;i<qubitValues[0];i++)
			{
				if (qubitValues[i+1] & tempL1) // found
				{
					if (!found)
					{
						if (i != rowDone)  // need to swap the values to bring the found value forward
						{
							tempL2 = qubitValues[rowDone+1];
							qubitValues[rowDone+1] = qubitValues[i+1];
							qubitValues[i+1] = tempL2;
						}
						found = 1;
						rowDone++;
					}
				}
			}
			if (found)
			{
				for (i=0;i<qubitValues[0];i++)
				{
					if ((i+1) == rowDone)
						continue;
					else
					{
						if (qubitValues[i+1] & tempL1)
						{
							qubitValues[i+1] ^= qubitValues[rowDone];				
						}
					}
				}
			}
		}
		// elimination done, now to solve
		// theory: 
		// 1. if only 1 bit is set, then that position should be 0
		// 2. if > 1 bits s set, then all positions should be 1 
/*
printf("qubitvalues : ");
for (i=0;i<qubitValues[0];i++)
	printf(" [%d] %lu ", i, qubitValues[i+1]);
printf("\n");
*/


		tempL2 = 0xFFFFFFFFFFFFFFFF >> (64 - numBits) ;  // assuming tempL2 is unsigned long
		tempL1 = 1;
		solutionMask = 0;
		rowDone = 0;
		for (i=0;i<numBits;i++)	
		{
			if ((tempL1 & qubitValues[rowDone+1]) > 0)
			{
				if ((tempL1 & qubitValues[rowDone+1]) == qubitValues[rowDone+1])  // only 1 bit set 
				{
					tempL2 ^= tempL1;
				}
				rowDone++;
			}
			else
				solutionMask |= tempL1; // possibly more than 1 solution

			tempL1<<=1;
		}
				
		qubitValues[1] = tempL2;  // store the first solution

		if (solutionMask > 0)
		{
			tempL2 = solutionMask;
			numSolutions = 1;
			while (tempL2 > 0) // count number of bits
			{
				if ((tempL2 & 0x01) > 0)
					numSolutions<<=1;
				tempL2>>=1;
			}
			numSolutions--; 		// we can ignore mask 00000...000

			solutionList = (unsigned long *) malloc(numSolutions * sizeof(unsigned long));
			if (!solutionList)
			{
				fprintf(stderr,"error: unable to malloc solutionList\n");
				return NULL;
			}
			for (i=0;i<numSolutions;i++)
				solutionList[i] = 0;
			tempL1 = 1;
			for (i=0;i<numBits;i++)
			{
				if (tempL1 & solutionMask)
				{
					for (j=0;j<numSolutions;j++)
					{
						if ((j+1) & tempL1)
						{
							solutionList[j] |= tempL1;
						}
					}
				}
				tempL1<<=1;
			}
			j = 1;
			for (i=0;i<numSolutions;i++)
			{
				tempL2 = solutionList[i] ^ qubitValues[1];
				if (tempL2)
				{
					if (j < qubitValues[0])
					{
						j++;
						qubitValues[j] = tempL2;
					}
					else
						break;  // no more space to return more solutions
				}
			}	
			free(solutionList);
			qubitValues[0] = j;
		}

			
	}	
	return qubitValues;
}

// internal functions

/*********************************************************************
* Filename:   sha256.c
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Implementation of the SHA-256 hashing algorithm.
              SHA-256 is one of the three algorithms in the SHA2
              specification. The others, SHA-384 and SHA-512, are not
              offered in this implementation.
              Algorithm specification can be found here:
               * http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              This implementation uses little endian byte order.
*********************************************************************/


/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

/**************************** VARIABLES *****************************/
static const WORD k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*********************** FUNCTION DEFINITIONS ***********************/
static void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
	WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
	WORD i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

static void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
	WORD i;

	i = ctx->datalen;

	// Pad whatever data is left in the buffer.
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	// Append to the padding the total message's length in bits and transform.
	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	// Since this implementation uses little endian byte ordering and SHA uses big endian,
	// reverse all the bytes when copying the final state to the output hash.
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}


/*
   Chaskey-12 reference C implementation

   Written in 2015 by Nicky Mouha, based on Chaskey

   To the extent possible under law, the author has dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
   
   NOTE: This implementation assumes a little-endian architecture
         that does not require aligned memory accesses.
*/

#define ROTL(x,b) (uint32_t)( ((x) >> (32 - (b))) | ( (x) << (b)) )

#define ROUND \
  do { \
    v[0] += v[1]; v[1]=ROTL(v[1], 5); v[1] ^= v[0]; v[0]=ROTL(v[0],16); \
    v[2] += v[3]; v[3]=ROTL(v[3], 8); v[3] ^= v[2]; \
    v[0] += v[3]; v[3]=ROTL(v[3],13); v[3] ^= v[0]; \
    v[2] += v[1]; v[1]=ROTL(v[1], 7); v[1] ^= v[2]; v[2]=ROTL(v[2],16); \
  } while(0)
  
#define PERMUTE \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND; \
  ROUND;

const volatile uint32_t C[2] = { 0x00, 0x87 };

#define TIMESTWO(out,in) \
  do { \
    out[0] = (in[0] << 1) ^ C[in[3] >> 31]; \
    out[1] = (in[1] << 1) | (in[0] >> 31); \
    out[2] = (in[2] << 1) | (in[1] >> 31); \
    out[3] = (in[3] << 1) | (in[2] >> 31); \
  } while(0)
    
static void subkeys(uint32_t k1[4], uint32_t k2[4], const uint32_t k[4]) {
  TIMESTWO(k1,k);
  TIMESTWO(k2,k1);
}

static void chaskey(uint8_t *tag, uint32_t taglen, const uint8_t *m, const uint32_t mlen, const uint32_t k[4], const uint32_t k1[4], const uint32_t k2[4]) {

  const uint32_t *M = (uint32_t*) m;
  const uint32_t *end = M + (((mlen-1)>>4)<<2); /* pointer to last message block */

  const uint32_t *l;
  uint8_t lb[16];
  const uint32_t *lastblock;
  uint32_t v[4];
  
  int i;
  uint8_t *p;
  
  v[0] = k[0];
  v[1] = k[1];
  v[2] = k[2];
  v[3] = k[3];

  if (mlen != 0) {
    for ( ; M != end; M += 4 ) {
#ifdef DEBUG
      printf("(%3d) v[0] %08x\n", mlen, v[0]);
      printf("(%3d) v[1] %08x\n", mlen, v[1]);
      printf("(%3d) v[2] %08x\n", mlen, v[2]);
      printf("(%3d) v[3] %08x\n", mlen, v[3]);
      printf("(%3d) compress %08x %08x %08x %08x\n", mlen, m[0], m[1], m[2], m[3]);
#endif
      v[0] ^= M[0];
      v[1] ^= M[1];
      v[2] ^= M[2];
      v[3] ^= M[3];
      PERMUTE;
    }
  }

  if ((mlen != 0) && ((mlen & 0xF) == 0)) {
    l = k1;
    lastblock = M;
  } else {
    l = k2;
    p = (uint8_t*) M;
    i = 0;
    for ( ; p != m + mlen; p++,i++) {
      lb[i] = *p;
    }
    lb[i++] = 0x01; /* padding bit */
    for ( ; i != 16; i++) {
      lb[i] = 0;
    }
    lastblock = (uint32_t*) lb;
  }

#ifdef DEBUG
  printf("(%3d) v[0] %08x\n", mlen, v[0]);
  printf("(%3d) v[1] %08x\n", mlen, v[1]);
  printf("(%3d) v[2] %08x\n", mlen, v[2]);
  printf("(%3d) v[3] %08x\n", mlen, v[3]);
  printf("(%3d) last block %08x %08x %08x %08x\n", mlen, lastblock[0], lastblock[1], lastblock[2], lastblock[3]);
#endif
  v[0] ^= lastblock[0];
  v[1] ^= lastblock[1];
  v[2] ^= lastblock[2];
  v[3] ^= lastblock[3];

  v[0] ^= l[0];
  v[1] ^= l[1];
  v[2] ^= l[2];
  v[3] ^= l[3];

  PERMUTE;

#ifdef DEBUG
  printf("(%3d) v[0] %08x\n", mlen, v[0]);
  printf("(%3d) v[1] %08x\n", mlen, v[1]);
  printf("(%3d) v[2] %08x\n", mlen, v[2]);
  printf("(%3d) v[3] %08x\n", mlen, v[3]);
#endif

  v[0] ^= l[0];
  v[1] ^= l[1];
  v[2] ^= l[2];
  v[3] ^= l[3];

  memcpy(tag,v,taglen);

}

    
/*******
  DES implementation - from https://github.com/tarequeh/DES

The MIT License

Copyright (c) 2010 Tareque Hossain <tareque@codexn.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*******/


int initial_key_permutaion[] = {57, 49,  41, 33,  25,  17,  9,
				 1, 58,  50, 42,  34,  26, 18,
				10,  2,  59, 51,  43,  35, 27,
				19, 11,   3, 60,  52,  44, 36,
				63, 55,  47, 39,  31,  23, 15,
				 7, 62,  54, 46,  38,  30, 22,
				14,  6,  61, 53,  45,  37, 29,
				21, 13,   5, 28,  20,  12,  4};

int initial_message_permutation[] =   {58, 50, 42, 34, 26, 18, 10, 2,
					60, 52, 44, 36, 28, 20, 12, 4,
					62, 54, 46, 38, 30, 22, 14, 6,
					64, 56, 48, 40, 32, 24, 16, 8,
					57, 49, 41, 33, 25, 17,  9, 1,
					59, 51, 43, 35, 27, 19, 11, 3,
					61, 53, 45, 37, 29, 21, 13, 5,
					63, 55, 47, 39, 31, 23, 15, 7};

int key_shift_sizes[] = {-1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

int sub_key_permutation[] =    {14, 17, 11, 24,  1,  5,
				 3, 28, 15,  6, 21, 10,
				23, 19, 12,  4, 26,  8,
				16,  7, 27, 20, 13,  2,
				41, 52, 31, 37, 47, 55,
				30, 40, 51, 45, 33, 48,
				44, 49, 39, 56, 34, 53,
				46, 42, 50, 36, 29, 32};

int message_expansion[] = 	 {32,  1,  2,  3,  4,  5,
				 4,  5,  6,  7,  8,  9,
				 8,  9, 10, 11, 12, 13,
				12, 13, 14, 15, 16, 17,
				16, 17, 18, 19, 20, 21,
				20, 21, 22, 23, 24, 25,
				24, 25, 26, 27, 28, 29,
				28, 29, 30, 31, 32,  1};

int S1[] = {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
	 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
	 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13};

int S2[] = {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9};

int S3[] = {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12};

int S4[] = { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14};

int S5[] = { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3};

int S6[] = {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13};

int S7[] = { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12};

int S8[] = {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11};

int right_sub_message_permutation[] =    {16,  7, 20, 21,
					29, 12, 28, 17,
					 1, 15, 23, 26,
					 5, 18, 31, 10,
					 2,  8, 24, 14,
					32, 27,  3,  9,
					19, 13, 30,  6,
					22, 11,  4, 25};

int final_message_permutation[] =  {40,  8, 48, 16, 56, 24, 64, 32,
				39,  7, 47, 15, 55, 23, 63, 31,
				38,  6, 46, 14, 54, 22, 62, 30,
				37,  5, 45, 13, 53, 21, 61, 29,
				36,  4, 44, 12, 52, 20, 60, 28,
				35,  3, 43, 11, 51, 19, 59, 27,
				34,  2, 42, 10, 50, 18, 58, 26,
				33,  1, 41,  9, 49, 17, 57, 25};


static void print_char_as_binary(char input) {
	int i;
	for (i=0; i<8; i++) {
		char shift_byte = 0x01 << (7-i);
		if (shift_byte & input) {
			printf("1");
		} else {
			printf("0");
		}
	}
}

static void print_key_set(key_set key_set){
	int i;
	printf("K: \n");
	for (i=0; i<8; i++) {
		printf("%02X : ", key_set.k[i]);
		print_char_as_binary(key_set.k[i]);
		printf("\n");
	}
	printf("\nC: \n");

	for (i=0; i<4; i++) {
		printf("%02X : ", key_set.c[i]);
		print_char_as_binary(key_set.c[i]);
		printf("\n");
	}
	printf("\nD: \n");

	for (i=0; i<4; i++) {
		printf("%02X : ", key_set.d[i]);
		print_char_as_binary(key_set.d[i]);
		printf("\n");
	}
	printf("\n");
}

static void generate_sub_keys(unsigned char* main_key, key_set* key_sets) {
	int i, j;
	int shift_size;
	unsigned char shift_byte, first_shift_bits, second_shift_bits, third_shift_bits, fourth_shift_bits;

	for (i=0; i<8; i++) {
		key_sets[0].k[i] = 0;
	}

	for (i=0; i<56; i++) {
		shift_size = initial_key_permutaion[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= main_key[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		key_sets[0].k[i/8] |= (shift_byte >> i%8);
	}

	for (i=0; i<3; i++) {
		key_sets[0].c[i] = key_sets[0].k[i];
	}

	key_sets[0].c[3] = key_sets[0].k[3] & 0xF0;

	for (i=0; i<3; i++) {
		key_sets[0].d[i] = (key_sets[0].k[i+3] & 0x0F) << 4;
		key_sets[0].d[i] |= (key_sets[0].k[i+4] & 0xF0) >> 4;
	}

	key_sets[0].d[3] = (key_sets[0].k[6] & 0x0F) << 4;


	for (i=1; i<17; i++) {
		for (j=0; j<4; j++) {
			key_sets[i].c[j] = key_sets[i-1].c[j];
			key_sets[i].d[j] = key_sets[i-1].d[j];
		}

		shift_size = key_shift_sizes[i];
		if (shift_size == 1){
			shift_byte = 0x80;
		} else {
			shift_byte = 0xC0;
		}

		// Process C
		first_shift_bits = shift_byte & key_sets[i].c[0];
		second_shift_bits = shift_byte & key_sets[i].c[1];
		third_shift_bits = shift_byte & key_sets[i].c[2];
		fourth_shift_bits = shift_byte & key_sets[i].c[3];

		key_sets[i].c[0] <<= shift_size;
		key_sets[i].c[0] |= (second_shift_bits >> (8 - shift_size));

		key_sets[i].c[1] <<= shift_size;
		key_sets[i].c[1] |= (third_shift_bits >> (8 - shift_size));

		key_sets[i].c[2] <<= shift_size;
		key_sets[i].c[2] |= (fourth_shift_bits >> (8 - shift_size));

		key_sets[i].c[3] <<= shift_size;
		key_sets[i].c[3] |= (first_shift_bits >> (4 - shift_size));

		// Process D
		first_shift_bits = shift_byte & key_sets[i].d[0];
		second_shift_bits = shift_byte & key_sets[i].d[1];
		third_shift_bits = shift_byte & key_sets[i].d[2];
		fourth_shift_bits = shift_byte & key_sets[i].d[3];

		key_sets[i].d[0] <<= shift_size;
		key_sets[i].d[0] |= (second_shift_bits >> (8 - shift_size));

		key_sets[i].d[1] <<= shift_size;
		key_sets[i].d[1] |= (third_shift_bits >> (8 - shift_size));

		key_sets[i].d[2] <<= shift_size;
		key_sets[i].d[2] |= (fourth_shift_bits >> (8 - shift_size));

		key_sets[i].d[3] <<= shift_size;
		key_sets[i].d[3] |= (first_shift_bits >> (4 - shift_size));

		for (j=0; j<48; j++) {
			shift_size = sub_key_permutation[j];
			if (shift_size <= 28) {
				shift_byte = 0x80 >> ((shift_size - 1)%8);
				shift_byte &= key_sets[i].c[(shift_size - 1)/8];
				shift_byte <<= ((shift_size - 1)%8);
			} else {
				shift_byte = 0x80 >> ((shift_size - 29)%8);
				shift_byte &= key_sets[i].d[(shift_size - 29)/8];
				shift_byte <<= ((shift_size - 29)%8);
			}

			key_sets[i].k[j/8] |= (shift_byte >> j%8);
		}
	}
}

static void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode) {
	int i, k;
	int shift_size;
	unsigned char shift_byte;

	unsigned char initial_permutation[8];
	memset(initial_permutation, 0, 8);
	memset(processed_piece, 0, 8);

	for (i=0; i<64; i++) {
		shift_size = initial_message_permutation[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= message_piece[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		initial_permutation[i/8] |= (shift_byte >> i%8);
	}

	unsigned char l[4], r[4];
	for (i=0; i<4; i++) {
		l[i] = initial_permutation[i];
		r[i] = initial_permutation[i+4];
	}

	unsigned char ln[4], rn[4], er[6], ser[4];

	int key_index;
	for (k=1; k<=16; k++) {
		memcpy(ln, r, 4);

		memset(er, 0, 6);

		for (i=0; i<48; i++) {
			shift_size = message_expansion[i];
			shift_byte = 0x80 >> ((shift_size - 1)%8);
			shift_byte &= r[(shift_size - 1)/8];
			shift_byte <<= ((shift_size - 1)%8);

			er[i/8] |= (shift_byte >> i%8);
		}

		if (mode == DECRYPTION_MODE) {
			key_index = 17 - k;
		} else {
			key_index = k;
		}

		for (i=0; i<6; i++) {
			er[i] ^= key_sets[key_index].k[i];
		}

		unsigned char row, column;

		for (i=0; i<4; i++) {
			ser[i] = 0;
		}

		// 0000 0000 0000 0000 0000 0000
		// rccc crrc cccr rccc crrc cccr

		// Byte 1
		row = 0;
		row |= ((er[0] & 0x80) >> 6);
		row |= ((er[0] & 0x04) >> 2);

		column = 0;
		column |= ((er[0] & 0x78) >> 3);

		ser[0] |= ((unsigned char)S1[row*16+column] << 4);

		row = 0;
		row |= (er[0] & 0x02);
		row |= ((er[1] & 0x10) >> 4);

		column = 0;
		column |= ((er[0] & 0x01) << 3);
		column |= ((er[1] & 0xE0) >> 5);

		ser[0] |= (unsigned char)S2[row*16+column];

		// Byte 2
		row = 0;
		row |= ((er[1] & 0x08) >> 2);
		row |= ((er[2] & 0x40) >> 6);

		column = 0;
		column |= ((er[1] & 0x07) << 1);
		column |= ((er[2] & 0x80) >> 7);

		ser[1] |= ((unsigned char)S3[row*16+column] << 4);

		row = 0;
		row |= ((er[2] & 0x20) >> 4);
		row |= (er[2] & 0x01);

		column = 0;
		column |= ((er[2] & 0x1E) >> 1);

		ser[1] |= (unsigned char)S4[row*16+column];

		// Byte 3
		row = 0;
		row |= ((er[3] & 0x80) >> 6);
		row |= ((er[3] & 0x04) >> 2);

		column = 0;
		column |= ((er[3] & 0x78) >> 3);

		ser[2] |= ((unsigned char)S5[row*16+column] << 4);

		row = 0;
		row |= (er[3] & 0x02);
		row |= ((er[4] & 0x10) >> 4);

		column = 0;
		column |= ((er[3] & 0x01) << 3);
		column |= ((er[4] & 0xE0) >> 5);

		ser[2] |= (unsigned char)S6[row*16+column];

		// Byte 4
		row = 0;
		row |= ((er[4] & 0x08) >> 2);
		row |= ((er[5] & 0x40) >> 6);

		column = 0;
		column |= ((er[4] & 0x07) << 1);
		column |= ((er[5] & 0x80) >> 7);

		ser[3] |= ((unsigned char)S7[row*16+column] << 4);

		row = 0;
		row |= ((er[5] & 0x20) >> 4);
		row |= (er[5] & 0x01);

		column = 0;
		column |= ((er[5] & 0x1E) >> 1);

		ser[3] |= (unsigned char)S8[row*16+column];

		for (i=0; i<4; i++) {
			rn[i] = 0;
		}

		for (i=0; i<32; i++) {
			shift_size = right_sub_message_permutation[i];
			shift_byte = 0x80 >> ((shift_size - 1)%8);
			shift_byte &= ser[(shift_size - 1)/8];
			shift_byte <<= ((shift_size - 1)%8);

			rn[i/8] |= (shift_byte >> i%8);
		}

		for (i=0; i<4; i++) {
			rn[i] ^= l[i];
		}

		for (i=0; i<4; i++) {
			l[i] = ln[i];
			r[i] = rn[i];
		}
	}

	unsigned char pre_end_permutation[8];
	for (i=0; i<4; i++) {
		pre_end_permutation[i] = r[i];
		pre_end_permutation[4+i] = l[i];
	}

	for (i=0; i<64; i++) {
		shift_size = final_message_permutation[i];
		shift_byte = 0x80 >> ((shift_size - 1)%8);
		shift_byte &= pre_end_permutation[(shift_size - 1)/8];
		shift_byte <<= ((shift_size - 1)%8);

		processed_piece[i/8] |= (shift_byte >> i%8);
	}
}

	

	
