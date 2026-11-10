#ifndef __ENVOYCONFIG_H__
#define __ENVOYCONFIG_H__


/************************************************
*
* @File : envoyConfig.h
* @Purpose : Header file for the Envoy module, defining functions and structures related to Envoy operations within the Maester system.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/



// Project Includes
#include "../utils.h"
#include "../COMMANDS/detectCommand.h"




// Prototypes
void envoy_process(int to_maester, int from_maester);

void execute_pledge_mission(Mission *mission, Mission *result);

void execute_pledge_response_mission(Mission *mission, Mission *result);

void execute_list_products_mission(Mission *mission, Mission *result);

void execute_trade_mission(Mission *mission, Mission *result);




int createEnvoys(Maester *maester);

void assignMission(Maester *maester, Mission mission);

void* envoy_thread(void* arg);

void check_envoyResponses(Maester *maester, Envoy *envoy, Mission result);


#endif