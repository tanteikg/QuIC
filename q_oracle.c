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

void qOracle_setup(void)
{

	if (setupDone)
		return;
	OracleList[ORACLE_MODEXP] = &Oracle_ModularExponentiation;
	OracleList[ORACLE_EVENMAN_MODEXP] = &Oracle_EvenMansour_ModExp;

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

	unsigned int base,mod;
	unsigned long Yvalue;

	Yvalue = value;
	value >>= numYQubits;
	base = params[2];
	mod = params[3];

	unsigned long ret = (unsigned long) pow((double)base,(double)value);
	ret = ret % mod;
	
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

	Yvalue = value;
	value >>= numYQubits;
	base = params[2];
	mod = params[3];
	k1 = params[4];
	k2 = params[5];

	value = value ^ k1;
	
	unsigned long ret = (unsigned long) pow((double)base,(double)value);
	ret = ret % mod;
	ret = ret ^ k2;
	
	value = params[1];
	value >>= numYQubits;
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

	if (qubitValues)
	{
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

