/******************************************
 * Name: q_emul.h
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

#ifndef Q_EMUL_H
#define Q_EMUL_H

#include <stdio.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_QUBITS 63 
#define Q_VERSION 301

#define GATE_H      'H'
#define GATE_I      'I'
#define GATE_C      'C'
#define GATE_N      'N'
#define GATE_X      'X'
#define GATE_P      'P'
#define GATE_T      'T'
#define GATE_DELETE 'd'
#define GATE_CLONE  'c'
#define GATE_SWAP   's'
#define GATE_QFT    'f'
#define GATE_INVQFT 't'
#define MEASURE     'm'
#define MEASURE_0   '0'
#define MEASURE_1   '1'
#define ORACLE_NUM  'n'
#define ORACLE_ADD  '+'
#define ORACLE_SUB  '-'
#define ORACLE_MUL  '*'
#define ORACLE_DIV  '/'
#define ORACLE_POW  '^'
#define ORACLE_MOD  '%'
#define ORACLE_EQ   '='
#define DELIM_NEXT  ','
#define DELIM_PRINT '_'
#define DELIM_END   '.' 

typedef struct _QState
{
  unsigned long Value;
  double complex Count;
  struct _QState * next;
} QState;

double qEmul_GetProbability(void);
void qEmul_ResetProbability(void);
int qEmul_Version(void);
double complex qEmul_Count2List(QState * qList);
int qEmul_PrintList(int numQubits, QState * qList, char * outStr, int outStrLen);
void qEmul_CreateList(QState ** qList);
void qEmul_FreeList(QState * qList);
void qEmul_InsertInList_H(unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_X(unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_I(unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_0(unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_1(unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_CP(unsigned long cMask, unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_CT(unsigned long cMask, unsigned long mask,QState * currState, QState ** qList);
void qEmul_InsertInList_CN(unsigned long cMask, unsigned long mask, QState * currState, QState ** qList);
void qEmul_InsertInList_swap(unsigned long swapMask, unsigned long mask, QState * currState, QState ** qList);
void qEmul_InsertInList_QFT(unsigned long qftMask, unsigned long mask, QState * currState, QState ** qList);

void qEmul_InsertInList_oracle(unsigned long nMask, unsigned long addMask, unsigned long subMask, unsigned long mulMask, unsigned long divMask, unsigned long modMask, unsigned long powMask, unsigned long resMask, QState * currState, QState ** qList);

int qEmul_oracle(int numQubits, unsigned long (*Oracle)(int), QState ** qList);
int qEmul_exec(int numQubits, char * Algo, QState **qList);


#ifdef __cplusplus
}
#endif

#endif 
