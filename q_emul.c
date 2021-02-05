/******************************************
 * Name: q_emul.c
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
#include <sys/time.h>
#include <math.h>
#include "q_emul.h"

static double Probability = 1.0;
#define PI (2 * acos(0.0))

double qEmul_GetProbability(void)
{
	return Probability;
}

void qEmul_ResetProbability(void)
{
	Probability = 1.0;
}

int qEmul_Version(void)
{
	return Q_VERSION;
}

double complex qEmul_Count2List(QState * qList)
{
        QState * tempPtr;
        double complex count = 0.0;

        tempPtr = qList;
        while (tempPtr)
        {
                count += fabs(tempPtr->Count*tempPtr->Count);
                tempPtr = tempPtr->next;
        }
        return count;
}

static int printBinStr(char * binString, int numQubits, unsigned long value)
{
	int i;

	for (i = 0; i < numQubits;i++)
	{
		binString[numQubits-i-1] = (value % 2)? '1':'0';
		value >>= 1;
	}
	binString[numQubits] = 0;

}

int qEmul_PrintList(int numQubits, QState * qList, char * outStr, int outStrLen)
{
	QState * temp;
	char tempStr[1000];
	char tempBinStr[MAX_QUBITS+1];	

	if (!outStr)
		return -1;
	temp = qList;
	if (!temp)
	{
		sprintf(tempStr,"Empty list\n");
		if (strlen(tempStr) > outStrLen - strlen(outStr))
			return -1;
		else
		{
			strcat(outStr,tempStr);
			return 0;
		}
	}
	while (temp)
	{
		printBinStr(tempBinStr, numQubits, temp->Value);
		sprintf(tempStr,"Value %c[%s] Count[%02.0f%s]\n",(creal(temp->Count)<0.0)?'-':' ', tempBinStr, (fabs(creal(temp->Count)) > fabs(cimag(temp->Count)))? fabs(creal(temp->Count)):fabs(cimag(temp->Count)),(fabs(creal(temp->Count)) > fabs(cimag(temp->Count)))? "":"i");
		temp = temp->next;
		if (strlen(tempStr) > outStrLen - strlen(outStr))
			return -1;
		else
		{
			strcat(outStr,tempStr);
		}
		
	}
	return 0;
} 

void qEmul_FreeList(QState * qList)
{
	QState * temp;

	while (qList)
	{
		temp = qList;
		qList = qList->next;
		free(temp);
	}
}

void qEmul_CreateList(QState ** qList)
{
	QState *temp;
	temp = malloc(sizeof(QState));

	if (!temp)
	{
		fprintf(stderr,"Error in malloc qList\n");
		exit(-1);
	}
	temp->Value = 0;
	temp->Count = 1.0;
	temp->next = NULL; 
	Probability = 1.0;
	srand(time(NULL));
	*qList = temp;
	qOracle_setup();
}

static void InsertInList(QState * newState, QState ** qList)
{
	QState * temp;
	QState * tmpPtr, * prevPtr;

	if (!newState)
		return;

	if (!*qList)
	{
		*qList = malloc(sizeof(QState));
		if (!*qList)
		{
			fprintf(stderr,"Error: unable to malloc \n");
			exit(-1);
		}
		memcpy(*qList,newState,sizeof(QState));
	}
	else
	{
		prevPtr = NULL;
		tmpPtr = *qList;
		while ((tmpPtr) && (tmpPtr->Value < newState->Value))
		{
			prevPtr = tmpPtr;
			tmpPtr = tmpPtr->next;
		}
		if ((!tmpPtr) || (tmpPtr->Value != newState->Value))
		{
			temp = malloc(sizeof(QState));
			if (!temp)
			{
				fprintf(stderr,"Error: unable to malloc\n");
				exit(-1);
			}
			memcpy(temp,newState,sizeof(QState));
			if (prevPtr == NULL) // need to replace the first in list
			{
				temp->next = *qList;
				*qList = temp;
			}
			else
			{
				temp->next = prevPtr->next;
				prevPtr->next = temp;
			}
		}
		else
		{
			tmpPtr->Count += newState->Count;
			if ((creal(tmpPtr->Count) == 0) && (cimag(tmpPtr->Count)==0))
			{
				if (prevPtr == NULL)
				{
					*qList = tmpPtr->next;
				}
				else	
					prevPtr->next = tmpPtr->next;
				free(tmpPtr);
			}
		}

	} 

}

void qEmul_InsertInList_H(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if (mask & currState->Value)
	{
		tempState.Value = currState->Value - mask;
		tempState.Count = currState->Count;
		InsertInList(&tempState,qList);
		tempState.Value = currState->Value;
		tempState.Count = 0 - currState->Count;
		InsertInList(&tempState,qList);
	}
	else
	{
		tempState.Value = currState->Value;
		tempState.Count = currState->Count;
		InsertInList(&tempState,qList);
		tempState.Value = currState->Value + mask;
		tempState.Count = currState->Count;
		InsertInList(&tempState,qList);
	}
}

void qEmul_InsertInList_CT(unsigned long cMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if ((cMask & currState->Value) == cMask) // allow for when cMask == 0
	{	
		if (mask & currState->Value)
		{
			tempState.Value = currState->Value;
			tempState.Count = currState->Count * (1 + _Complex_I) / sqrt(2.0) ; // e^{pi/4}
		}
		else
		{
			tempState.Value = currState->Value;
			tempState.Count = currState->Count;
		}
	}
	else
	{
		tempState.Value = currState->Value;
		tempState.Count = currState->Count;
	}
	
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_CP(unsigned long cMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if ((cMask & currState->Value) == cMask) // allow for when cMask == 0
	{	
		if (mask & currState->Value)
		{
			tempState.Value = currState->Value;
			tempState.Count = currState->Count * (cos(PI/2) + sin(PI/2)*_Complex_I);
		}
		else
		{
			tempState.Value = currState->Value;
			tempState.Count = currState->Count;
		}
	}
	else
	{
		tempState.Value = currState->Value;
		tempState.Count = currState->Count;
	}
	
	InsertInList(&tempState,qList);
}
 
void qEmul_InsertInList_X(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if (mask & currState->Value)
	{
		tempState.Value = currState->Value - mask;
	}
	else
		tempState.Value = currState->Value + mask;
	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_c(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;
	unsigned long tempVal;
	unsigned long multiplier = 1;
		
	if (!currState)
		return;

	tempState.next = NULL;

	tempVal = currState->Value;
	tempState.Value = 0;
	while (tempVal > 0)
	{
		if (tempVal & 0x01)
			tempState.Value += multiplier;
		if (mask & 0x01)
		{
			// copy one more time;
			multiplier <<=1;
			if (tempVal & 0x01)
				tempState.Value += multiplier;

		}	
		mask >>= 1;
		tempVal>>=1;
		multiplier <<= 1;
	}

	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_d(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;
	unsigned long tempVal;
	unsigned long multiplier;
		
	if (!currState)
		return;

	tempState.next = NULL;

	tempVal = currState->Value;
	tempState.Value = 0;
	multiplier = 1;
	while (tempVal > 0)
	{
		if (!(mask & 0x01))
		{
			if (tempVal & 0x01)
				tempState.Value += multiplier;
			multiplier <<= 1;
		}	
		mask >>= 1;
		tempVal>>=1;
	}

	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_I(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	tempState.Value = currState->Value;
	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_0(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if ((currState->Value & mask) == 0)
	{
		tempState.Value = currState->Value;
		tempState.Count = currState->Count;
		InsertInList(&tempState,qList);
	}
}

void qEmul_InsertInList_1(unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	if ((currState->Value & mask) == mask)
	{
		tempState.Value = currState->Value;
		tempState.Count = currState->Count;
		InsertInList(&tempState,qList);
	}
}

void qEmul_InsertInList_CN(unsigned long cMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;
	tempState.next = NULL;
	if (((cMask & currState->Value) == cMask) && (cMask != 0))
	{
		if (mask & currState->Value)
		{
			tempState.Value = currState->Value - mask;
		}
		else
			tempState.Value = currState->Value + mask;
	}
	else
		tempState.Value = currState->Value;
	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_INVQFT(unsigned long qftMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;
	QState tempState1;
	QState tempState2;
	unsigned long tempMask;
	unsigned long rValue = 2;

	if (!currState)
		return;
	if (!qftMask)
		return;
	if (!mask)
		return;

	tempState.next = NULL;
	tempState1.next = NULL;
	tempState2.next = NULL;

	// perform control rotations 
	tempState.Value = currState->Value;
	tempState.Count = currState->Count;

	tempMask = mask << 1;
	while ((tempMask < qftMask) && (tempMask != 0))
	{

		if (tempMask & qftMask)
		{
			if (tempMask & currState->Value)
			{
				if (mask & tempState.Value)
					tempState.Count *= (cos(PI/rValue) + sin(PI/rValue)*_Complex_I);
			}
			rValue*=2;
		}
		tempMask <<=1;
		
	}	

	// end with H if it is the last value
	if (mask & tempState.Value)
	{
		tempState1.Value = tempState.Value - mask;
		tempState1.Count = tempState.Count;
		tempState2.Value = tempState.Value;
		tempState2.Count = 0 - tempState.Count;
	}
	else
	{
		tempState1.Value = tempState.Value;
		tempState1.Count = tempState.Count;
		tempState2.Value = tempState.Value + mask;
		tempState2.Count = tempState.Count;
	}

 //printf("QFT value %ld %f %f, %ld %f %f\n",tempState1.Value,creal(tempState1.Count),cimag(tempState1.Count),tempState2.Value,creal(tempState2.Count),cimag(tempState2.Count)); 
	InsertInList(&tempState1,qList);
	InsertInList(&tempState2,qList);

}

void qEmul_InsertInList_QFT(unsigned long qftMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState1;
	QState tempState2;
	unsigned long tempMask;
	int rValue = 2;

	if (!currState)
		return;
	if (!qftMask)
		return;
	if (!mask)
		return;

	tempState1.next = NULL;
	tempState2.next = NULL;

	// start with H

	if (mask & currState->Value)
	{
		tempState1.Value = currState->Value - mask;
		tempState1.Count = currState->Count;
		tempState2.Value = currState->Value;
		tempState2.Count = 0 - currState->Count;
	}
	else
	{
		tempState1.Value = currState->Value;
		tempState1.Count = currState->Count;
		tempState2.Value = currState->Value + mask;
		tempState2.Count = currState->Count;
	}

	// perform control rotations 

	tempMask = mask;
	while (tempMask > 0)
	{
		tempMask >>=1;
		if (tempMask & qftMask)
		{
			if (tempMask & currState->Value)
			{
				if (mask & tempState1.Value)
					tempState1.Count *= (cos(PI/rValue) + sin(PI/rValue)*_Complex_I);
				if (mask & tempState2.Value)
					tempState2.Count *= (cos(PI/rValue) + sin(PI/rValue)*_Complex_I);
			}
			rValue*=2;
		}
		
	}	

 //printf("QFT value %ld %f %f, %ld %f %f\n",tempState1.Value,creal(tempState1.Count),cimag(tempState1.Count),tempState2.Value,creal(tempState2.Count),cimag(tempState2.Count)); 
	InsertInList(&tempState1,qList);
	InsertInList(&tempState2,qList);
}

void qEmul_InsertInList_swap(unsigned long swapMask, unsigned long mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;
	if (!swapMask)
		return;
	if (!mask)
		return;

	tempState.next = NULL;

	if (swapMask & currState->Value)
	{
		if (!(mask & currState->Value))
		{
			tempState.Value = currState->Value + mask - swapMask;
		}
		else
			tempState.Value = currState->Value;
	}
	else
	{
		if (mask & currState->Value)
		{
			tempState.Value = currState->Value - mask + swapMask;
		}
		else
			tempState.Value = currState->Value;

	}
	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

static unsigned long putValueToMask(unsigned long mask, unsigned long value)
{
	unsigned long retValue = 0;
	unsigned long multiple = 1;

	while (mask)
	{
		if (mask & 1)
		{
			if (value & 1)
			{
				retValue+=multiple;
			}
			multiple <<= 1;
			value >>= 1;
		}
		mask >>= 1;
	}
	return retValue;

}

static unsigned long getValueFromMask(unsigned long mask, unsigned long value)
{
	unsigned long multiple = 1;
	unsigned long retValue = 0;

	while ((value > 0) && (mask > 0))
	{
		if (mask & 1)
		{
			if (value & 1)
				retValue +=multiple;
			multiple <<=1;
		}
		value >>= 1;
		mask >>= 1;
	}

	return retValue;
}

void qEmul_InsertInList_oracle(unsigned long nMask, unsigned long addMask, unsigned long subMask, unsigned long mulMask, unsigned long divMask, unsigned long modMask, unsigned long powMask, unsigned long resMask, QState * currState, QState ** qList)
{
	QState tempState;
	unsigned long aValue = 0;
	unsigned long yValue = 0;
	unsigned long resValue = 0;
	unsigned long invresMask = 0xFFFFFFFFFFFFFFFF ^ resMask;

	if (!currState)
		return;

	if (nMask)
	{
		// get the value of a 

		aValue = getValueFromMask(nMask,currState->Value);
	}
	
	tempState.next = NULL;

	if (mulMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(mulMask,currState->Value);
	
		aValue *= yValue;
	}

	if (divMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(divMask,currState->Value);
	
		aValue /= yValue;
	}

	if (addMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(addMask,currState->Value);
	
		aValue += yValue;
	}

	if (subMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(subMask,currState->Value);
	
		aValue -= yValue;
	}

	if (powMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(powMask,currState->Value);

		aValue = (int)pow((double)aValue,(double)yValue);
	}

	if (modMask)
	{
		// get the value of yValue
		yValue = getValueFromMask(modMask,currState->Value);

		aValue %= yValue;
	}

	// fit it back into the result bits

	resValue = putValueToMask(resMask,aValue);
//	tempState.Value = (currState->Value & invresMask) + resValue;  
	tempState.Value = currState->Value ^ resValue; // doing a XOR instead of =

	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

// To call external oracle

int qEmul_oracle(unsigned int numYQubits, unsigned long (*Oracle)(unsigned long*), char * oracleParams, QState ** qList)
{
	QState * currPtr, * newList;
	QState tempState;
	unsigned long oracleArg[127]; // we don't expect the oracle to take in more than 127 arguments
	unsigned long tempLong;
	char tempStr[10000];
	int i;

	memset(oracleArg,0,sizeof(oracleArg));
	for (i=0;i<127;i++)
		oracleArg[i] = 0;
	oracleArg[0] = numYQubits;

	i = 2;
	while ((strlen(oracleParams) > 0) && (i<127))
	{
		memset(tempStr,0,sizeof(tempStr));
		sscanf(oracleParams,"%lu %[^\n]",&tempLong,tempStr);
		strcpy(oracleParams,tempStr);
		oracleArg[i++] = tempLong;
		
	}
	currPtr = *qList;
	if (!currPtr)
	{
		return -1;
	}
	newList = NULL;
	while (currPtr != NULL)
	{
		oracleArg[1] = currPtr->Value;	
		tempState.Value = Oracle(oracleArg);
		tempState.Count = currPtr->Count;
		tempState.next = NULL;
		InsertInList(&tempState,&newList);
		currPtr = currPtr->next;
	}
	qEmul_FreeList(*qList);
	*qList = newList;
	return 0;
	
}

// to call a classical function

int qEmul_function(unsigned int numBits, unsigned long * (*Function)(int, unsigned long *, unsigned long*), char * functionParams, QState ** qList)
{
	QState * currPtr, * newList;
	QState tempState;
	unsigned long functionArg[127]; // we don't expect the function to take in more than 127 arguments
	unsigned long tempLong;
	char tempStr[10000];
	int i;
	unsigned long * qubitValues, *newQubitValues;

	memset(functionArg,0,sizeof(functionArg));
	i = 0;
	while ((strlen(functionParams) > 0) && (i<127))
	{
		memset(tempStr,0,sizeof(tempStr));
		sscanf(functionParams,"%u %[^\n]",&tempLong,tempStr);
		strcpy(functionParams,tempStr);
		functionArg[i++] = tempLong;
		
	}
	currPtr = *qList;
	if (!currPtr)
	{
		return -1;
	}
	
	tempLong = 1;
	while (currPtr != NULL)
	{
		currPtr = currPtr->next;
		tempLong++;
	}
	if (tempLong < numBits)
		tempLong = numBits;
	qubitValues = (unsigned long *) malloc(tempLong * sizeof(unsigned long));
	if (!qubitValues)
	{
		fprintf(stderr,"error: unable to malloc qubitValues\n");
		return -1;
	}
	for (i=0;i<tempLong;i++)
		qubitValues[i] = 0;

	qubitValues[0] = tempLong-1; // number of values
	currPtr = *qList;
	i = 1;
	while (currPtr != NULL)
	{
		qubitValues[i++] = currPtr->Value;
		currPtr = currPtr->next;
	}

	newQubitValues = Function(numBits, functionArg, qubitValues); 
	newList = NULL;

	if (newQubitValues) // there is no free here, so newQubitValues is likely to be re-using the space allocated by qubitValues 
	{
		for (i=0;i<newQubitValues[0];i++)
		{
			tempState.Value = newQubitValues[i+1] ;
			tempState.Count = 1.0;
			tempState.next = NULL;
			InsertInList(&tempState,&newList);
		}
	}
	free(qubitValues);
	qEmul_FreeList(*qList);
	*qList = newList;
	return 0;
	
}

int qEmul_exec(int numQubits, char * qAlgo, QState ** qList)
{
	QState * currPtr, * newList;
	unsigned long mask;
	int i;
	int oracleDone = 0;
	int swapDone = 0;
	int retQubits = numQubits;

	currPtr = *qList;
	if (!currPtr)
	{
		return -1;
	}
	mask = 1;
	mask <<= numQubits - 1;
	newList = NULL;
	for (i = 0; i < numQubits; i++)
	{
		if (qAlgo[i] == GATE_H)
		{
			while (currPtr != NULL)
			{
				qEmul_InsertInList_H(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}
		}
		else if ((qAlgo[i] == GATE_I) || (qAlgo[i] == GATE_C) || (qAlgo[i] == ORACLE_ADD) || (qAlgo[i] == ORACLE_SUB) || (qAlgo[i] == ORACLE_MUL) || (qAlgo[i] == ORACLE_DIV) || (qAlgo[i] == ORACLE_POW) || (qAlgo[i] == ORACLE_MOD) || (qAlgo[i] == ORACLE_NUM))
		{
			while (currPtr != NULL)
			{
				qEmul_InsertInList_I(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}
		}
		else if (qAlgo[i] == GATE_X)
		{
			while (currPtr != NULL)
			{
				qEmul_InsertInList_X(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}

		}
		else if ((qAlgo[i] == GATE_P) || (qAlgo[i] == GATE_T))
		{
			int j;
			unsigned long tempVal = 1;
			tempVal <<= numQubits-1;
			unsigned long cMask = 0;

			for (j = 0; j < numQubits; j++)
			{
				if (qAlgo[j] == GATE_C)
					cMask += tempVal;
				tempVal >>= 1;
			}	
			while (currPtr != NULL)
			{
				if (qAlgo[i] == GATE_P)
					qEmul_InsertInList_CP(cMask, mask, currPtr, &newList);
				else // GATE_T
					qEmul_InsertInList_CT(cMask, mask, currPtr, &newList);

				currPtr = currPtr->next;
			}

		}
		else if (qAlgo[i] == GATE_SWAP)
		{
			unsigned long swapMask = 0;
			unsigned long tempVal = 1;
			tempVal <<= numQubits-1;
			int j;

			if (!swapDone)
			{
				for (j = 0; j < numQubits; j++)
				{
					if (i != j)
					{
						if (qAlgo[j] == GATE_SWAP)
							swapMask += tempVal;
					}
					tempVal >>= 1;
				}	
				while (currPtr != NULL)
				{
					qEmul_InsertInList_swap(swapMask, mask, currPtr, &newList);
					currPtr = currPtr->next;
				}
				swapDone = 1;
			}
			else
			{
				while (currPtr != NULL)
				{
					qEmul_InsertInList_I(mask, currPtr, &newList);
					currPtr = currPtr->next;
				}
			}
			

		}
		else if (qAlgo[i] == GATE_DELETE)
		{
			while (currPtr != NULL)
			{
				qEmul_InsertInList_d(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}
			retQubits--;

		}
		else if (qAlgo[i] == GATE_CLONE)
		{
			while (currPtr != NULL)
			{
				qEmul_InsertInList_c(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}
			retQubits++;

		}
		else if ((qAlgo[i] == MEASURE_0) || (qAlgo[i] == MEASURE_1) || (qAlgo[i] == MEASURE))
		{
			unsigned long startCount = qEmul_Count2List(*qList);
			unsigned long endCount = 0;
			char qubitVal;
			if (qAlgo[i] == MEASURE)
			{
				double chosen = 0;
				QState * tempPtr = *qList;
				while (tempPtr != NULL)
				{
					chosen += fabs(creal(tempPtr->Count));  
					chosen += fabs(cimag(tempPtr->Count));
					tempPtr = tempPtr->next;
				}
				
				if (chosen > 0)	
				{
					chosen = (rand() % (int) trunc(chosen)) + 1;
					tempPtr = *qList; //currPtr;
					while ((chosen - fabs(creal(tempPtr->Count)) - fabs(cimag(tempPtr->Count))) > 0)
					{
						chosen -= fabs(creal(tempPtr->Count));
						chosen -= fabs(cimag(tempPtr->Count));
						tempPtr = tempPtr->next;
					}
					if ((tempPtr->Value & mask) == 0)	
						qubitVal = MEASURE_0;
					else
						qubitVal = MEASURE_1;
				}
				else
				{	
					qubitVal = MEASURE_0;  // empty list so it doesn't matter what we are looking for
				}

			}
			else
				qubitVal = qAlgo[i];
			
			while (currPtr != NULL)
			{
				if (qubitVal == MEASURE_0)
					qEmul_InsertInList_0(mask, currPtr, &newList);
				else
					qEmul_InsertInList_1(mask, currPtr, &newList);
				currPtr = currPtr->next;
			}
			endCount = qEmul_Count2List(newList);
			if ((startCount > 0) && (qAlgo[i] != MEASURE))
			{
				Probability = (double) (Probability * (double)endCount) / startCount;
			}
		}
		else if (qAlgo[i] == GATE_N)
		{
			int j;
			unsigned long tempVal = 1;
			tempVal <<= numQubits-1;
			unsigned long cMask = 0;

			for (j = 0; j < numQubits; j++)
			{
				if (qAlgo[j] == GATE_C)
					cMask += tempVal;
				tempVal >>= 1;
			}	
			while (currPtr != NULL)
			{
				qEmul_InsertInList_CN(cMask, mask, currPtr, &newList);
				currPtr = currPtr->next;
			}

		}
		else if (qAlgo[i] == ORACLE_EQ)  // invoking oracle
		{
			if (oracleDone)
			{
				while (currPtr != NULL)
				{
					qEmul_InsertInList_I(mask, currPtr, &newList);
					currPtr = currPtr->next;
				}
			}
			else
			{
				int j;
				unsigned long tempVal = 1;
				tempVal <<= numQubits-1;
				unsigned long addMask = 0;
				unsigned long subMask = 0;
				unsigned long mulMask = 0;
				unsigned long divMask = 0;
				unsigned long powMask = 0;
				unsigned long nMask = 0;
				unsigned long modMask = 0;
				unsigned long resMask = 0;

				
				for (j = 0; j < numQubits; j++)
				{
					if (qAlgo[j] == ORACLE_NUM)
						nMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_ADD)
						addMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_SUB)
						subMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_MUL)
						mulMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_DIV)
						divMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_MOD)
						modMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_EQ)
						resMask += tempVal;
					else
					if (qAlgo[j] == ORACLE_POW)
						powMask += tempVal;
					tempVal >>= 1;
				}	
				while (currPtr != NULL)
				{
					qEmul_InsertInList_oracle(nMask,addMask,subMask,mulMask,divMask,modMask,powMask,resMask, currPtr, &newList);
					currPtr = currPtr->next;
				}
				oracleDone = 1;
			}
		}
		else if (qAlgo[i] == GATE_QFT)  // invoking QFT
		{
			unsigned long qftMask = 0;
			unsigned long tempVal = 1;
			tempVal <<= numQubits-1;
			int j;
			for (j = 0; j < numQubits; j++)
			{
				if (qAlgo[j] == GATE_QFT)
					qftMask += tempVal;
				tempVal >>= 1;
			}	
			while (currPtr != NULL)
			{
				qEmul_InsertInList_QFT(qftMask, mask, currPtr, &newList);
				currPtr = currPtr->next;
			}

		}
		else if (qAlgo[i] == GATE_INVQFT)  // invoking inverse QFT
		{
			unsigned long qftMask = 0;
			unsigned long tempVal = 1;
			tempVal <<= numQubits-1;
			int j;
			for (j = 0; j < numQubits; j++)
			{
				if (qAlgo[j] == GATE_INVQFT)
					qftMask += tempVal;
				tempVal >>= 1;
			}	
			while (currPtr != NULL)
			{
				qEmul_InsertInList_INVQFT(qftMask, mask, currPtr, &newList);
				currPtr = currPtr->next;
			}

		}
		else
		{
			fprintf(stderr,"ERROR: %c mode not yet implemented\n",qAlgo[i]);
			
		}
		currPtr = newList;
		qEmul_FreeList(*qList);
		*qList = currPtr;
		newList = NULL;
		mask >>= 1;
		
	}

	return retQubits;
}

