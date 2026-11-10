#ifndef __UTILS_H__
#define __UTILS_H__

/************************************************
*
* @File : utils.h
* @Purpose : Stores main functions and structures needed for the whole project.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

/* This define must be inlcuded prior to the System Includes due to the use of asprintf. 
If it is included after, and thus with the rest of constant definitions, there will always be an error.*/
#define _GNU_SOURCE

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// Constant Declarations
#define BOLD "\033[1m"
#define NORMAL "\033[0m"

// Custom Type Definitions
// Frame dimensions (protocol fixed at 320 bytes)
#define FRAME_SIZE    320
#define ORIGIN_SIZE    20
#define DEST_SIZE      20
#define DATA_MAX_SIZE (FRAME_SIZE - 1 - ORIGIN_SIZE - DEST_SIZE - 2 - 2)
// 1 (type) + 20 (origin) + 20 (dest) + 2 (data_len) + 2 (checksum) = 45 bytes overhead
// DATA_MAX_SIZE = 275 bytes

//Alliance status
#define ALLIANCE_DISCONNECTED   0
#define ALLIANCE_ACCEPTED       1
#define ALLIANCE_REJECTED       2
#define ALLIANCE_PENDING        3
#define ALLIANCE_SENT           4

//TimeOut references
#define TIMEOUT 120
#define NO_TIME ((time_t)-1)

// Frame types
#define TYPE_ALLIANCE_REQ    0x01
#define TYPE_SIGIL_DATA      0x02
#define TYPE_ALLIANCE_RESP   0x03
#define TYPE_PROD_LIST_REQ   0x11
#define TYPE_PROD_LIST_RESP  0x12
#define TYPE_PROD_LIST_DATA  0x13
#define TYPE_ORDER_REQ       0x14
#define TYPE_ORDER_DATA      0x15
#define TYPE_ORDER_RESP      0x16
#define TYPE_UNKNOWN_REALM   0x21
#define TYPE_NO_ALLIANCE     0x25
#define TYPE_PING            0x26
#define TYPE_DISCONNECT      0x27
#define TYPE_ACK_FILE        0x31
#define TYPE_ACK_MD5         0x32
#define TYPE_NACK            0x69

// Envoy missions
#define MISSION_NONE 0
#define MISSION_PLEDGE_REQUEST 1
#define MISSION_PLEDGE_RESPONSE 2
#define MISSION_LIST_PRODUCTS_REALM 3
#define MISSION_START_TRADE 4

typedef struct {
    char *psNameProduct;
    int nAmount;
} ShoppingItem;

typedef struct {
    char sNameProduct[100];
    int nAmount;
    float fWeight;
} Product;


typedef struct {
    char *sNameRealm;
    char *sIp;
    int nPort;
    int tStatus;
    time_t time; 

    Product *products;
    int numProducts;
} Alliance;

typedef struct {
    char *psNameRoute;
    char *psIP;
    int nPort;
} Route;

typedef struct {
    int type;           
    pid_t envoy_pid;

    char origin[ORIGIN_SIZE];
    char originIP[ORIGIN_SIZE];
    int originPort;

    char destination[DEST_SIZE];
    char destinationIP[DEST_SIZE];
    int destinationPort;

    char data[DATA_MAX_SIZE];   //mirar aixo

    int success;
} Mission;



typedef struct {
    int id;
    int isFree;

    int to_envoy[2];     // Maester -> Envoy
    int from_envoy[2];   // Envoy -> Maester

    int mission;
    char *destination;
    pid_t pid;
} Envoy;

typedef struct {
    char *psNameRealm;
    char *psPathFolder;
    char *psIP;
    int nPort;

    Envoy *ptEnvoys;
    int nNumEnvoys;

    Route *ptRoutes;
    int nNumRoutes;
    
    Alliance *ptAlliances;
    int nNnumAlliances;

    int fd_listen;

    char *stockName;

    pthread_mutex_t mutex_alliances;
    pthread_mutex_t mutex_products;
    pthread_mutex_t mutex_envoys;
    
} Maester;

typedef struct {
    int listen_fd;
    Maester *maester;
    Product *products;
    int numProducts;

    int exit;
    pthread_mutex_t mutex_exit;
} ServerComm;

typedef struct {
    uint8_t  type;
    char     origin[ORIGIN_SIZE];
    char     dest[DEST_SIZE];
    uint16_t data_len;
    char     data[DATA_MAX_SIZE];
    uint16_t checksum;
} Frame;


// Prototypes
void printg(char *psText);
void printgFile(int nFd, char *psText);

void freeVar(char **ppsBuffer);
void checkAndPrint(int nBytes, char **ppsBuffer);
void checkAndPrintFile(int nFd, int nBytes, char **ppsBuffer);

char* scanc();
char* scancFile(int nFd);
char *readLineFile(int nFd);

char **parseWords(char *psCommand, int *pnNumWords);
void removeAmpersand(char *psWord);

void splitIpPort(char *buffer, char **originIP, char **originPort);

void createAlliance(Maester *me, char *realm);
void updateAlliance(Maester *me, char *realmName, char *IP, int port, int status, time_t time);

Route* findRoute(Maester *me, char *realm);
Alliance *findAlliance(Maester *me, char *realm);
Envoy *findFreeEnvoy(Maester *me);


#endif