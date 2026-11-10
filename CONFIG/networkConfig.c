/************************************************
*
* @File : networkConfig.c
* @Purpose : Define the structures and function prototypes needed to read and store the Maester and Stock configuration files and terminal.
* @Author : Sara Gibert & Oriol Pros
* @Date : 03/05/2026
*
***********************************************/
#include "networkConfig.h"

/*********************************************************************
*  
*   @Name: server_thread
*   @Def: Thread function for handling incoming connections on the server socket.
*   @Arg:   In: -
*           Out: arg = pointer to a ServerComm struct containing the listening socket file descriptor and a pointer to the Maester struct.
*   @Ret: -
*
*********************************************************************/

void* network_thread(void* arg) {
    ServerComm *args = (ServerComm*)arg;

    int listen_fd = args->listen_fd;
    Maester *myMaester = args->maester;
    Product *myProducts = args->products;
    int numProducts = args->numProducts;

    while (1) {

        pthread_mutex_lock(&args->mutex_exit);
        int exit = args->exit;
        pthread_mutex_unlock(&args->mutex_exit);
        
        if (exit) {
            break;
        }

        fd_set set;
        FD_ZERO(&set);

        int max_fd = listen_fd;

        FD_SET(listen_fd, &set);

        struct timeval timeout;
        timeout.tv_sec = 1;   // cada segon revisa timeout
        timeout.tv_usec = 0;

        int ready = select(max_fd + 1, &set, NULL, NULL, &timeout);

        check_timeOuts(myMaester);

        if (ready > 0 && FD_ISSET(listen_fd, &set)) {

            int connection_fd = accept_connection(listen_fd); 
            if (connection_fd < 0) {
                break;
            }

            Frame f;
            memset(&f, 0, sizeof(Frame));
        
            if (get_frame(connection_fd, &f) < 0) {
                close(connection_fd);
                continue;
            }

            if (!validate_checksum(&f)) {
                send_nack_file(&f, myMaester);
                printg(BOLD "$ ");
                close(connection_fd);
                break;
            }

            switch (f.type) {

                case TYPE_ALLIANCE_REQ:
                    //printg("Type 1\n");
                    pthread_mutex_lock(&myMaester->mutex_alliances);

                    pledge_request(&f, connection_fd, myMaester); 
                    printg(BOLD "$ ");

                    pthread_mutex_unlock(&myMaester->mutex_alliances);
                    break;

                case TYPE_ALLIANCE_RESP:
                    //printg("Type 3\n");
                    pthread_mutex_lock(&myMaester->mutex_alliances);

                    char origin[ORIGIN_SIZE];
                    memset(origin, 0, ORIGIN_SIZE);

                    pledge_response(&f, connection_fd, myMaester, origin);

                    pthread_mutex_unlock(&myMaester->mutex_alliances);


                    pthread_mutex_lock(&myMaester->mutex_envoys);
                    for (int i = 0; i < myMaester->nNumEnvoys; i++) {
                        Envoy *envoy = &myMaester->ptEnvoys[i];
                        if (envoy->mission == MISSION_PLEDGE_REQUEST && envoy->destination != NULL && strcasecmp(envoy->destination, origin) == 0) {
                            
                            envoy->isFree = 1;
                            free(envoy->destination);
                            envoy->destination = NULL;
                            envoy->mission = MISSION_NONE;

                        }
                    }
                    pthread_mutex_unlock(&myMaester->mutex_envoys);

                    printg(BOLD "$ ");
                    
                    break;

                case TYPE_PROD_LIST_REQ:
                    //printg("Type 11\n");
                    pthread_mutex_lock(&myMaester->mutex_products);

                    list_product_request(&f, connection_fd, myMaester);
                    printg(BOLD "$ ");

                    pthread_mutex_unlock(&myMaester->mutex_products);
                    break;

                case TYPE_ORDER_REQ:
                    //printg("Type 14\n");
                    pthread_mutex_lock(&myMaester->mutex_products);

                    order_request(&f, connection_fd, myMaester, myProducts, numProducts);
                    printg(BOLD "$ ");

                    pthread_mutex_unlock(&myMaester->mutex_products);
                    break;

                case TYPE_DISCONNECT:
                    //printg("Type 27\n");
                    pthread_mutex_lock(&myMaester->mutex_alliances);

                    disconnect_maester(&f, myMaester);
                    printg(BOLD "$ ");
                    
                    pthread_mutex_unlock(&myMaester->mutex_alliances);
                    break;
            }
        
            close(connection_fd);
        }
    }

    return NULL;
}


