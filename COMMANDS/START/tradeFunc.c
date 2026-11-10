/************************************************
*
* @File : tradeFunc.c
* @Purpose : Manages the sub-menu and functionality of the trade command.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
***********************************************/

// Own inclusions
#include "tradeFunc.h"

/*********************************************************************
*  
*   @Name: findProduct
*   @Def: Given the Product strucutre, checks whether if a prodcut is found in the products.
*   @Arg:   In: nNumProducts = pointer to typedef structure of Products of products willing to be traded.
*           Out: ptProduct = pointer to typedef structure of Products of products available to be traded,
*                   psName = string of the name that must be found.
*   @Ret: Retruns whether the product was found in the list or not.
*
*********************************************************************/

int findProduct(Product *ptProducts, int nNumProducts, char *psName) {
    int nFound   = 0;
    int i   = 0;

    for (i = 0; i < nNumProducts; i++) {
        if (0 == strcasecmp(ptProducts[i].sNameProduct, psName)) {
            nFound = 1;
        }
    }
    return nFound;
}


/*********************************************************************
*  
*   @Name: findProductInList
*   @Def: Given the Shopping list strucutre, finds whether a prodcut is in the list and where it is.
*   @Arg:   In: nSize = size of shopping list.
*           Out: ptList = pointer to typedef structure of List of products willing to be traded, psName = string of the name that must be found.
*   @Ret: Retruns position of the wanted product in the list.
*
*********************************************************************/

int findProductInList(ShoppingItem *ptList, int nSize, char *psName) {
    int i   = 0;

    for (i = 0; i < nSize; i++) {
        if (0 == strcasecmp(ptList[i].psNameProduct, psName))
            return i;
    }
    // Will return -1 if the product hasn't been found, since -1 is not a valid index.
    return -1;
}


/*********************************************************************
*  
*   @Name: readProductName
*   @Def: Enables the product name to be multiple words, and considers it as a single word.
*   @Arg:   In: nFirstWord = position at which the first word of the product's name is placed at, nLastWord = position at which the last word of the product's name is placed at.
*           Out: ppsWords = array of strings with the different command words separated.
*   @Ret: Retruns a string with the product's name words.
*
*********************************************************************/

char *readProductName(char **ppsWords, int nFirstWord, int nLastWord) {
    char *psResult  = NULL;
    char *psTemp    = NULL;
    int nBytes   = 0;
    int i;

    // Concatenate all middle tokens into a single product name string
    for (i = nFirstWord; i < nLastWord; i++) {
        if (NULL == psResult) {
            nBytes = asprintf(&psResult, "%s", ppsWords[i]);
            if (0 > nBytes){
                return NULL;
            }
        } 
        else {
            nBytes = asprintf(&psTemp, "%s %s", psResult, ppsWords[i]);
            if (0 > nBytes) {
                free(psResult);
                free(psTemp);
                return NULL;
            }
            free(psResult);
            psResult = psTemp;
        }
    }
    return psResult;
}


/*********************************************************************
*  
*   @Name: addProduct
*   @Def: 
*   @Arg:   In: nQuantity = number that indicates the ammount of products needed to add.
*           Out: ptProduct = pointer to typedef structure of Products of products available to be traded, 
*                nNumListProducts = number of products in the shopping list, psName = string of the name that must be found.
*   @Ret: Returns the updated shopping list with the item added correctly.
*
*********************************************************************/

ShoppingItem *addProduct(Product *ptProduct, int *pnNumProducts, ShoppingItem *ptList, int *nNumListProducts, char *psName, int nQuantity) {
    ShoppingItem *ptTemp    = NULL;
    int nIndex   = 0;
    int nFound  = 0;

    //Verify that the item exists in the master catalog (not clear in statement, so can be commented)
    nFound = findProduct(ptProduct, *pnNumProducts, psName);
    
    if (!nFound) {
        char *psBuffer = NULL;
        int nBytes = 0;
        nBytes = asprintf(&psBuffer, NORMAL "Product not found.\n");
        checkAndPrint(nBytes, &psBuffer);
        return ptList;
    }

    // Check if item is already in the list to update quantity instead of adding new entry
    nIndex = findProductInList(ptList, *nNumListProducts, psName);

    // New item in the shopping list
    if (-1 == nIndex) {
        
        ptTemp = realloc(ptList, (*nNumListProducts + 1) * sizeof(ShoppingItem));

        if (!ptTemp) {
            printg("ERROR: Could not allocate memory.\n");
            return ptList;
        }

        ptList = ptTemp;

        ptList[*nNumListProducts].psNameProduct = malloc(strlen(psName) + 1);

        if (!ptList[*nNumListProducts].psNameProduct) {
            printg("ERROR: Could not allocate memory.\n");
            return ptList;
        }

        strcpy(ptList[*nNumListProducts].psNameProduct, psName);
        ptList[*nNumListProducts].nAmount = nQuantity;

        (*nNumListProducts)++;
    }
    else {
        // Increment the quantity of an existing item in the list
        ptList[nIndex].nAmount += nQuantity;
    }

    return ptList;
}


