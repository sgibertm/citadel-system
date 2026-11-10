/************************************************
*
* @File : threadHandler.c
* @Purpose : Implements the functions that handle the different types of frames received by the Maester, both for direct communication and for hops. These functions are called by the threads created in the main function when a frame is received.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own includes
#include "threadHandler.h"

/*********************************************************************
*  
*   @Name: check_timeOuts
*   @Def: Checks for any pending or sent alliances that have timed out and updates their status to rejected if they have.
*   @Arg:   In: -
*           Out: me = pointer to the Maester struct containing the Maester's information and alliances.
*   @Ret: -
*
*********************************************************************/

void check_timeOuts(Maester *me) {

    time_t now = time(NULL);

    for (int i = 0; i < me->nNnumAlliances; i++) {

        Alliance *a = &me->ptAlliances[i];

        if (a == NULL) {
            continue;
        }

        if ((a->tStatus == ALLIANCE_PENDING || a->tStatus == ALLIANCE_SENT) && a->time != NO_TIME && (now - a->time) >= TIMEOUT) {

            pthread_mutex_lock(&me->mutex_alliances);

            char *buffer = NULL;
            int bytes = 0;
            bytes = asprintf(&buffer, NORMAL "\r>>> Alliance with %s rejected due timeout.\n", a->sNameRealm);
            checkAndPrint(bytes, &buffer);
            printg(BOLD "$ ");
            
            updateAlliance(me, a->sNameRealm, "RESET", 0, ALLIANCE_REJECTED, 0);
            
            pthread_mutex_unlock(&me->mutex_alliances);

            pthread_mutex_lock(&me->mutex_envoys);
            for (int i = 0; i < me->nNumEnvoys; i++) {
                Envoy *envoy = &me->ptEnvoys[i];
                if (envoy->mission == MISSION_PLEDGE_REQUEST && envoy->destination != NULL && strcasecmp(envoy->destination, a->sNameRealm) == 0) {
                    
                    envoy->isFree = 1;
                    envoy->mission = MISSION_NONE;
                    free(envoy->destination);
                    envoy->destination = NULL;

                    bytes = asprintf(&buffer, "Envoy from mission to %s is now available.\n", a->sNameRealm);
                    checkAndPrint(bytes, &buffer);

                }
            }
            pthread_mutex_unlock(&me->mutex_envoys);


        }
    }
    return;
}


/*********************************************************************
*  
*   @Name: pledge_request
*   @Def: Handles the pledge request frame.
*   @Arg:   In: sockfd = file descriptor of the socket to send the acknowledgment
*           Out: me = pointer to the Maester struct containing the Maester's information and alliances.
*   @Ret: -
*
*********************************************************************/

