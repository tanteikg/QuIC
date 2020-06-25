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

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_QUBITS 31 
#define Q_VERSION 201

#define GATE_H      'H'
#define GATE_I      'I'
#define GATE_C      'C'
#define GATE_N      'N'
#define GATE_X      'X'
#define GATE_DELETE 'd'
#define GATE_CLONE  'c'
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
  int Value;
  int Count;
  struct _QState * next;
} QState;

double qEmul_GetProbability(void);
void qEmul_ResetProbability(void);
int qEmul_Version(void);
int qEmul_Count2List(QState * qList);
int qEmul_PrintList(int numQubits, QState * qList, char * outStr, int outStrLen);
void qEmul_CreateList(QState ** qList);
void qEmul_FreeList(QState * qList);
void qEmul_InsertInList_H(int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_X(int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_I(int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_0(int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_1(int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_CN(int cnMask, int mask,QState * currState, QState ** qList);
void qEmul_InsertInList_oracle(int nMask, int addMask, int subMask, int mulMask, int divMask, int modMask, int powMask, int resMask, QState * currState, QState ** qList);

int qEmul_oracle(int numQubits, int(*Oracle)(int), QState ** qList);
int qEmul_exec(int numQubits, char * Algo, QState **qList);


#ifdef __cplusplus
}
#endif

#endif 
