/************************************************
*
* @File : localConfig.c 
* @Purpose : Load information into structures by reading files and keep terminal of the program going.
* @Author : Sara Gibert & Oriol Pros
* @Date : 06/03/2026
*
***********************************************/

// Own inclusions
#include "localConfig.h"

/*********************************************************************
*  
*   @Name: freeMaester
*   @Def: Given the structure of Maester, it frees it all.
*   @Arg:   In: -
*           Out: tMaester = pointer to typedef structure of Maester.
*   @Ret: -
*
*********************************************************************/

void freeMaester(Maester *tMaester) {
    int i = 0;
    
    free(tMaester->psNameRealm);
    free(tMaester->psPathFolder);
    free(tMaester->psIP);
    free(tMaester->stockName);


    // Iterate through each route to free nested dynamic strings
    for (i = 0; i < tMaester->nNumRoutes; i++) {
        free(tMaester->ptRoutes[i].psNameRoute);
        free(tMaester->ptRoutes[i].psIP);
    }
    free(tMaester->ptRoutes);


    for (i = 0; i < tMaester->nNnumAlliances; i++) {
        free(tMaester->ptAlliances[i].sNameRealm);
        free(tMaester->ptAlliances[i].sIp);
        if (tMaester->ptAlliances[i].products != NULL) {
            free(tMaester->ptAlliances[i].products); 
            tMaester->ptAlliances[i].products = NULL;
        }
    }
    free(tMaester->ptAlliances);


    for (int i = 0; i < tMaester->nNumEnvoys; i++) {

        close(tMaester->ptEnvoys[i].to_envoy[0]);
        close(tMaester->ptEnvoys[i].to_envoy[1]);
        close(tMaester->ptEnvoys[i].from_envoy[0]);
        close(tMaester->ptEnvoys[i].from_envoy[1]);

        if (tMaester->ptEnvoys[i].pid > 0) {
                
            kill(tMaester->ptEnvoys[i].pid, SIGTERM);
            waitpid(tMaester->ptEnvoys[i].pid, NULL, 0);
        }

    }

    free(tMaester->ptEnvoys);
}

/*********************************************************************
*  
*   @Name: freeProduct
*   @Def: Given the structure of Products, it frees it all.
*   @Arg:   In: -
*           Out: ptProducts = double pointer to typedef structure of Products.
*   @Ret: -
*
*********************************************************************/

void freeProduct(Product **ptProducts) {
    // Release the block of product memory and nullify the pointer
    free(*ptProducts); 
    *ptProducts = NULL;
}


/*********************************************************************
*  
*   @Name: readMasterFile
*   @Def: Given the filename and the empty structure of Maester, read the file while filling in the structure.
*   @Arg:   In: -
*           Out: psFilename = pointer strig to the name of the file, ptMaester = pointer to typedef strucutre of Maester.
*   @Ret: Returns 1 if the operation is successful , 0 otherwise.
*
*********************************************************************/

int readMasterFile(char *psFilename, Maester *ptMaester) {
    int nFd     = 0;
    char *psBuffer  = NULL;
    Route *ptRoute  = NULL;
    char *psNameRoute   = NULL;
    char *psIP      = NULL;
    int nPort   = 0;
    int nReadCorrectly  = 1;

    nFd = open(psFilename, O_RDONLY);

    if (0 > nFd) {
        printg("ERROR: Could not open the file.\n");
        return 0;
    }
    
    // Read and clean all data from file
    ptMaester->psNameRealm = readLineFile(nFd);
    if (NULL == ptMaester->psNameRealm) {
        close(nFd);
        return 0;
    }
    removeAmpersand(ptMaester->psNameRealm);

    ptMaester->psPathFolder = readLineFile(nFd);
    if (NULL == ptMaester->psPathFolder) {
        close(nFd);
        return 0;
    }

    psBuffer = readLineFile(nFd);
    if (NULL == psBuffer) {
        close(nFd);
        return 0;
    }
    ptMaester->nNumEnvoys = atoi(psBuffer);
    freeVar(&psBuffer);

    ptMaester->psIP = readLineFile(nFd);
    if (NULL == ptMaester->psIP) {
        close(nFd);
        return 0;
    }

    psBuffer = readLineFile(nFd);
    if (NULL == psBuffer) {
        close(nFd);
        return 0;
    }
    ptMaester->nPort = atoi(psBuffer);
    freeVar(&psBuffer);

    // For ignoring --- ROUTES --- from file.
    psBuffer = readLineFile(nFd); 
    if (NULL == psBuffer) {
        close(nFd);
        return 0;
    }
    freeVar(&psBuffer);

    ptMaester->ptRoutes = NULL;
    ptMaester->nNumRoutes = 0;

    ptMaester->ptAlliances = NULL;
    ptMaester->nNnumAlliances = 0;

    //Change for phase 3, assign the stock file name to the Maester structure
    ptMaester->stockName = NULL;

    nReadCorrectly = 1;
    while(nReadCorrectly) {
        
        psNameRoute = scancFile(nFd);

        if (NULL == psNameRoute) {
            break;
        }
        else {
            removeAmpersand(psNameRoute);
            
            psIP = scancFile(nFd);
            if (NULL == psIP) {
                freeVar(&psNameRoute);
                close(nFd);
                return 0;
            }

            psBuffer = scancFile(nFd);
            if (NULL == psBuffer) {
                freeVar(&psNameRoute);
                freeVar(&psIP);
                close(nFd);
                return 0;
            }
            nPort = atoi(psBuffer);
            freeVar(&psBuffer);

            // Increment count and expand memory for the new route
            ptMaester->nNumRoutes++;

            Route *temp = realloc(ptMaester->ptRoutes,sizeof(Route) * ptMaester->nNumRoutes);
            if (NULL == temp) {
                printg("ERROR: Could not allocate memory.\n");
                freeVar(&psNameRoute);
                freeVar(&psIP);
                close(nFd);
                return 0;
            }
            ptMaester->ptRoutes = temp;

            ptRoute = &ptMaester->ptRoutes[ptMaester->nNumRoutes - 1];

            ptRoute->psNameRoute = malloc(strlen(psNameRoute) + 1);
            if (NULL == ptRoute->psNameRoute) {
                printg("ERROR: Could not allocate memory.\n");
                freeVar(&psNameRoute);
                freeVar(&psIP);
                close(nFd);
                return 0;
            }
            strcpy(ptRoute->psNameRoute, psNameRoute);

            ptRoute->psIP = malloc(strlen(psIP) + 1);
            if (NULL == ptRoute->psIP) {
                printg("ERROR: Could not allocate memory.\n");
                freeVar(&psNameRoute);
                freeVar(&psIP);
                free(ptRoute->psNameRoute);
                close(nFd);
                return 0;
            }
            strcpy(ptRoute->psIP, psIP);

            ptRoute->nPort = nPort;

            freeVar(&psNameRoute);
            freeVar(&psIP);
        }
    }

    close(nFd);








    return 1;
}