void pledge_request(Frame *f, int sockfd, Maester *me) {
    int bytes = 0;
    char *buffer = f->origin;
    char *originIP = NULL;
    char *originPort = NULL;
    char *originName = NULL;
    
    uint16_t data_len = ntohs(f->data_len);   // convert ONCE, reuse everywhere
    char *copy = malloc(data_len + 1);
    if (!copy) {
        return;
    } 
    memcpy(copy, f->data, data_len);
    copy[data_len] = '\0';

    originName = strtok(copy, "&");
    char *filename = strtok(NULL, "&");
    char *filesize_str = strtok(NULL, "&");
    char *md5 = strtok(NULL, "&");

    if (!originName || !filename || !filesize_str || !md5) {
        printg("ERROR: Could not separate data from header\n");
        free(copy);
        return;
    }

    int filesize = atoi(filesize_str);
    char *md5_saved = strdup(md5);
    char *name_saved = strdup(originName);

    splitIpPort(f->origin, &originIP, &originPort);

    if (strcasecmp(f->dest, me->psNameRealm) == 0) {

        bytes = asprintf(&buffer, NORMAL "\r>>> Alliance request received from %s.\n", name_saved);
        checkAndPrint(bytes, &buffer);

        Alliance *ally = findAlliance(me, name_saved);
        if (ally) {
            if (ally->tStatus == ALLIANCE_ACCEPTED) {
                bytes = asprintf(&buffer, "The realm %s is already an ally, cannot accept pledge.\n", name_saved);
                checkAndPrint(bytes, &buffer);
                free(md5_saved);   
                free(name_saved);   
                free(originIP);
                free(originPort);
                free(copy);
                return;
            }
        }
        else {
            createAlliance(me, name_saved);
        }
        updateAlliance(me, name_saved, originIP, atoi(originPort), ALLIANCE_PENDING, time(NULL));
        
        free(originIP);
        free(originPort);
        free(copy);

        send_ack(sockfd, me);

        char *filepath = NULL;
        asprintf(&filepath, "REALMS/%s/%s.png", me->psPathFolder, name_saved);
        
        int fd = open(filepath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            printg("ERROR: Could not create new sigil file.\n");
            free(filepath);
            return;
        }

        Frame chunk;
        int total_received = 0;
        while (total_received < filesize) {
            if (get_frame(sockfd, &chunk) < 0 || chunk.type != TYPE_SIGIL_DATA) {
                close(fd);
                free(filepath);
                return;
            }
            uint16_t chunk_len = ntohs(chunk.data_len);
            write(fd, chunk.data, chunk_len);
            total_received += chunk_len;
        }
        close(fd);

        Frame ack_md5;
        char *supposed_md5 = calculate_md5sum(filepath);
        char *data = NULL;
        if (strcmp(supposed_md5, md5_saved) == 0) {
            asprintf(&data, "CHECK_OK&%s", me->psNameRealm);
        }
        else {
            asprintf(&data, "CHECK_KO&%s", me->psNameRealm);
            ally->numProducts = 0;
        }
        build_frame(&ack_md5, TYPE_ACK_MD5, name_saved, me->psNameRealm, (uint16_t)strlen(data), data);
        send_frame(sockfd, &ack_md5);

        free(md5_saved);
        free(supposed_md5);
        free(name_saved);
        free(data);
        free(filepath);

        return;
    }

    else {
        char *buffer = NULL;
        bytes = asprintf(&buffer, NORMAL "\r>>> Received hop: %s → %s\n", name_saved, f->dest);
        checkAndPrint(bytes, &buffer);

        Route *route = findRoute(me, f->dest);
        if (!route) {
            printg("ERROR: Unknown realm\n");
            free(md5_saved); 
            free(name_saved); 
            free(originIP);
            free(originPort);
            free(copy);
            return;
        }

        char *IP = route->psIP;
        int port = route->nPort;
        char *routeName = route->psNameRoute;

        if (strcmp(IP, "*.*.*.*") == 0) {
            Route *defaultRoute = findRoute(me, "DEFAULT");
            IP = defaultRoute->psIP;
            port = defaultRoute->nPort;
            routeName = defaultRoute->psNameRoute;

            bytes = asprintf(&buffer, NORMAL "Route not found, sending to default: %s → %s:%d\n",routeName, IP, port);
            checkAndPrint(bytes, &buffer);
        }
        else {
            bytes = asprintf(&buffer, NORMAL "Route found: %s → %s:%d\n",routeName, IP, port);
            checkAndPrint(bytes, &buffer);
        }

        int new_sock = connect_to_maester(IP, port);
        if (new_sock < 0) {
            printg(NORMAL "\rERROR: Cannot forward pledge\n");
            free(originIP);
            free(originPort);
            free(md5_saved);  
            free(name_saved);  
            free(copy); 
            return;
        }

        bytes = asprintf(&buffer, NORMAL "Forwarding PLEDGE request...\n");
        checkAndPrint(bytes, &buffer);

        send_frame(new_sock, f);

        Frame frameResponse;
        if (get_frame(new_sock, &frameResponse) < 0 || frameResponse.type != TYPE_ACK_FILE) {
            printg("ERROR: Did not recieve ACK FILE from hop.\n");
            free(originIP);
            free(originPort);
            free(copy);
            free(md5_saved); 
            free(name_saved);  
            close(new_sock);
            return;
        }
        
        send_frame(sockfd, &frameResponse);

        int total_relayed = 0;
        Frame chunk;
        while (total_relayed < filesize) {
            if (get_frame(sockfd, &chunk) < 0 || chunk.type != TYPE_SIGIL_DATA) {
                printg("ERROR: Did not get data CHUNK.\n");
                free(originIP); 
                free(originPort);
                free(md5_saved); 
                free(name_saved);
                free(copy); 
                close(new_sock);
                return;
            }

            send_frame(new_sock, &chunk);
            total_relayed += ntohs(chunk.data_len);
        }

        Frame ackMd5;
        if (get_frame(new_sock, &ackMd5) < 0 || ackMd5.type != TYPE_ACK_MD5) {
            printg("ERROR: Could not get ACK MD5 from hop.\n");
            free(originIP); 
            free(originPort);
            free(md5_saved); 
            free(name_saved);
            free(copy); 
            close(new_sock);
            return;
        }
        send_frame(sockfd, &ackMd5);

        free(originIP);
        free(originPort);
        free(md5_saved); 
        free(name_saved);
        free(copy);
        close(new_sock);
    }
}


