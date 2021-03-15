/******************************************
 * Name: QuICrun.c
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

#define TAG_COMMENT '#'
#define TAG_OPEN '{'
#define TAG_CLOSE '}'
#define TAG_LOOP 'L'

#define TAG_EXEC '!'
#define TAG_ORACLE 'O'
#define TAG_FUNCTION 'F'
#define TAG_WRITE 'W'
#define TAG_READ 'R'

void usage(char * Cmd)
{
	fprintf(stderr,"Usage: %s <numQubits> <FileName>\n", Cmd);
	return;
}

int main(int argc, char * argv[])
{
	FILE * qFile;
	int numQubits = 0;
	int i;
	int done = 0;
	int loop = 0;
	long int loopPosn;
	char nextGate[MAX_QUBITS+1];
	char *AlgoStr;
	int mask = 1;
	QState * qList = NULL;
	float Probability = 1.0;
	char outString[10000];
	char algoBuf[10000];
	
	fprintf(stderr,"QuICrun version %1.2f\n",(float)qEmul_Version()/100.0);
	fprintf(stderr,"--------------------\n");

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

	if (!(qFile = fopen(argv[2],"r")))
	{
		fprintf(stderr,"Error: Unable to open <%s> for reading\n",argv[1]);
		return -1; 
	}

	qEmul_CreateList(&qList);
	while (!feof(qFile) && !(done))
	{
		memset(algoBuf,0,sizeof(algoBuf));
		fgets(algoBuf,sizeof(algoBuf)-1,qFile);

		AlgoStr = algoBuf;	

		if (*AlgoStr == TAG_COMMENT)
			continue;
		else if (*AlgoStr == TAG_OPEN)
		{
			if (AlgoStr[1] == TAG_LOOP)
			{
				sscanf((char*)&(AlgoStr[2]),"%d",&loop);
				loopPosn = ftell(qFile);
				continue;
			}
			else
			{
				fprintf(stderr,"unsupported tag %s\n",AlgoStr);
				continue;
			}
			
		}
		else if (*AlgoStr == TAG_CLOSE)
		{
			if (loop > 1)
			{
				loop--;
				fseek(qFile,loopPosn,SEEK_SET);
			}
			continue;
		}
		else if (*AlgoStr == TAG_EXEC)
		{
			if (AlgoStr[1] == TAG_ORACLE)
			{
				unsigned int oracleNum,oracleYBit;
				char oracleParams[sizeof(algoBuf)];

				sscanf(&(AlgoStr[2]),"%u %u %[^\n]s",&oracleNum,&oracleYBit,oracleParams);
				
				qEmul_oracle(oracleYBit,OracleList[oracleNum],oracleParams,&qList);
				continue;

			}
			else if (AlgoStr[1] == TAG_FUNCTION)
			{
				unsigned int functionNum, numBit;
				char functionParams[sizeof(algoBuf)];

				sscanf(&(AlgoStr[2]),"%u %u %[^\n]s",&functionNum,&numBit,functionParams);
				
				qEmul_function(numBit,FunctionList[functionNum],functionParams,&qList);
				continue;

			}
			else if (AlgoStr[1] == TAG_READ)
			{
				char fileName[sizeof(algoBuf)]; 

				sscanf(&(AlgoStr[2]),"%s",fileName);
				
				qEmul_Read(numQubits,fileName,&qList);
				continue;
			}
			else if (AlgoStr[1] == TAG_WRITE)
			{
				char fileName[sizeof(algoBuf)]; 

				sscanf(&(AlgoStr[2]),"%s",fileName);
				
				qEmul_Write(numQubits,fileName,qList);
				continue;
		
			}
			else
			{
				fprintf(stderr,"unsupported tag %s\n",AlgoStr);
				continue;
			}
		}

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
					fprintf(stdout,outString);
				}
				break;
					
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
					fprintf(stdout,outString);
				}
				else if (*AlgoStr == DELIM_END)
				{
					done = 1;
					continue;
				}
				
				AlgoStr++;
			}
	
		}
	
	}		
	fclose(qFile);
	if ((Probability = qEmul_GetProbability())< 1.0)
		fprintf(stdout,"probability of result is : %2.1f%% \n",Probability*100);
	memset(outString,0,sizeof(outString));
	qEmul_PrintList(numQubits,qList,outString,sizeof(outString)-1);
	fprintf(stdout,outString);
	qEmul_FreeList(qList);
	return 0;

	
}

