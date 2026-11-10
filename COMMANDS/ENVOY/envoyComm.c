/************************************************
*
* @File : envoyComm.c
* @Purpose : Is in charge of running all envoy commands the user might input, and do its proper operations for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "envoyComm.h"


/*********************************************************************
*  
*   @Name: envoyStatus
*   @Def: Prints the status of all the envoys of the Maester.
*   @Arg:   In: -
*           Out: maester = pointer to the Maester struct, which contains all the information of the Maester, including its envoys.
*   @Ret: 
*
*********************************************************************/

void envoyStatus(Maester *maester) {

    char *buffer = NULL;
    int bytes = 0;

    for (int i = 0; i < maester->nNumEnvoys; i++) {

        Envoy *envoy = &maester->ptEnvoys[i];
        
        if (envoy->isFree) {
            bytes = asprintf(&buffer, "- ENVOY %d: FREE\n", envoy->id);
            checkAndPrint(bytes, &buffer);
        }
        else {
            bytes = asprintf(&buffer, "- ENVOY %d: ON MISSION (PLEDGE to %s)\n", envoy->id, envoy->destination);
            checkAndPrint(bytes, &buffer);
        }

    }
}


