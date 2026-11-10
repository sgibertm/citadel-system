/************************************************
*
* @File : utils.c
* @Purpose : Stores main functions/ prototypes frequently used throughout the whole project.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "utils.h"

/*********************************************************************
*  
*   @Name: printg
*   @Def: Writes on the screen whatever wants to be written.
*   @Arg:   In: -
*           Out: psText = pointer string of what wants to be written.
*   @Ret: -
*
*********************************************************************/

void printg(char *psText) {
    write(1, psText, strlen(psText));
}

/*********************************************************************
*  
*   @Name: printgFile
*   @Def: Writes in a file whatever wants to be written.
*   @Arg:   In: nFd = number of the file descriptor.
*           Out: psText = pointer string of what wants to be written.
*   @Ret: -
*
*********************************************************************/

void printgFile(int nFd, char *psText) {
    write(nFd, psText, strlen(psText));
}

/*********************************************************************
*  
*   @Name: freeVar
*   @Def: Frees any kind of buffer used in any code.
*   @Arg:   In: -
*           Out: ppsBuffer = double pointer to the buffer that needs to be freed.
*   @Ret: -
*
*********************************************************************/

void freeVar(char **ppsBuffer) {
    free(*ppsBuffer);
    *ppsBuffer = NULL;
}

/*********************************************************************
*  
*   @Name: checkAndPrint
*   @Def: Checks for every buffer used whether the memory allocation was succesfull or not.
*   @Arg:   In: nBytes = number of bytes that a buffer has.
*           Out: ppsBuffer = double pointer to the buffer that needs to be checked.
*   @Ret: -
*
*********************************************************************/

void checkAndPrint(int nBytes, char **ppsBuffer) {
    if (0 > nBytes) {
        printg("ERROR: Could not allocate memory.\n");
    } 
    else if (NULL != *ppsBuffer) {
        printg(*ppsBuffer);
    }

    freeVar(ppsBuffer);
}

/*********************************************************************
*  
*   @Name: checkAndPrintFile
*   @Def: Checks if memory allocation was a success and prints it into the file.
*   @Arg:   In: nFd = number of the file descriptor, nBytes = number of bytes that a buffer has.
*           Out: ppsBuffer = double pointer to the buffer that needs to be checked.
*   @Ret: -
*
*********************************************************************/

void checkAndPrintFile(int nFd, int nBytes, char **ppsBuffer) {
    if (0 > nBytes) {
        printg("ERROR: Could not allocate memory.\n");
        close(nFd);
    } 
    else if (NULL != *ppsBuffer) {
        printgFile(nFd, *ppsBuffer);
    }
    freeVar(ppsBuffer);
}

/*********************************************************************
*  
*   @Name: scanc
*   @Def: Reads a string inputed from the user.
*   @Arg:   In: -
*           Out: -
*   @Ret: Returns a string of the read line.
*
*********************************************************************/

char* scanc() {
    char *psBuffer  = NULL;
    int nBytes      = 0;
    char cCharacter;
    int nBytesRead  = 0;
    char *psTemp    = NULL;

    // Read characters one-by-one until a newline or error occurs
    while (0 < (nBytesRead = read(0, &cCharacter, 1)) && '\n' != cCharacter) {
        psTemp = realloc(psBuffer, nBytes + 1);
        if (!psTemp) {
            printg("ERROR: Could not allocate memory.\n");
            freeVar(&psBuffer);
            return NULL;
        }
        psBuffer = psTemp;
        
        psBuffer[nBytes] = cCharacter;
        nBytes++;
    }

    if (0 > nBytesRead) {
        freeVar(&psBuffer);
        return NULL;
    }    
    
    if (0 == nBytesRead && 0 == nBytes) {
        // TRUE EOF
        return NULL;
    }

    // Final allocation to hold the null terminator
    psTemp = realloc(psBuffer, nBytes + 1);
    if (!psTemp) {
        printg("ERROR: Could not allocate memory.\n");
        freeVar(&psBuffer);
        return NULL;
    }
    else {
        psBuffer = psTemp;
        psBuffer[nBytes] = '\0';
    }

    return psBuffer;
}

/*********************************************************************
*  
*   @Name: scancFile
*   @Def: Reads a string from a file.
*   @Arg:   In: nFd = number of the file descriptor.
*           Out: -
*   @Ret: Returns a string of the read line from file.
*
*********************************************************************/