/*********************************************************************
*  
*   @Name: pledge_response
*   @Def: Handles the pledge response frame, both for direct responses and hops.
*   @Arg:   In: -
*           Out: f = pointer to the Frame struct containing the received frame data, me = pointer to the Maester struct containing the Maester's information and alliances.
*   @Ret: -
*
*********************************************************************/

void pledge_response(Frame *f, int sockfd, Maester *me, char *nameOrigin) {
    if (!f || !me) {
        return;
    }

    int bytes = 0;
    char *buffer = NULL;
    char *originIP = NULL;
    char *originPort = NULL;

    splitIpPort(f->origin, &originIP, &originPort);

    uint16_t data_len = ntohs(f->data_len);
    char *data_copy = malloc(data_len + 1);
    if (!data_copy) { 
        free(originIP); 
        free(originPort); 
        return; 
    }
    memcpy(data_copy, f->data, data_len);
    data_copy[data_len] = '\0';

    char *response = strtok(data_copy, "&");
    char *originName = strtok(NULL, "&");

    if (!response || !originName) {
        free(data_copy);
        free(originIP);
        free(originPort);
        return;
    }



    if (strcasecmp(f->dest, me->psNameRealm) == 0) {

        if (strcmp(response, "ACCEPT") == 0) {
            bytes = asprintf(&buffer, NORMAL "\r>>> Alliance with %s forged successfully!\n", originName);
            checkAndPrint(bytes, &buffer);
            updateAlliance(me, originName, originIP, atoi(originPort), ALLIANCE_ACCEPTED, NO_TIME);

        } 
        else {
            bytes = asprintf(&buffer, NORMAL "\r>>> Alliance with %s rejected.\n", originName);
            checkAndPrint(bytes, &buffer);
            updateAlliance(me, originName, "RESET", 0, ALLIANCE_REJECTED, NO_TIME);
        }

        strcpy(nameOrigin, originName);
        bytes = asprintf(&buffer, "Envoy from mission to %s is now available.\n", originName);
        checkAndPrint(bytes, &buffer);


        send_ack(sockfd, me);

        free(data_copy);
        free(originIP);
        free(originPort);
    }

    else {
        bytes = asprintf(&buffer, NORMAL "\r>>> Received RESPONSE hop: %s → %s\n", originName, f->dest);
        checkAndPrint(bytes, &buffer);

        Route *route = findRoute(me, f->dest);
        if (!route) {
            printg("ERROR: Unknown realm\n");
            free(data_copy);
            free(originIP);
            free(originPort);
            return;
        }

        char *IP = route->psIP;
        int port = route->nPort;
        char *routeName = route->psNameRoute;

        if (strcmp(IP, "*.*.*.*") == 0) {
            Route *defaultRoute = findRoute(me, "DEFAULT");
            IP = defaultRoute->psIP;
            port = defaultRoute->nPort;
            routeName = defaultRoute->psNameRoute;

            bytes = asprintf(&buffer, NORMAL "Route not found, sending to default: %s → %s:%d\n",routeName, IP, port);
            checkAndPrint(bytes, &buffer);
        }
        else {
            bytes = asprintf(&buffer, NORMAL "Route found: %s → %s:%d\n",routeName, IP, port);
            checkAndPrint(bytes, &buffer);
        }

        int new_sock = connect_to_maester(IP, port);
        if (new_sock < 0) {
            printg(NORMAL "\rERROR: Cannot forward pledge\n");
            free(data_copy);
            free(originIP);
            free(originPort);
            return;
        }

        bytes = asprintf(&buffer, NORMAL "Forwarding PLEDGE response...\n");
        checkAndPrint(bytes, &buffer);

        send_frame(new_sock, f);

        Frame frameResponse;
        int recievedFrame = get_frame(new_sock, &frameResponse);
        if (recievedFrame < 0 || frameResponse.type != TYPE_ACK_FILE) {
            free(originIP);
            free(originPort);
            free(data_copy);
            close(new_sock);
            return;
        }
        
        send_frame(sockfd, &frameResponse);
        
        free(originIP);
        free(originPort);
        free(data_copy);
        close(new_sock);
    }
}


