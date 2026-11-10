#ifndef __FRAME_H__
#define __FRAME_H__

/************************************************
*
* @File : frame.h 
* @Purpose : Define the structures and function prototypes needed to handle frames in the network communication.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

#include "../utils.h"
#include"../CONFIG/networkConfig.h"


uint16_t calculate_checksum(Frame *f);

int validate_checksum(Frame *f);

void build_frame(Frame *f, uint8_t type, char *origin, char *destination, uint16_t data_len, char *data);

int send_frame(int sockfd, Frame *f);

int get_frame(int sockfd, Frame *f);

void send_nack(int sockfd, Maester *me);
void send_nack_file(Frame *f, Maester *me);

void send_ack_frame(Frame *f, Maester *me);
void send_ack(int sockfd, Maester *me);

char *calculate_md5sum(char *pathSigil);

#endif
