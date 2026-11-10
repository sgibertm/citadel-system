/************************************************
*
* @File :  detectCommand.c
* @Purpose :  Runs all commands the user might imput, and operates differently for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "detectCommand.h"


/*********************************************************************
*  
*   @Name: detectCommand
*   @Def: Detects the command entered by the user and calls the appropriate function.
*   @Arg:   In: nNumWords = number of words that a command has.
*           Out: ppsWords = double pointer to words in command, ptMaester = pointer to typedef strucutre of Maester, 
*                ptProducts = double pointer to typedef structure of Products,pnNumProducts = pointer to number of products.
*   @Ret: Returns 1 if the operation is successful , 0 otherwise.
*
*********************************************************************/

int detectCommand(char **ppsWords, int nNumWords, Maester *ptMaester, Product *ptProducts, int *pnNumProducts) {
    int nParsed     = 0;
    char *psBuffer  = NULL;
    int nBytes      = 0;

    // EXIT COMMAND
    if (1 == nNumWords && 0 == strcasecmp(ppsWords[0], "EXIT")) {        
        nParsed = 1;

        nBytes = asprintf(&psBuffer, "The Maester of %s signs off. The ravens rest.\n\n", ptMaester->psNameRealm);
        checkAndPrint(nBytes, &psBuffer);

        return 0;
    }

    // LIST COMMANDS
    else if (2 <= nNumWords && 0 == strcasecmp(ppsWords[0], "LIST")) {

        // LIST REALMS
        if (2 == nNumWords && 0 == strcasecmp(ppsWords[1], "REALMS")) {
            nParsed = 1;
            listRealms(ptMaester);
        }

        // LIST PRODUCTS
        else if (2 == nNumWords && 0 == strcasecmp(ppsWords[1], "PRODUCTS")) {
            nParsed = 1;
            listProducts(ptProducts, pnNumProducts);
        }

        //LIST PRODUCTS REALM
        else if (3 == nNumWords && 0 == strcasecmp(ppsWords[1], "PRODUCTS")) {
            int nExists = 0;
            int i = 0;

            for (i = 0; i < ptMaester->nNumRoutes; i++) {
                if (0 == strcasecmp(ppsWords[2], ptMaester->ptRoutes[i].psNameRoute)) {
                    nExists = 1;
                }
            }

            if (nExists) {
                nParsed = 1;

                listProductsRealm(ptMaester, ppsWords[2]);
            }
            else {
                nParsed = 1;
                printg("ERROR: Unknown realm to get products from.\n");
            }
        }
    }
    
    // PLEDGE COMMANDS
    else if (0 == strcasecmp(ppsWords[0], "PLEDGE")) {

        if (nNumWords < 2) {
            nParsed = 1;
            printg("Did you mean to send a pledge? Please review syntax.\n");
            return 1;
        }

        // PLEDGE STATUS
        if (0 == strcasecmp(ppsWords[1], "STATUS")) {
            if (2 == nNumWords) {
                nParsed = 1;
                pledgeStatus(ptMaester);
            } 
            else {
                nParsed = 1;
                printg("Did you mean to send a pledge? Please review syntax.\n");
            }
        }

        // PLEDGE REQUEST
        else if (3 == nNumWords) {
            int nExists = 0;
            int i = 0;

            for (i = 0; i < ptMaester->nNumRoutes; i++) {
                if (0 == strcasecmp(ppsWords[1], ptMaester->ptRoutes[i].psNameRoute) && 0 != strcasecmp(ppsWords[1], "default")) {
                    nExists = 1;
                }
            }

            if (nExists) {
                nParsed = 1;

                pledgeRequest(ptMaester, ppsWords[1], ppsWords[2]);
            }
            else {
                nParsed = 1;
                printg("ERROR: Unknown realm to pledge.\n");
            }
        }

        // PLEDGE RESPOND
        else if (4 == nNumWords && 0 == strcasecmp(ppsWords[1], "RESPOND")) { // words[2] = realm, words[3] = ACCEPT/REJECT

            if (0 == strcasecmp(ppsWords[3], "ACCEPT")) {
                nParsed = 1;
                pledgeRespond(ptMaester, ppsWords[2], 1);
            }
            
            else if (0 == strcasecmp(ppsWords[3], "REJECT")) {
                nParsed = 1;
                pledgeRespond(ptMaester, ppsWords[2], 0);
            }
            else {
                nParsed = 1;
                printg("Did you mean to send a pledge response? Please review syntax.\n");
            }
        }

        else {
            nParsed = 1;
            printg("Did you mean to send a pledge? Please review syntax.\n");
        }
    }

    // START COMMANDS
    else if (0 == strcasecmp(ppsWords[0], "START")) {

        if (2 == nNumWords && 0 == strcasecmp(ppsWords[1], "TRADE")) {
            nParsed = 1;
            printg("Missing arguments, can’t start a trade. Please review the syntax.\n");
        }
        
        // START TRADE REALM
        else if(3 == nNumWords && 0 == strcasecmp(ppsWords[1], "TRADE")) {
            nParsed = 1;
            startTradeRealm(ptMaester, ppsWords[2]);
        }  
    }

    // ENVOY COMMANDS
    else if (0 == strcasecmp(ppsWords[0], "ENVOY")) {

        // ENVOY STATUS
        if (2 == nNumWords && 0 == strcasecmp(ppsWords[1], "STATUS")) {
            nParsed = 1;
            envoyStatus(ptMaester);
        }
    }

    if (!nParsed) {
        printg("Unknown command\n");
    }

    return 1;
}