/*********************************************************************
*  
*   @Name: list_product_request
*   @Def: Handles the list product request frame, both for direct requests and hops.
*   @Arg:   In: connection_fd = file descriptor of the socket to send the acknowledgment and the product list frames
*           Out: f = pointer to the Frame struct containing the received frame data, me = pointer to the Maester struct containing the Maester's information and alliances.
                    me = pointer to the Maester struct containing the Maester's information and alliances.
*   @Ret: -
*
*********************************************************************/

void list_product_request(Frame *f, int connection_fd, Maester *me) {

    char *buffer = NULL;
    int nBytes = 0;

    uint16_t data_len = ntohs(f->data_len);

    char *data_copy = malloc(data_len + 1);
    if (!data_copy) {
        return;
    }

    memcpy(data_copy, f->data, data_len);
    data_copy[data_len] = '\0';

    char *destination = strtok(data_copy, "&");
    if (!destination) {
        free(data_copy);
        return;
    }

    Alliance *ally = findAlliance(me, destination);
    if (!ally) {
        nBytes = asprintf(&buffer, NORMAL "\rERROR: Could not trade with realm %s, not an ally.\n", destination);
        checkAndPrint(nBytes, &buffer);
        free(data_copy);
        return;
    }

    nBytes = asprintf(&buffer, NORMAL "\r>>> LIST PRODUCTS request from %s.\n", destination);
    checkAndPrint(nBytes, &buffer);

    char *pathStock = NULL;
    char *origin = NULL;
    char *md5 = NULL;
    char *data = NULL;
    
    asprintf(&pathStock, "%s", me->stockName);
    asprintf(&origin, "%s:%d", me->psIP, me->nPort);    
    md5 = calculate_md5sum(pathStock);

    int fd = open(pathStock, O_RDONLY);
    if (fd < 0) {
        printg("ERROR: Invalid sigil file. Please Pledge again.\n");
        free(data_copy);
        return;
    }
    int filesize = 0;
    char c;
    // Read exactly 1 byte at a time until EOF
    while (read(fd, &c, 1) > 0) {
        filesize++;
    }
    close(fd);

    Frame resp;
    asprintf(&data, "%s&%d&%s", me->stockName, filesize, md5);
    build_frame(&resp, TYPE_PROD_LIST_RESP, origin, destination, (uint16_t)strlen(data), data);
    send_frame(connection_fd, &resp);

    free(data);
    free(md5);

    Frame ack31;
    if (get_frame(connection_fd, &ack31) < 0 || ack31.type != TYPE_ACK_FILE) {
        printg("ERROR: Did not receive ACK FILE.\n");
        free(origin);
        free(data_copy);
        return;
    }

    printg("\rSending product list.\n");

    fd = open(me->stockName, O_RDONLY);
    if (fd < 0) {
        free(origin);
        free(data_copy);
        close(fd);
        return;
    }

    Product prod;
    while (read(fd, &prod, sizeof(Product)) == sizeof(Product)) {
        Frame dataFrame;
        build_frame(&dataFrame, TYPE_PROD_LIST_DATA, origin, destination,(uint16_t)sizeof(Product), (char *)&prod);
        send_frame(connection_fd, &dataFrame);
    }
    close(fd);

    Frame md5_resp;
    if (get_frame(connection_fd, &md5_resp) < 0 || md5_resp.type != TYPE_ACK_MD5) {
        printg("ERROR: Did not receive ACK MD5.\n");
        free(origin);
        free(data_copy);
        free(pathStock);
        return;
    }

    uint16_t hdrLen = ntohs(md5_resp.data_len);
    char *hdr = malloc(hdrLen + 1);
    
    if (!hdr) {
        free(origin);
        free(data_copy);
        free(pathStock);
        close(connection_fd);
        return;
    }
    
    memcpy(hdr, md5_resp.data, hdrLen);
    hdr[hdrLen] = '\0';

    char *itsok = strtok(hdr, "&");
    char *deliveryRealm = strtok(NULL, "&"); 

    if (itsok) {
        nBytes = asprintf(&buffer, "\rProducts delivered to %s.\n", deliveryRealm);
        checkAndPrint(nBytes, &buffer);
    }
    else {
        printg("ERROR: Products recalled due to uncorrectly delivery.\n");
    }

    free(hdr);
    free(origin);
    free(pathStock);
    free(data_copy);
}


