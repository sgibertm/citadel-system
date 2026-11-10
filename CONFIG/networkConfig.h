#ifndef __NETWORKCONFIG_H__
#define __NETWORKCONFIG_H__

/************************************************
*
* @File : networkConfig.h
* @Purpose : Define the structures and function prototypes needed to read and store the Maester and Stock configuration files and terminal.
* @Author : Sara Gibert & Oriol Pros
* @Date : 03/05/2026
*
***********************************************/

// Project Includes
#include "../utils.h"
#include "../SOCKET/frame.h"
#include "../SOCKET/threadHandler.h"

#define ERROR_SOCKET -1

// Prototypes
void* network_thread(void* arg);

int start_listen(char *ip, int port);

int accept_connection(int listen_fd);

int connect_to_maester(const char *ip, int port);

void maesterDisconnection(Maester *maester);

#endif