char* scancFile(int nFd) {
    char *psBuffer  = NULL;
    int nBytes      = 0;
    char cCharacter;
    int nBytesRead  = 0;
    char *psTemp    = NULL;

    // Read until a delimiter (space or newline) or end of file
    while (0 < (nBytesRead = read(nFd, &cCharacter, 1)) && '\n' != cCharacter && ' ' != cCharacter) {
        psTemp = realloc(psBuffer, nBytes + 1);
        if (!psTemp) {
            printg("ERROR: Could not allocate memory.\n");
            freeVar(&psBuffer);
            return NULL;
        }
        psBuffer = psTemp;
        psBuffer[nBytes++] = cCharacter;
    }

    if (0 > nBytesRead) {
        freeVar(&psBuffer);
        return NULL;
    }
    
    if ( 0 == nBytesRead && 0 == nBytes) {
        // TRUE EOF
        return NULL;
    }

    // Close the string with a null terminator
    psTemp = realloc(psBuffer, nBytes + 1);
    if (!psTemp) {
        printg("ERROR: Could not allocate memory.\n");
        free(psBuffer);
        return NULL;
    }

    psBuffer = psTemp;
    psBuffer[nBytes] = '\0';

    return psBuffer;
}

/*********************************************************************
*  
*   @Name: readLineFile
*   @Def: Reads an entire line from a file.
*   @Arg:   In: nFd = number of the file descriptor.
*           Out: -
*   @Ret: Returns a read line from file.
*
*********************************************************************/

char *readLineFile(int nFd) {
    char cCharacter;
    char *psBuffer  = NULL;
    int nSize       = 0;
    char *psNewBuffer   = NULL;

    // Continue reading until a newline character is encountered
    while (0 < read(nFd, &cCharacter, 1)) {
        if ('\n' == cCharacter) {
            break;
        }

        // Reserve space for the current character plus the future null terminator
        psNewBuffer = realloc(psBuffer, nSize + 2);
        if (!psNewBuffer) {
            printg("ERROR: Could not allocate memory.\n");
            freeVar(&psBuffer);
            return NULL;
        }

        psBuffer = psNewBuffer;
        psBuffer[nSize] = cCharacter;
        nSize++;
    }

    if (psBuffer) {
        psBuffer[nSize] = '\0';
    }
        
    return psBuffer;
}

/*********************************************************************
*  
*   @Name: parseWords
*   @Def: Devides inputed command into different words.
*   @Arg:   In: -
*           Out: psCommand = string which is the inputed command of user, pnNumWords = number that counts how many words the command line has.
*   @Ret: Returns an array of strings containging all separated words from command.
*
*********************************************************************/

char **parseWords(char *psCommand, int *pnNumWords) {

    if (!psCommand || !pnNumWords) {
        return NULL;
    }

    char **ppsWords    = NULL;
    int nSameWord   = 0;
    int nWordIndex  = 0;
    int nStart      = 0;
    int nLength     = 0;
    int i;
    int j;

    *pnNumWords = 0;

    // Count number of words.
    for (i = 0; psCommand[i] != '\0'; i++) {
        if (' ' != psCommand[i] && !nSameWord) {
            nSameWord = 1;
            (*pnNumWords)++;
        }
        else if (' ' == psCommand[i]) {
            nSameWord = 0;
        }
    }

    // Fixed previously memory leak
    if (0 == *pnNumWords) {
        return NULL;
    }

    // Allocate the array of pointers for the discovered words
    ppsWords = malloc(sizeof(char*) * (*pnNumWords));

    nWordIndex = 0;

    for (i = 0; psCommand[i] != '\0'; ) {
        while (' ' == psCommand[i]) {
            i++;
        }
        
        nStart = i;
        while (' ' != psCommand[i] && '\0' != psCommand[i]) {
            i++;
        }

        nLength = i - nStart;
        if (0 < nLength) {

            ppsWords[nWordIndex] = malloc(nLength + 1);

            for (j = 0; j < nLength; j++) {
                ppsWords[nWordIndex][j] = psCommand[nStart + j];
            }

            ppsWords[nWordIndex][nLength] = '\0';
            nWordIndex++;
        }
    }
    return ppsWords;
}

/*********************************************************************
*  
*   @Name: removeAmpersand
*   @Def: Removes any & found in Realm name.
*   @Arg:   In: -
*           Out: psWord = pointer string to the word that wants to be changed.
*   @Ret: -
*
*********************************************************************/

void removeAmpersand(char *psWord) {
    int i = 0;
    int j = 0;

    while (psWord[i] != '\0') {
        if (psWord[i] != '&') {
            psWord[j++] = psWord[i];
        }
        i++;
    }
    psWord[j] = '\0';
}

/*********************************************************************
*  
*   @Name: splitIpPort
*   @Def:   Splits the IP and port from a string in the format ip:Port.
*   @Arg:   In: -
*           Out: buffer = string of the format ip:Port, 
                originIP = double pointer to string where the IP will be stored, 
                originPort = double pointer to string where the port will be stored.
*   @Ret: -
*
*********************************************************************/