/*********************************************************************
*  
*   @Name: order_request
*   @Def: Handles the order request frame, both for direct requests and hops.
*   @Arg:   In: trade_socket = file descriptor of the socket to send the acknowledgment and the product list frames
*           Out: f = pointer to the Frame struct containing the received frame data, 
*               trade_socket = file descriptor of the socket to send the acknowledgment and the product list frames,
*               me = pointer to the Maester struct containing the Maester's information and alliances
*               products = pointer to typedef structure of Products of products available to be traded,
*               numProducts = number of products available to be traded.
*   @Ret: -
*
*********************************************************************/

void order_request(Frame *f, int trade_socket, Maester *me, Product *products, int numProducts) {

    char *originIP = NULL;
    char *originPort = NULL;
    int originPortNum = 0;
    char *originName = NULL;

    splitIpPort(f->origin, &originIP, &originPort);
    originPortNum = atoi(originPort);

    for (int i = 0; i < me->nNnumAlliances; i++) {
        if (me->ptAlliances[i].sIp != NULL && strcmp(me->ptAlliances[i].sIp, originIP) == 0 && me->ptAlliances[i].nPort == originPortNum) {
            originName = me->ptAlliances[i].sNameRealm;
            break;
        }
    }

    free(originIP);
    free(originPort);

    if (originName) {
        char *buffer = NULL;
        int bytes = asprintf(&buffer, NORMAL "\r>>> Trade request received from %s.\n", originName);
        checkAndPrint(bytes, &buffer);
    } 
    else {
        printg("ERROR: Could not find trade's Realm\n");
        return;
    }

    uint16_t hdr_len = ntohs(f->data_len);
    char *hdr_copy = malloc(hdr_len + 1);
    if (!hdr_copy) {
        return;
    }
    memcpy(hdr_copy, f->data, hdr_len);
    hdr_copy[hdr_len] = '\0';

    char *filename_received = strtok(hdr_copy, "&");
    char *fsize_received = strtok(NULL, "&");
    char *md5_received = strtok(NULL, "&");

    if (!filename_received || !fsize_received || !md5_received) {
        printg("ERROR: Could not recieve order headder.\n");
        free(hdr_copy);
        return;
    }

    int filesize = atoi(fsize_received);
    char *md5_saved = strdup(md5_received);

    send_ack(trade_socket, me);


    char *pathShoppingList = NULL;
    asprintf(&pathShoppingList, "REALMS%s/%s_shopping_list.txt", me->psPathFolder, originName);

    int tmpfd = open(pathShoppingList, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (tmpfd < 0) {
        printg("ERROR: Could not create temp file for order.\n");
        free(md5_saved);
        free(hdr_copy);
        free(pathShoppingList);
        return;
    }

    int total_received = 0;
    Frame incoming;
    while (total_received < filesize) {
        if (get_frame(trade_socket, &incoming) < 0 || incoming.type != TYPE_ORDER_DATA) {
            printg("ERROR: Could not finish order file transfer.\n");
            close(tmpfd);
            free(md5_saved);
            free(hdr_copy);
            free(pathShoppingList);
            return;
        }
        uint16_t chunk_len = ntohs(incoming.data_len);
        write(tmpfd, incoming.data, chunk_len);
        total_received += chunk_len;
    }
    close(tmpfd);

    char *md5_computed = calculate_md5sum(pathShoppingList);

    Frame ack_md5; 
    char *ack_data = NULL;
    int itsok = 0;
    if (strcmp(md5_computed, md5_saved) == 0) {
        itsok = 1;
        asprintf(&ack_data, "CHECK_OK&%s", me->psNameRealm);
    }
    else {
        asprintf(&ack_data, "CHECK_KO&%s", me->psNameRealm);
        printg("ERROR: Could not resolve trade due to file corruption.\n");
    }
    build_frame(&ack_md5, TYPE_ACK_MD5, me->psNameRealm, originName, (uint16_t)strlen(ack_data), ack_data);
    send_frame(trade_socket, &ack_md5);

    
    free(md5_computed);
    free(md5_saved);

    char *reason = NULL;
    if (itsok) {
        reason = updateStockFromOrder(pathShoppingList, me->stockName, products, numProducts);
        if (!reason) {
            printg("ERROR: Could not update stock.\n");
        }
        else {
            
            Frame order_resp;
            char *origin = NULL;
            char *data = NULL;
            asprintf(&origin, "%s:%d", me->psIP, me->nPort);
            if (strcasecmp(reason, "OK") == 0) {
                asprintf(&data, "%s", reason);
            }
            else {
                asprintf(&data, "%s&%s", "REJECT", reason);
            }
            
            build_frame(&order_resp, TYPE_ORDER_RESP, origin, me->psNameRealm, (uint16_t)strlen(data), data);
            send_frame(trade_socket, &order_resp);

            free(origin);
            free(data);
        }
        
    }

    free(pathShoppingList);
    free(ack_data);
    free(hdr_copy);
}


/*********************************************************************
*  
*   @Name: disconnect_maester
*   @Def: Handles the disconnect frame, both for direct disconnections and hops.
*   @Arg:   In: -
*           Out: f = pointer to the Frame struct containing the received frame data, me = pointer to the Maester struct containing the Maester's information and alliances.
*   @Ret: -
*
*********************************************************************/

void disconnect_maester(Frame *f, Maester *me) {

    char *originIP = NULL;
    char *originPort = NULL;
    int originPortNum = 0;
    char *originName = NULL;

    splitIpPort(f->origin, &originIP, &originPort);

    originPortNum = atoi(originPort);

    for (int i = 0; i < me->nNnumAlliances; i++) {

        if (me->ptAlliances[i].sIp != NULL && strcmp(me->ptAlliances[i].sIp, originIP) == 0 &&
            me->ptAlliances[i].nPort == originPortNum) {
            originName = me->ptAlliances[i].sNameRealm;
            break;
        }
    }

    if (originName) {

        updateAlliance(me, originName, "RESET", 0, ALLIANCE_DISCONNECTED, 0);

        char *buffer = NULL;
        int bytes = asprintf(&buffer, NORMAL "\r>>> Maester of %s has disconnected.\n", originName);
        checkAndPrint(bytes, &buffer);
    }

    free(originIP);
    free(originPort);

    return;

}
