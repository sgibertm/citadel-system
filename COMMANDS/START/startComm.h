#ifndef __STARTCOMM_H__
#define __STARTCOMM_H__

/************************************************
*
* @File :  startComm.h
* @Purpose :  Runs all start commands the user might imput, and operates differently for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Project Includes
#include "../../utils.h"
#include "tradeFunc.h"
#include "../../SOCKET/frame.h"
//#include "../../CONFIG/networkConfig.h"

// Prototypes
void startTradeRealm(Maester *ptMaester, char *psRealm);

#endif