void splitIpPort(char *buffer, char **originIP, char **originPort) {

    int index = -1;

    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == ':') {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return;
    }

    int ip_len = index;

    *originIP = malloc(ip_len + 1);
    if (!*originIP) {
        return;
    } 

    memcpy(*originIP, buffer, ip_len);
    (*originIP)[ip_len] = '\0';

    int start = index + 1;
    int end = start;

    while (buffer[end] != '\0' && buffer[end] != '#') {
        end++;
    }

    int port_len = end - start;

    *originPort = malloc(port_len + 1);
    if (!*originPort) {
        free(*originIP);
        return;
    }

    memcpy(*originPort, buffer + start, port_len);
    (*originPort)[port_len] = '\0';
}

/*********************************************************************
*  
*   @Name: createAlliance
*   @Def: Creates an alliance with a realm in the Maester's configuration.
*   @Arg:   In: port = integer of the port number of the realm
*           Out: me = pointer to the Maester struct, realm = string of the realm that wants to be allied, IP = string of the IP address of the realm.
*   @Ret: -
*
*********************************************************************/

void createAlliance(Maester *me, char *realm) {

    Alliance *temp = NULL;

    temp = realloc(me->ptAlliances, sizeof(Alliance) * (me->nNnumAlliances + 1));
    if(!temp) {
        printg("ERROR: Could not allocate memory.\n");
        return;
    }

    me->ptAlliances = temp;

    Alliance *newAlliance = &me->ptAlliances[me->nNnumAlliances];

    newAlliance->sNameRealm = malloc(strlen(realm) + 1);
    if (!newAlliance->sNameRealm) {
        printg("ERROR: Could not allocate memory.\n");
        return;
    }

    strcpy(newAlliance->sNameRealm, realm);
    newAlliance->sIp = NULL;
    newAlliance->nPort = 0;
    newAlliance->tStatus = ALLIANCE_DISCONNECTED;

    newAlliance->time = 0;

    newAlliance->products = NULL;
    newAlliance->numProducts = 0;

    me->nNnumAlliances++;
}

/*********************************************************************
*  
*   @Name: updateAlliance
*   @Def: Updates the status of an alliance with a realm in the Maester's configuration.
*   @Arg:   In: nStatus = integer of the new status of the alliance, which can be ALLIANCE_DISCONNECTED, ALLIANCE_PENDING, ALLIANCE_ACCEPTED or ALLIANCE_REJECTED.
*           Out: me = pointer to the Maester struct, realm = string of the realm that wants to be updated.
*   @Ret: -
*
*********************************************************************/

void updateAlliance(Maester *me, char *realmName, char *IP, int port, int status, time_t time) {

    Alliance *alliance = findAlliance(me, realmName);
    if (!alliance) {
        printg("ERROR: Alliance not found\n");
        return;
    }

    if (IP != NULL) {

        if (strcmp(IP, "RESET") == 0) {
            free(alliance->sIp);
            alliance->sIp = NULL;
        }
        else {

            free(alliance->sIp);

            alliance->sIp = malloc(strlen(IP) + 1);
            if (!alliance->sIp) {
                printg("ERROR: Could not allocate memory.\n");
                return;
            }
            strcpy(alliance->sIp, IP);
        }
    }

    if (port != -1) {
        alliance->nPort = port;
    }

    if (status != -1) {
        alliance->tStatus = status;
    }

    if (time != NO_TIME) {
        alliance->time = time;
    }

}


/*********************************************************************
*  
*   @Name: findRoute
*   @Def: Finds the route of a realm in the Maester's configuration.
*   @Arg:   In: -
*           Out: ptMaester = pointer to the Maester struct, realm = string of the realm that wants to be found.
*   @Ret: Returns a pointer to the route of the realm if found, or NULL if not found.
*
*********************************************************************/

Route* findRoute(Maester *me, char *realm) {
    for (int i = 0; i < me->nNumRoutes; i++) {
        if (strcasecmp(me->ptRoutes[i].psNameRoute, realm) == 0) {
            return &me->ptRoutes[i];
        }
    }
    return NULL;
}

/*********************************************************************
*  
*   @Name: findAlliance
*   @Def: Finds the alliance of a realm in the Maester's configuration.
*   @Arg:   In: -
*           Out: ptMaester = pointer to the Maester struct, realm = string of the realm that wants to be found.
*   @Ret: Returns a pointer to the alliance of the realm if found, or NULL if not found.
*
*********************************************************************/

Alliance *findAlliance(Maester *me, char *realm) {
    
    /*
    if (!me || !realm) {
        return NULL;
    }
    */
    
    for (int i = 0; i < me->nNnumAlliances; i++) {
        if (me->ptAlliances[i].sNameRealm && strcasecmp(me->ptAlliances[i].sNameRealm, realm) == 0) {
            return &me->ptAlliances[i];
        }
    }
    return NULL;
}


Envoy *findFreeEnvoy(Maester *me) {
    for (int i = 0; i < me->nNumEnvoys; i++) {
        if (me->ptEnvoys[i].isFree == 1) {
            return &me->ptEnvoys[i];
        }
    }

    return NULL;
}