/*********************************************************************
*  
*   @Name: removeProduct
*   @Def: Removes a product from the shopping list.
*   @Arg:   In: nQuantity = number that indicates the ammount of products needed to remove.
*           Out: ptList = pointer to typedef structure of List of products willing to be traded, 
*                nNumListProducts = number of products in the shopping list, psName = string of the name that must be found.
*   @Ret: Returns the updated shopping list with the item removed correctly.
*
*********************************************************************/

ShoppingItem *removeProduct(ShoppingItem *ptList, int *nNumListProducts, char *psName, int nQuantity) {
    ShoppingItem *ptTemp    = NULL;
    int nIndex   = 0;
    char *psBuffer    = NULL;
    int nBytes   = 0;
    int nTotalAmount    = 0;
    int i   = 0;

    // Find the item index in the current shopping list
    nIndex = findProductInList(ptList, *nNumListProducts, psName);

    if (-1 == nIndex) {
        nBytes = asprintf(&psBuffer, NORMAL "Product '%s' not found.\n", psName);
        checkAndPrint(nBytes, &psBuffer);
        return ptList;
    }

    nTotalAmount = ptList[nIndex].nAmount - nQuantity;

    // If some quantity remains, simply update the counter
    if (0 < nTotalAmount) {
        ptList[nIndex].nAmount = nTotalAmount;
    } 
    else {
        free(ptList[nIndex].psNameProduct);

        for (i = nIndex; i < *nNumListProducts - 1; i++) {
            ptList[i] = ptList[i + 1];
        }

        (*nNumListProducts)--;

        // Resize the array down or free it if now empty
        if (0 < *nNumListProducts) {
            ptTemp = realloc(ptList, (*nNumListProducts) * sizeof(ShoppingItem));

            if (!ptTemp) {
                printg("ERROR: Could not allocate memory.\n");
                return ptList;
            }

            ptList = ptTemp;
        } 
        else {
            free(ptList);
            ptList = NULL;
        }
    }

    return ptList;
}


/*********************************************************************
*  
*   @Name: makeShoppingFile
*   @Def: Writes into the shopping_list.txt file with the products willing to be traded. For this 
*         phase, will ALWAYS write into the shopping_list.txt file.
*   @Arg:   In: -
*           Out: ptList = pointer to typedef structure of List of products willing to be traded, 
*                nNumListProducts = number of products in the shopping list, 
*                psFilename = string which is the name of the file the information will be written into.
*   @Ret: -
*
*********************************************************************/