/*********************************************************************
*  
*   @Name: start_listen
*   @Def: Initializes a listening socket on the specified IP and port, and returns the file descriptor of the socket.
*   @Arg:   In: port = integer of the port number to listen on.
*           Out: ip = string of the IP address to listen on.
*   @Ret: integer of the file descriptor of the listening socket, or -1 if an error occurred.
*
*********************************************************************/

int start_listen(char *ip, int port) {
    struct sockaddr_in server_s;
    int fd_socket = -1;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket < 0) {
        printg("ERROR: Could not create socket\n");
        return ERROR_SOCKET;
    }

    memset(&server_s, 0, sizeof(server_s));
    server_s.sin_family = AF_INET;
    server_s.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_s.sin_addr) < 0) {
        printg("ERROR: Could not convert IP address\n");
        close(fd_socket);
        return ERROR_SOCKET;
    }

    if (bind(fd_socket, (void *)&server_s, sizeof(server_s)) < 0) {
        printg("ERROR: Could not bind to port\n");
        close(fd_socket);
        return ERROR_SOCKET;
    }

    listen(fd_socket, 5); 

    return fd_socket;
}


/*********************************************************************
*  
*   @Name: accept_connection
*   @Def: Accepts an incoming connection on the listening socket.
*   @Arg:   In: listen_fd - The file descriptor of the listening socket.
*           Out: -
*   @Ret: The file descriptor of the accepted connection, or ERROR_SOCKET on failure.
*
*********************************************************************/

int accept_connection (int listen_fd) { 
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int fd_socket = -1;
    
    fd_socket = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

    if (fd_socket < 0) {
        return ERROR_SOCKET;
    }
    return fd_socket;
}


/*********************************************************************
*  
*   @Name: connect_to_maester
*   @Def: Establishes a connection to another Maester given its IP and port, returning the file descriptor of the connected socket.
*   @Arg:   In: port = integer of the port number to connect to.
*           Out: ip = string of the IP address to connect to.
*   @Ret: The file descriptor of the connected socket, or -1 on failure.
*
*********************************************************************/

int connect_to_maester(const char *ip, int port) {
    
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        printg(NORMAL "\rERROR: Could not create socket\n");
        printg(BOLD "$ ");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        printg(NORMAL "\rERROR: Invalid IP\n");
        printg(BOLD "$ ");
        close(client);
        return -1;
    }

    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printg(NORMAL "\rERROR: Could not connect to Maester\n");
        printg(BOLD "$ ");
        close(client);
        return -1;
    }

    return client;
}


/*********************************************************************
*  
*   @Name: maesterDisconnection
*   @Def: Disconnects from another Maester.
*   @Arg:   In: -
*           Out: maester = pointer to the Maester struct representing the current Maester, containing information about its alliances and connection details.
*   @Ret: -
*
*********************************************************************/

void maesterDisconnection(Maester *maester) {

    char *buffer = NULL;
    int bytes = 0;

    char *origin = NULL;
    asprintf(&origin, "%s:%d", maester->psIP, maester->nPort);

    char *data = NULL;
    asprintf(&data, "DISCONNECT");

    for (int i = 0; i < maester->nNnumAlliances; i++) {

        if (maester->ptAlliances[i].tStatus == ALLIANCE_ACCEPTED && maester->ptAlliances[i].sIp != NULL) {

            int ally_socket = connect_to_maester(maester->ptAlliances[i].sIp, maester->ptAlliances[i].nPort);
            if (ally_socket < 0) {
                asprintf(&buffer, "ERROR: Could not send disconnection notificiation to realm %s.\n", maester->ptAlliances[i].sNameRealm);
                checkAndPrint(bytes, &buffer);
                continue;
            }
            else {
                Frame disconnect;
                char *destination = maester->ptAlliances[i].sNameRealm;
                build_frame(&disconnect, TYPE_DISCONNECT, origin, destination, (uint16_t)strlen(data), data);
                send_frame(ally_socket, &disconnect);

                close(ally_socket);
            }
        }
    }

    free(origin);
    free(data);
}



