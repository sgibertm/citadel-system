#ifndef __DETECTCOMMAND_H__
#define __DETECTCOMMAND_H__

/************************************************
*
* @File :  detectCommand.h
* @Purpose :  Runs all commands the user might imput, and operates differently for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Project Includes
#include "LIST/listComm.h"
#include "START/startComm.h"
#include "PLEDGE/pledgeComm.h"
#include "ENVOY/envoyComm.h"

// Prototypes

int detectCommand(char **ppsWords, int nNumWords, Maester *ptMaester, Product *ptProducts, int *pnNumProducts);

#endif
