#ifndef __TRADEFUNC_H__
#define __TRADEFUNC_H__

/************************************************
*
* @File : tradeFunc.h
* @Purpose : Manages all functionality of the trade command.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Project Includes
#include "../../utils.h"
#include "../../SOCKET/frame.h"
#include "../../CONFIG/networkConfig.h"
#include "../../CONFIG/envoyConfig.h"

// Prototypes
int findProduct(Product *ptProducts, int nNumProducts, char *psName);

int findProductInList(ShoppingItem *ptList, int nSize, char *psName);

char *readProductName(char **ppsWords, int nFirstWord, int nLastWord);

ShoppingItem *addProduct(Product *ptProduct, int *pnNumProducts, ShoppingItem *ptList, int *nNumListProducts, char *psName, int nQuantity);

ShoppingItem *removeProduct(ShoppingItem *ptList, int *nNumListProducts, char *psName, int nQuantity);

void makeShoppingFile(ShoppingItem *ptList, int *nNumListProducts, char *psFilename);

void sendShoppingList (Maester *me, Alliance *ally, char *filename, int numListProducts);

void freeShoppingList (ShoppingItem *ptList, int nNumListProducts);

int detectTradeCommand (Maester *me, char *psRealm, char **ppsWords, int nNumWords, Alliance *ally, Product *ptProducts, int *pnNumProducts, ShoppingItem **pptList, int *pnNumListProducts);

char *checkProduct(char *prod_name, int qty_requested, char *stockFile);

char *updateStockFromOrder(char *temp_file, char *stockFile, Product *products, int numProducts);

#endif