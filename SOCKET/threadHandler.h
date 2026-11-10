#ifndef __THREADHANDLER_H__
#define __THREADHANDLER_H__

/************************************************
*
* @File : threadHandler.c
* @Purpose : Implements the functions that handle the different types of frames received by the Maester, both for direct communication and for hops. These functions are called by the threads created in the main function when a frame is received.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

#include "../utils.h"
#include "frame.h"
#include "../CONFIG/networkConfig.h"
#include "../COMMANDS/START/tradeFunc.h"

void check_timeOuts(Maester *me);

void pledge_request(Frame *f, int sockfd, Maester *me);

void pledge_response(Frame *f, int sockfd, Maester *me, char *nameOrigin);

void list_product_request(Frame *f, int connection_fd, Maester *me);

void order_request (Frame *f, int trade_socket, Maester *me, Product *products, int numProducts);

void disconnect_maester(Frame *f, Maester *me);

#endif
