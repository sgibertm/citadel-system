#ifndef __LISTCOMM_H__
#define __LISTCOMM_H__

/************************************************
*
* @File :  listComm.h
* @Purpose :  Runs all list commands the user might imput, and operates differently for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Project Includes
#include "../../utils.h"
#include "../../CONFIG/networkConfig.h"
#include "../../CONFIG/envoyConfig.h"

// Prototypes
void listRealms(Maester *ptMaester);

void listProducts (Product *ptProducts, int *pnNumProducts);

void listProductsRealm(Maester *ptMaester, char *destination);

#endif