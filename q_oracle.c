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

static void sha256_init(SHA256_CTX *ctx);
static void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len);
static void sha256_final(SHA256_CTX *ctx, BYTE hash[]);

void qOracle_setup(void)
{

	if (setupDone)
		return;
	OracleList[ORACLE_MODEXP] = &Oracle_ModularExponentiation;
	OracleList[ORACLE_EVENMAN_MODEXP] = &Oracle_EvenMansour_ModExp;
	OracleList[ORACLE_SHA2256] = &Oracle_SHA256;
	OracleList[ORACLE_EVENMAN_SHA2256] = &Oracle_EvenMansour_SHA256;

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



	

	
