/************************************************
*
* @File : startComm.c
* @Purpose : Is in charge of running all start commands the user might imput, and do its proper operations for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "startComm.h"

/*********************************************************************
*  
*   @Name: startTradeRealm
*   @Def: Given all necessary structures of the trading system, keeps the trade terminal running until inputed send or cancel.
*   @Arg:   In: -
*           Out: psRealm = pointer string of the name of the Realm the user wants to trade with, 
*                ptMaester = pointer to typedef strucutre of Maester.
*   @Ret: -
*
*********************************************************************/

void startTradeRealm(Maester *ptMaester, char *psRealm) {
    char *psBuffer  = NULL;
    int nBytes      = 0;
    int nExists     = 0;
    int i   = 0;
    int nRunningTrade   = 0;
    char *psCommand   = NULL;
    ShoppingItem *ptList  = NULL;
    int NumListProducts   = 0;
    char **ppsWords = NULL;
    int nNumWords = 0;

    // Checks if the psRealm exist to start a trade.
    for (i = 0; i < ptMaester->nNumRoutes; i++) {
        if (0 == strcasecmp(psRealm, ptMaester->ptRoutes[i].psNameRoute)) {
            nExists = 1;
        }
    }
    
    if (!nExists) {
        nBytes = asprintf(&psBuffer, "Realm '%s' does not exist.\n", psRealm);
        checkAndPrint(nBytes, &psBuffer);
        return;
    }

    nExists = 0;

    for (i = 0; i < ptMaester->nNnumAlliances; i++) {
        if (0 == strcasecmp(psRealm, ptMaester->ptAlliances[i].sNameRealm) && ptMaester->ptAlliances[i].tStatus == ALLIANCE_ACCEPTED) {
            nExists = 1;
        }
    }

    if (!nExists) {
        nBytes = asprintf(&psBuffer, "ERROR: Cannot trade with realm %s, not an ally.\n", psRealm);
        checkAndPrint(nBytes, &psBuffer);
        return;
    }

    int loadedProducts = 1;

    for (i = 0; i < ptMaester->nNnumAlliances; i++) {
        if (0 == strcasecmp(psRealm, ptMaester->ptAlliances[i].sNameRealm)) {
            if (ptMaester->ptAlliances[i].products == NULL) {
                loadedProducts = 0;
            }
        }
    }

    if (!loadedProducts) {
        printg("ERROR: No products available. Use LIST PRODUCTS first.\n");
        return;
    }

    // Prints the wellcome to the trade terminal
    nBytes = asprintf(&psBuffer, "Entering trade mode with %s.\n", psRealm);
    checkAndPrint(nBytes, &psBuffer);

    Alliance *ally = findAlliance(ptMaester, psRealm);

    // List all items currently available for trade
    printg("Available products: ");
    for (i = 0; i < ally->numProducts; i++) {
        nBytes = asprintf(&psBuffer, "%s", ally->products[i].sNameProduct);
        checkAndPrint(nBytes, &psBuffer);
        
        if (i < ally->numProducts - 1) {
            printg(", ");
        }
    }
    printg(".\n");

    nRunningTrade = 1;
    // Enter secondary command loop for trade-specific actions
    while (nRunningTrade) {
        
        nBytes = asprintf(&psBuffer, BOLD "(trade)> " );
        checkAndPrint(nBytes, &psBuffer);
        psCommand = scanc();

        if (psCommand) {

            ppsWords = parseWords(psCommand, &nNumWords);

            if (0 < nNumWords) {

                nRunningTrade = detectTradeCommand(ptMaester, psRealm, ppsWords, nNumWords, ally, ally->products, &ally->numProducts, &ptList, &NumListProducts);
            
                // free the words inside the command
                for (i = 0; i < nNumWords; i++) {
                    freeVar(&ppsWords[i]);
                }

            }
            else {
                printg("Unknown command\n");
            }
            
        }
        //CONTROL + C detected.
        else if (NULL == psCommand) {
            nRunningTrade = 0;
        }
        free(ppsWords);
        ppsWords = NULL;
        freeVar(&psCommand);
    }

    freeShoppingList(ptList, NumListProducts);
}