/*********************************************************************
*  
*   @Name: readStockFile
*   @Def: Given the filename, the number of products, and the empty structure of Products, read the file while filling in the structure adn updating number of products.
*   @Arg:   In: -
*           Out: psFilename = pointer strig to the name of the file, pptProducts = double pointer to typedef strucutre of Products, pnNumProducts = pointer to number of products.
*   @Ret: Returns 1 if the operation is successful , 0 otherwise.
*
*********************************************************************/

int readStockFile (char *psFilename, Product **pptProducts, int *pnNumProducts) {
    // Temporary pointer for realloc.
    Product *ptTemp   = NULL;
    // Flag to indicate if everything went well during the reading process
    int nOk     = 1; 
    int nFd     = 0;
    int nBytesRead       = 0;
    
    *pptProducts = NULL;
    *pnNumProducts = 0;

    nFd = open(psFilename, O_RDONLY);
    
    if (0 > nFd) {
        printg("ERROR: Could not open binary file.\n");
        return 0;
    }

    nOk = 1;
    // Sequentially read binary structures into the dynamic array
    while (nOk) {
        ptTemp = realloc(*pptProducts, (*pnNumProducts + 1) * sizeof(Product));
        
        if (!ptTemp) {
            printg("ERROR: Could not allocate memory.\n");
            nOk = 0;
        }
        else {
            // Update the grids pointer to the newly allocated memory.
            *pptProducts = ptTemp;   
            nBytesRead = read(nFd, &((*pptProducts)[*pnNumProducts]), sizeof(Product));

            if (0 > nBytesRead) {
                nOk = 0;
                break;
            }

            //If it has reached the end of the file.
            if (0 == nBytesRead){    
                break;
            }

            (*pnNumProducts)++;
        }
    }

    close(nFd);

    if (!nOk) { 
        freeProduct(pptProducts); 
    }

    return nOk;
}

/*********************************************************************
*  
*   @Name: startCitadelSystem
*   @Def: Given all necessary structures of the system, keeps the terminal running until inputed EXIT.
*   @Arg:   In: -
*           Out: ptMaester = pointer to typedef strucutre of Maester, ptProducts = pointer to typedef strucutre of Products, pnNumProducts = pointer to number of products
*   @Ret: -
*
*********************************************************************/

void startCitadelSystem(Maester *ptMaester, Product *ptProducts, int *pnNumProducts) {
    int nRunning    = 1;
    char *psCommand   = NULL;
    char **ppsWords    = NULL;
    int nNumWords   = 0;
    char *psBuffer   = NULL;
    int nBytes   = 0;
    int i   = 0;

    nBytes = asprintf(&psBuffer, "\nMaester of %s initialized. The board is set.\n", ptMaester->psNameRealm);
    checkAndPrint(nBytes, &psBuffer);

    // Interactive shell loop
    while (nRunning) {

        printg(BOLD "$ ");
        psCommand = scanc();
        printg(NORMAL);

        if (psCommand) {

            ppsWords = parseWords(psCommand, &nNumWords);

            if (0 < nNumWords) {    
                nRunning = detectCommand(ppsWords, nNumWords, ptMaester, ptProducts, pnNumProducts);

                // Free the words inside the command.
                for (i = 0; i < nNumWords; i++) {
                    freeVar(&ppsWords[i]);
                    
                }
                free(ppsWords);
                ppsWords = NULL;
            }
        }
     
        //CONTROL + C detected.
        else if (NULL == psCommand) {
            nBytes = asprintf(&psBuffer, "\nThe Maester of %s signs off. The ravens rest.\n\n", ptMaester->psNameRealm);
            checkAndPrint(nBytes, &psBuffer);
            nRunning = 0;
        }
        
        freeVar(&psCommand);

    }
}
