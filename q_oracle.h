/******************************************
 * Name: q_oracle.h
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
#ifndef Q_ORACLE
#define Q_ORACLE

#define MAX_ORACLE 64  // we should eventually support all algorithms found in CKM_* within pkcs11t.h
#define MAX_FUNCTION 64  

#define ORACLE_MODEXP		10
#define ORACLE_EVENMAN_MODEXP	11	

#define ORACLE_MD5 		20
#define ORACLE_SHA1		21
#define ORACLE_SHA2256		22
#define ORACLE_SHA2512		23
#define ORACLE_SHA3384		24

#define ORACLE_DES		31
#define ORACLE_3DES		32
#define ORACLE_AES128		33
#define ORACLE_AES256		34

#define ORACLE_PADCBC		40
#define ORACLE_PADGCM		41

#define FUNCTION_GAUSS_ELI	10

extern unsigned long (* OracleList[MAX_ORACLE])(unsigned long *);
extern unsigned long * (* FunctionList[MAX_FUNCTION])(int, unsigned long *, unsigned long *);

void qOracle_setup(void);
unsigned long Oracle_ModularExponentiation(unsigned long * params);
unsigned long Oracle_EvenMansour_ModExp(unsigned long * params);

unsigned long * Function_Gaussian_Elimination_Binary(int numBits, unsigned long * functionParams, unsigned long * qubitValues);


#endif