void makeShoppingFile (ShoppingItem *ptList, int *nNumListProducts, char *psFilename) {    
    int nFd  = 0;
    char *psBuffer    = NULL;
    int nBytes   = 0;
    int i   = 0;


    nFd = open(psFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (0 > nFd) {
        printg("Error: Could not create list file.\n");
        return;
    }

    for (i = 0; i < *nNumListProducts; i++) {

        nBytes = asprintf(&psBuffer, "%s&%d\n", ptList[i].psNameProduct, ptList[i].nAmount);
        if (0 > nBytes) {
            printg("ERROR: Could not allocate memory.\n");
            close(nFd);
            return;
        }

        printgFile(nFd, psBuffer);
        freeVar(&psBuffer);
    }

    close(nFd);
}


/*********************************************************************
*  
*   @Name: sendShoppingList
*   @Def: Sends the shopping list file to the ally realm through a trade connection, following the protocol for file transfer defined 
*         in the statement.
*   @Arg:   In: -
*           Out: me = pointer to typedef structure of Maester with the information of the current realm, 
*                ally = pointer to typedef structure of Alliance with the information of the ally realm,
*                filename = string which is the name of the file the information will be written into.
*   @Ret: Returns a number indication whether the shopping list was sent successfully or not.
*
*********************************************************************/

void sendShoppingList (Maester *me, Alliance *ally, char *filename, int numListProducts) {


    char *data = NULL;
    asprintf(&data, "%s&%d",filename, numListProducts);

    Mission mission;
    memset(&mission, 0, sizeof(Mission));

    mission.type = MISSION_START_TRADE;
    strcpy(mission.origin, me->psNameRealm);
    strcpy(mission.originIP, me->psIP);
    mission.originPort = me->nPort;

    strcpy(mission.destination, ally->sNameRealm);
    strcpy(mission.destinationIP, ally->sIp);
    mission.destinationPort = ally->nPort;


    strcpy(mission.data, data);

    assignMission(me, mission);

    free(data);

    return;
}


/*********************************************************************
*  
*   @Name: freeShoppingList
*   @Def: Ensures to free all space from memory occupied by the Shopping Item structure.
*   @Arg:   In: nNumListProducts = number of products in the shopping list.
*           Out: ptList = pointer to typedef structure of List of products willing to be traded.    
*   @Ret: -
*
*********************************************************************/

void freeShoppingList (ShoppingItem *ptList, int nNumListProducts) {

    for(int i = 0; i < nNumListProducts; i++) {
        free(ptList[i].psNameProduct);
    }
    free(ptList);

}

/*********************************************************************
*  
*   @Name: detectTradeCommand
*   @Def: Encures to detect the type of command needed for the trade sub-menu.
*   @Arg:   In: nNumWords = number of words the inputed command has.
*           Out: me = pointer to typedef structure of Maester with the information of the current realm, 
*                psRealm = string that indicates the realm the trade wants to de done with,
*                ppsWords = array of strings with the different command words separated,
*                nNumProducts = pointer to typedef structure of Products of products available to be traded,
*                ally = pointer to typedef structure of Alliance with the information of the ally realm,
*                ptProduct = pointer to typedef structure of Products of products available to be traded,
*                pptList = double pointer to typedef structure of List of products willing to be traded,
*                nNumListProducts = number of products in the shopping list.
*   @Ret: Returns a number indication whether the sub-menu trade should continue to work or not.
*
*********************************************************************/

int detectTradeCommand (Maester *me, char *psRealm, char **ppsWords, int nNumWords, Alliance *ally, Product *ptProduct, int *pnNumProducts, ShoppingItem **pptList, int *pnNumListProducts) {
    int nParsed     = 0;
    int nRunningTrade    = 1;
    char *psBuffer  = NULL;
    int nBytes     = 0;
    int nQuantity   = 0;
    char *psProductName     = NULL;

    if (1 == nNumWords && 0 == strcasecmp(ppsWords[0], "cancel")) {

        nParsed = 1;
        freeShoppingList(*pptList, *pnNumListProducts);

        *pptList = NULL;
        *pnNumListProducts = 0;

        nBytes = asprintf(&psBuffer, NORMAL "Trade list to %s has been cancelled.\n", psRealm);
        checkAndPrint(nBytes, &psBuffer);

        nRunningTrade = 0;
    }

    else if(1 == nNumWords && 0 == strcasecmp(ppsWords[0], "send")) {
        nParsed = 1;

        // Prevent sending an empty list
        if (NULL == *pptList || 0 == *pnNumListProducts) {
            printg(NORMAL "Shopping list is empty.\n");   
            return nRunningTrade;
        }

        if (ally->sIp == NULL || ally->tStatus != ALLIANCE_ACCEPTED) {
            nBytes = asprintf(&psBuffer, NORMAL "ERROR: %s is no more an ally. Cannot send trade order.\n", ally->sNameRealm);
            checkAndPrint(nBytes, &psBuffer);
            return nRunningTrade;
        }

        char *shoppingListFile = "REALMS/shopping_list.txt";
        makeShoppingFile(*pptList, pnNumListProducts, shoppingListFile);

        sendShoppingList(me, ally, shoppingListFile, *pnNumListProducts);


        freeShoppingList(*pptList, *pnNumListProducts); 
        *pptList = NULL;
        *pnNumListProducts = 0;

        nRunningTrade = 0;
    }

    // Process "add" and "remove" commands with multi-word product names
    else if (1 <= nNumWords && (0 == strcasecmp(ppsWords[0], "add") || 0 == strcasecmp(ppsWords[0], "remove"))) {

        nParsed = 1;

        if (1 == nNumWords || 2 == nNumWords) {
            if (1 == nNumWords) {
                printg(NORMAL "Did you mean to add ' '? Please review syntax.\n");
                
            }
            else {
                nBytes = asprintf(&psBuffer, NORMAL "Did you mean to add '%s'? Please review syntax.\n", ppsWords[1]);
                checkAndPrint(nBytes, &psBuffer);
            }
            return nRunningTrade;
        }

        // Treat the last word as the numeric quantity
        nQuantity = atoi(ppsWords[nNumWords - 1]);
    
        // Check if last word is actually a number
        nQuantity = atoi(ppsWords[nNumWords - 1]);

        if (0 >= nQuantity) {
            // Last word is not a valid number — whole thing after "add/remove" is the product name
            psProductName = readProductName(ppsWords, 1, nNumWords);
            if (NULL == psProductName) {
                printg("ERROR: Could not allocate memory.\n");
                return nRunningTrade;
            }
            nBytes = asprintf(&psBuffer, NORMAL "ERROR: Invalid amount for '%s'.\n", psProductName);
            checkAndPrint(nBytes, &psBuffer);
        }
        else {
            // Last word is the quantity, everything in between is the product name
            psProductName = readProductName(ppsWords, 1, nNumWords - 1);
            if (NULL == psProductName) {
                printg("ERROR: Could not allocate memory.\n");
                return nRunningTrade;
            }
            if (0 == strcasecmp(ppsWords[0], "add")) {
                *pptList = addProduct(ptProduct, pnNumProducts, *pptList, pnNumListProducts, psProductName, nQuantity);
            }
            else {
                *pptList = removeProduct(*pptList, pnNumListProducts, psProductName, nQuantity);
            }
        }

        freeVar(&psProductName);
    }

    if (!nParsed) {
        printg(NORMAL "Unknown command\n");
    }
    
    return nRunningTrade;
}


/*********************************************************************
*  
*   @Name: checkProduct
*   @Def: Checks whether a product is in stock and available to be traded.
*   @Arg:   In: -
*           Out: prod_name = string of the name of the product that needs to be checked, 
*                qty_requested = number that indicates the ammount of products needed to check, 
*                stockFile = string which is the name of the file where the stock information is.
*   @Ret: Returns a string with the reason why the product cannot be traded, or NULL if it can be traded.
*
*********************************************************************/

char *checkProduct(char *prod_name, int qty_requested, char *stockFile) {

    int stock_fd = open(stockFile, O_RDONLY);
    if (stock_fd < 0) {
        return "UNKNOWN_PRODUCT";
    }

    Product prod;
    int found = 0;

    while (read(stock_fd, &prod, sizeof(Product)) == sizeof(Product)) {

        if (strcasecmp(prod.sNameProduct, prod_name) == 0) {
            found = 1;

            if (prod.nAmount < qty_requested) {
                close(stock_fd);
                return "OUT_OF_STOCK";
            }

            break;
        }
    }

    close(stock_fd);

    if (!found) {
        return "UNKNOWN_PRODUCT";
    }

    return NULL;
}


/*********************************************************************
*  
*   @Name: updateStockFromOrder
*   @Def: Updates the stock file after receiving an order, by removing the quantities of the products that have been ordered.
*   @Arg:   In: -
*           Out: temp_file = string of the name of the temporary file where the order information is,
*                stockFile = string which is the name of the file where the stock information is,
*                products = pointer to typedef structure of Products of products available to be traded,
*                numProducts = number of products available to be traded.
*   @Ret: Returns a string with the reason why the stock could not be updated, or "OK" if it was updated successfully.
*
*********************************************************************/

char* updateStockFromOrder(char *temp_file, char *stockFile, Product *products, int numProducts) {

    char *reject_reason = NULL;

    int tempfd = open(temp_file, O_RDONLY);
    if (tempfd < 0) {
        printg("ERROR: Could not open order file.\n");
        return NULL;
    }

    char *line;

    //per validar la list
    while ((line = readLineFile(tempfd)) && !reject_reason) {

        char *prod_name = strtok(line, "&");
        char *qty_str   = strtok(NULL, "&");

        if (prod_name && qty_str) {
            int qty = atoi(qty_str);
            reject_reason = checkProduct(prod_name, qty, stockFile);
        }

        free(line);
    }

    close(tempfd);


    if (reject_reason) {
        char *buffer = NULL;
        int bytes = asprintf(&buffer, NORMAL "ERROR: Could not sent trade order due to %s\n", reject_reason);
        checkAndPrint(bytes, &buffer);
        return reject_reason;
    }

    // update stock
    int stock_fd = open(stockFile, O_RDONLY);
    if (stock_fd < 0) {
        return NULL;
    }

    int tmp_fd = open("REALMS/tmp_stock", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (tmp_fd < 0) {
        close(stock_fd);
        return NULL;
    }

    Product prod;
    while (read(stock_fd, &prod, sizeof(Product)) == sizeof(Product)) {

        // tornar a llegir order per cada producte
        int tempfd2 = open(temp_file, O_RDONLY);
        if (tempfd2 < 0) {
            break;
        }

        char *line;

        while ((line = readLineFile(tempfd2))) {

            char *prod_name = strtok(line, "&");
            char *qty_str   = strtok(NULL, "&");

            if (prod_name && qty_str && strcasecmp(prod.sNameProduct, prod_name) == 0) {
                prod.nAmount -= atoi(qty_str);

                for (int i = 0; i < numProducts; i++) {
                    if (strcasecmp(products[i].sNameProduct, prod_name) == 0) {
                        products[i].nAmount -= atoi(qty_str);
                        break;
                    }
                }
            }

            free(line);
        }

        close(tempfd2);

        write(tmp_fd, &prod, sizeof(Product));
    }


    close(stock_fd);
    close(tmp_fd);

    remove(stockFile);
    rename("REALMS/tmp_stock", stockFile);

    
    printg("Order processed successfully. Stock updated.\n");

    return "OK";
}