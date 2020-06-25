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

int qEmul_Count2List(QState * qList)
{
        QState * tempPtr;
        int count = 0;

        tempPtr = qList;
        while (tempPtr)
        {
                count += tempPtr->Count*tempPtr->Count;
                tempPtr = tempPtr->next;
        }
        return count;
}

static int printBinStr(char * binString, int numQubits, int value)
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
		sprintf(tempStr,"Value %c[%s] Count[%02d]\n",(temp->Count<0)?'-':' ', tempBinStr, abs(temp->Count));
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
		printf("Error in malloc qList\n");
		exit(-1);
	}
	temp->Value = 0;
	temp->Count = 1;
	temp->next = NULL; 
	Probability = 1.0;
	srand(time(NULL));
	*qList = temp;
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
			printf("Error: unable to malloc \n");
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
				printf("Error: unable to malloc\n");
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
			if (tmpPtr->Count == 0)
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

void qEmul_InsertInList_H(int mask,QState * currState, QState ** qList)
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
 
void qEmul_InsertInList_X(int mask,QState * currState, QState ** qList)
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

void qEmul_InsertInList_c(int mask,QState * currState, QState ** qList)
{
	QState tempState;
	int tempVal;
	int multiplier = 1;
		
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

void qEmul_InsertInList_d(int mask,QState * currState, QState ** qList)
{
	QState tempState;
	int tempVal;
	int multiplier;
		
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

void qEmul_InsertInList_I(int mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;

	tempState.next = NULL;

	tempState.Value = currState->Value;
	tempState.Count = currState->Count;
	InsertInList(&tempState,qList);
}

void qEmul_InsertInList_0(int mask,QState * currState, QState ** qList)
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

void qEmul_InsertInList_1(int mask,QState * currState, QState ** qList)
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

void qEmul_InsertInList_CN(int cnMask, int mask,QState * currState, QState ** qList)
{
	QState tempState;

	if (!currState)
		return;
	tempState.next = NULL;
	if (((cnMask & currState->Value) == cnMask) && (cnMask != 0))
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

static int putValueToMask(int mask, int value)
{
	int retValue = 0;
	int addOn = 1;

	while (mask)
	{
		if (mask & 1)
		{
			if (value & 1)
			{
				retValue+=addOn;
			}
			addOn <<= 1;
			value >>= 1;
		}
		mask >>= 1;
	}
	return retValue;

}

static int getValueFromMask(int mask, int value)
{
	int addOn = 1;
	int retValue = 0;

	while ((value > 0) && (mask > 0))
	{
		if (mask & 1)
		{
			if (value & 1)
				retValue +=addOn;
			addOn <<=1;
		}
		value >>= 1;
		mask >>= 1;
	}

	return retValue;
}

void qEmul_InsertInList_oracle(int nMask, int addMask, int subMask, int mulMask, int divMask, int modMask, int powMask, int resMask, QState * currState, QState ** qList)
{
	QState tempState;
	int aValue = 0;
	int yValue = 0;
	int resValue = 0;
	int invresMask = 0xFFFFFFFF ^ resMask;

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

int qEmul_oracle(int numQubits, int(*Oracle)(int), QState ** qList)
{
	QState * currPtr, * newList;
	QState tempState;

	currPtr = *qList;
	if (!currPtr)
	{
		return -1;
	}
	newList = NULL;
	while (currPtr != NULL)
	{
		tempState.Value = Oracle(currPtr->Value);
		tempState.Count = currPtr->Count;
		tempState.next = NULL;
		InsertInList(&tempState,&newList);
		currPtr = currPtr->next;
	}
	qEmul_FreeList(*qList);
	*qList = newList;
	
}

int qEmul_exec(int numQubits, char * qAlgo, QState ** qList)
{
	QState * currPtr, * newList;
	int mask;
	int i;
	int oracleDone = 0;
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
			int startCount = qEmul_Count2List(*qList);
			int endCount = 0;
			char qubitVal;
			if (qAlgo[i] == MEASURE)
			{
				int chosen = 0;
				QState * tempPtr = *qList;
				while (tempPtr != NULL)
				{
					chosen += (int) abs(tempPtr->Count);
					tempPtr = tempPtr->next;
				}
				
				if (chosen > 0)	
				{
					chosen = (rand() % chosen) + 1;
					tempPtr = currPtr;
					while (chosen - (int) abs(tempPtr->Count) > 0)
					{
						chosen -= (int) abs(tempPtr->Count);
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
			int tempVal = 1;
			tempVal <<= numQubits-1;
			int cnMask = 0;

			for (j = 0; j < numQubits; j++)
			{
				if (qAlgo[j] == GATE_C)
					cnMask += tempVal;
				tempVal >>= 1;
			}	
			while (currPtr != NULL)
			{
				qEmul_InsertInList_CN(cnMask, mask, currPtr, &newList);
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
				int tempVal = 1;
				tempVal <<= numQubits-1;
				int addMask = 0;
				int subMask = 0;
				int mulMask = 0;
				int divMask = 0;
				int powMask = 0;
				int nMask = 0;
				int modMask = 0;
				int resMask = 0;

				
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
		else
		{
			printf("ERROR: %c mode not yet implemented\n",qAlgo[i]);
			
		}
		currPtr = newList;
		qEmul_FreeList(*qList);
		*qList = currPtr;
		newList = NULL;
		mask >>= 1;
		
	}

	return retQubits;
}

