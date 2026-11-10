/************************************************
*
* @File : pledgeComm.c
* @Purpose : Is in charge of running all pledge commands the user might imput, and do its proper operations for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "pledgeComm.h"

/*********************************************************************
*  
*   @Name: pledgeRequest
*   @Def: Sends a pledge request to the Maester of the realm the user wants to ally with, with the sigil of the realm as data.
*   @Arg:   In: -
*           Out: ptMaester = pointer to the Maester struct, which contains all the information of the Maester, 
                destination = name of the realm the user wants to ally with, 
                sigil = name of the sigil file of the realm.
*   @Ret: -
*
*********************************************************************/

void pledgeRequest(Maester *ptMaester, char *destination, char *sigil) {

    char *buffer = NULL;
    int nBytes = 0;
    
    Alliance *ally = findAlliance(ptMaester, destination);
    if (ally) {

        if (ally->tStatus == ALLIANCE_PENDING || ally->tStatus == ALLIANCE_SENT) {
            nBytes = asprintf(&buffer, "Alliance with realm %s is pending.\n", destination);
            checkAndPrint(nBytes, &buffer);
            return;
        }

        if (ally->tStatus == ALLIANCE_ACCEPTED) {
            nBytes = asprintf(&buffer, "The realm %s is already an ally, cannot pledge.\n", destination);
            checkAndPrint(nBytes, &buffer);
            return;
        }
        
        if (ally->tStatus == ALLIANCE_DISCONNECTED) {
            nBytes = asprintf(&buffer, "Previous alliance with realm %s was disconnected. Sending new pledge request...\n", destination);
            checkAndPrint(nBytes, &buffer);
        }
            
        if (ally->tStatus == ALLIANCE_REJECTED) {
            nBytes = asprintf(&buffer, "Previous pledge to realm %s was rejected. Sending new pledge request...\n", destination);
            checkAndPrint(nBytes, &buffer);
        }
    }


    Route *ptRoute = findRoute(ptMaester, destination);
    if (!ptRoute) { 
        printg("ERROR: Unknown realm\n");
        return;
    }

    char *IP = ptRoute->psIP;
    int port = ptRoute->nPort;
    if (strcmp(ptRoute->psIP, "*.*.*.*") == 0) {
        Route *defaultRoute = findRoute(ptMaester, "DEFAULT");
        if (!defaultRoute) {
            printg("ERROR: Default route not found.\n");
        }
        IP = defaultRoute->psIP;
        port = defaultRoute->nPort;
    }

    char *pathSigil = NULL;
    asprintf(&pathSigil, "REALMS%s/%s", ptMaester->psPathFolder, sigil);

    Mission mission;
    memset(&mission, 0, sizeof(Mission));

    mission.type = MISSION_PLEDGE_REQUEST;

    strcpy(mission.origin, ptMaester->psNameRealm);
    strcpy(mission.originIP, ptMaester->psIP);
    mission.originPort = ptMaester->nPort;

    strcpy(mission.destination, destination);
    strcpy(mission.destinationIP, IP);
    mission.destinationPort = port;

    strcpy(mission.data, pathSigil);

    assignMission(ptMaester, mission);      

    free(pathSigil);

    return;
}

/*********************************************************************
*  
*   @Name: pledgeRespond
*   @Def: Responds to a pledge request from another Maester.
*   @Arg:  In: -
*          Out: me = pointer to the Maester struct, destination = name of the realm that sent the request, accept = boolean indicating whether to accept the pledge. 
*   @Ret: -
*
*********************************************************************/

void pledgeRespond(Maester *me, char *destination, int accept) {
    char *psBuffer = NULL;
    int nBytes = 0;

    Alliance *ally = findAlliance(me, destination);
    if (!ally) {
        printg("Unknown ally\n");
        return;
    }

    if (ally->tStatus != ALLIANCE_PENDING) {
        printg("No pending pledge from this realm\n");
        return;
    }


    Route *r = findRoute(me, destination);
    if (!r) {
        nBytes = asprintf(&psBuffer, "Unknown route to %s. Cannot send response.\n", destination);
        checkAndPrint(nBytes, &psBuffer);
        return;
    }

    char *IP = r->psIP;
    int port = r->nPort;
    if (strcmp(IP, "*.*.*.*") == 0) {
        Route *defaultRoute = findRoute(me, "DEFAULT");
        if (!defaultRoute) {
            printg("ERROR: Default route not found.\n");
            return;
        }
        IP = defaultRoute->psIP;
        port = defaultRoute->nPort;
    }


    Mission mission;
    memset(&mission, 0, sizeof(Mission));

    mission.type = MISSION_PLEDGE_RESPONSE;
    strcpy(mission.origin, me->psNameRealm);
    strcpy(mission.originIP, me->psIP);
    mission.originPort = me->nPort;

    strcpy(mission.destination, destination);
    strcpy(mission.destinationIP, IP);
    mission.destinationPort = port;
    
    if (accept) {
        strcpy(mission.data, "1");
    } else {
        strcpy(mission.data, "0");
    }


    assignMission(me, mission); 
    
    return;
}

/*********************************************************************
*  
*   @Name: pledgeStatus
*   @Def: Prints the status of all the alliances of the Maester.
*   @Arg:   In: -
*           Out: maester = pointer to the Maester struct, which contains all the information of the Maester, including its alliances.
*   @Ret: 
*
*********************************************************************/

void pledgeStatus(Maester *maester) {

    char *buffer = NULL;
    int nBytes = 0;
    char *status = NULL;
    char *realm = NULL;

    for (int i = 1; i < maester->nNumRoutes; i++) {

        realm = maester->ptRoutes[i].psNameRoute;

        Alliance *a = findAlliance(maester, realm);

        if (a) {
            switch (a->tStatus) {
                case ALLIANCE_PENDING:
                    status = "PENDING";
                    break;
                case ALLIANCE_ACCEPTED:
                    status = "ALLIED  ";
                    break;
                case ALLIANCE_REJECTED:
                    status = "FAILED  ";
                    break;
                case ALLIANCE_DISCONNECTED:
                    status = "DISCONNECTED";
                    break;
                case ALLIANCE_SENT:
                    status = "SENT    ";
                    break;
                default:
                    status = "UNKNOWN";
                    break;
            }
        }
        else {
            status = "UNKNOWN";
        }

        nBytes = asprintf(&buffer, "- %-13s: %s\n", maester->ptRoutes[i].psNameRoute, status);
        checkAndPrint(nBytes, &buffer);
    }
}

