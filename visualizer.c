/******************************************
 * Name: visualizer.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "q_emul.h"

void usage(char * Cmd)
{

	printf("Usage: %s <numQubits> <algorithm>\n", Cmd);
	printf("e.g. %s 2 HH (to hadamard both qubits)\n     %s 2 HI,CN. (to create a bell state)\n",Cmd, Cmd);
	printf("     %s 3 HHX,ICN,CIN,HHH. or %s 3 HII,CNI,ICN. (to create the GHZ)\n",Cmd,Cmd);
	printf("     %s 3 HHI,IIX,IIH,III,CCN,III,IIH,IIX,HHI,XXI,IHI,CNI,IHI,XXI,HHI. (to perform a 2 qubit grover)\n",Cmd);
	printf("     %s 4 HHHI,IIIX,IIIH,IIII,CCCN,IIII,IIIH,IIIX,HHHI,XXXI,IIHI,CCNI,IIHI,XXXI,HHHI,IIIX,IIIH,IIII,CCCN,IIII,IIIH,IIIX,HHHI,XXXI,IIHI,CCNI,IIHI,XXXI,HHHI. (to perform a 3 qubit grover)\n", Cmd);
	printf("     %s 5 IIIIX,IHHHH,CIIIN,IHHHH,ImIII. (constant deutsche jozsa)\n", Cmd);
	printf("     %s 4 HHII,CINI,CIIN,ICNI,ICIN,IImm,HHII,mmII. (simon's algorithm for s=11)\n",Cmd);
	printf("     %s 5 IIIX,HHHH,CIIN,ICIN,IICN,HHHH,mIII (balanced deutsche jozsa)\n", Cmd);
	printf("     %s 8 XIXIXIII,nn***===. (to calculate 2 * 5) \n",Cmd);
	printf("     %s 12 XXHHXXIXIIII,nn^^%%%%====. (to calculate 3^i mod 13 where 0 < i < 3)\n",Cmd);
	printf("Notes: 1. 0 < numQubits < %d\n",MAX_QUBITS);
	printf("       2. Valid gates are H (Hadamard), I (Identity), X (Not), CN (Control-Not)\n"); 
	printf("          Valid Oracle operations are *,/,+,-,^,% (multiply, divide, add, subtract, power, modulo). Use n for 1st parameter, operation for 2nd parameter and = for result \n"); 
	printf("          Valid measurements are 0 (to continue when 0 is measured) and 1 (when 1 is measured) or use m for random measure\n"); 
	printf("          Others: d to delete a qubit, c to clone a qubit \n");
	printf("       3. Use _ as delimiter instead of , to print intermediate result, . to end\n"); 
}

int main(int argc, char * argv[])
{
	int numQubits = 0;
	int i;
	int done = 0;
	char nextGate[MAX_QUBITS+1];
	char * AlgoStr;
	int mask = 1;
	QState * qList = NULL;
	float Probability = 1.0;
	char outString[10000];
	char algoBuf[10000];
	
	printf("QuIC version %1.2f\n",(float)qEmul_Version()/100.0);
	printf("-----------------\n");

	if (argc != 3)
	{
		usage(argv[0]);
		return -1; 
	}

	numQubits = atoi(argv[1]);
	if ((numQubits <= 0) || (numQubits > MAX_QUBITS))
	{
		usage(argv[0]);
		return -1;
	}

	AlgoStr = argv[2];
	
	qEmul_CreateList(&qList);

	while (!done)
	{
		if (strlen(AlgoStr) < numQubits)
		{
			if (*AlgoStr == DELIM_END)
			{
				done = 1;
				continue;
			}
			else if (*AlgoStr == DELIM_PRINT)
			{
				memset(outString,0,sizeof(outString));
				qEmul_PrintList(numQubits, qList,outString,sizeof(outString)-strlen(outString)-1);
				printf(outString);
			}
			memset(algoBuf,0,sizeof(algoBuf));
			printf("more gates ? _ to print, . to end-> ");
			scanf("%s",algoBuf);
			AlgoStr = algoBuf;
			if (strlen(AlgoStr) < numQubits)
			{
				continue;
			}
				
		}
		memset(nextGate,0,sizeof(nextGate));
		memcpy(nextGate,AlgoStr,numQubits);	
		AlgoStr += numQubits;
		numQubits = qEmul_exec(numQubits,nextGate,&qList);
		if (*AlgoStr != '\0')
		{
			if (*AlgoStr == DELIM_PRINT)
			{
				memset(outString,0,sizeof(outString));
				sprintf(outString,"completed gate %s\n",nextGate);
				qEmul_PrintList(numQubits, qList,outString,sizeof(outString)-strlen(outString)-1);
				printf(outString);
			}
			else if (*AlgoStr == DELIM_END)
			{
				done = 1;
				continue;
			}
			
			AlgoStr++;
		}

	}

	printf("--- \n");
	if ((Probability = qEmul_GetProbability())< 1.0)
		printf("probability of result is : %2.1f%% \n",Probability*100);
	memset(outString,0,sizeof(outString));
	qEmul_PrintList(numQubits,qList,outString,sizeof(outString)-1);
	printf(outString);
	qEmul_FreeList(qList);
	
	return 0;

	
}

