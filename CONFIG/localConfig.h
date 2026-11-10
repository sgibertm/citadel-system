#ifndef __LOCALCONFIG_H__
#define __LOCALCONFIG_H__

/************************************************
*
* @File : localConfig.h
* @Purpose : Define the structures and function prototypes needed to read and store the Maester and Stock configuration files and terminal.
* @Author : Sara Gibert & Oriol Pros
* @Date : 06/03/2026
*
***********************************************/

// Project Includes
#include "../utils.h"
#include "../COMMANDS/detectCommand.h"

// Prototypes
void freeMaester(Maester *ptMaester);

void freeProduct(Product **pptProducts);

int readMasterFile(char *psFilename, Maester *ptMaester);

int readStockFile (char *psFilename, Product **pptProducts, int *pnNumProducts);

void startCitadelSystem(Maester *ptMaester, Product *pptProducts, int *pnNumProducts);


#endif