#ifndef __PLEDGECOMM_H__
#define __PLEDGECOMM_H__

/************************************************
*
* @File :  pledgeComm.h
* @Purpose :  Runs all pledge commands the user might imput, and operates differently for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Project Includes
#include "../../utils.h"
#include "../../SOCKET/frame.h"
#include "../../CONFIG/envoyConfig.h"
//#include "../../CONFIG/networkConfig.h"

// Prototypes
void pledgeRequest(Maester *ptMaester, char *destination, char *sigil);

void pledgeRespond(Maester *me, char *realm, int accept);

void pledgeStatus(Maester *maester);

#endif