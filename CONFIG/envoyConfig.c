/************************************************
*
* @File : envoyConfig.c 
* @Purpose : Defines the functions for the Envoy processes and the Maester's management of them.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "envoyConfig.h"


/////////////
// ENVOY
/////////////

/*********************************************************************
*  
*   @Name: envoy_process
*   @Def: Main event loop for a forked Envoy process.
*         Reads missions from parent via from_maester pipe,
*         executes them (pledge, list, trade), and sends results back via to_maester.
*   @Arg:   In: to_maester = write fd for sending results to parent,
*               from_maester = read fd for receiving missions from parent,
*               envoy_id = unique envoy identifier.
*           Out: -
*   @Ret: -
*
*********************************************************************/

void envoyProcess(int to_maester, int from_maester) {
    Mission mission;
    Mission result;
    int nRead;

    while (1) {
        nRead = read(from_maester, &mission, sizeof(Mission));
        if (nRead <= 0) {
            break;
        }

        memset(&result, 0, sizeof(Mission));
        result.type = mission.type;
        result.success = 0;
        result.envoy_pid = mission.envoy_pid;
        strcpy(result.data, "");

        strcpy(result.origin, mission.origin);
        strcpy(result.originIP, mission.originIP);
        result.originPort = mission.originPort;

        strcpy(result.destination, mission.destination);
        strcpy(result.destinationIP, mission.destinationIP);
        result.destinationPort = mission.destinationPort;

        
        switch (mission.type) {
            case MISSION_PLEDGE_REQUEST:
                execute_pledge_mission(&mission, &result);
                break;
                
            case MISSION_PLEDGE_RESPONSE:
                execute_pledge_response_mission(&mission, &result);
                break;
                
            case MISSION_LIST_PRODUCTS_REALM:
                execute_list_products_mission(&mission, &result);
                break;
                
            case MISSION_START_TRADE:
                execute_trade_mission(&mission, &result);
                break;
                
            default:
                break;
        }

        write(to_maester, &result, sizeof(Mission));
    }
    
    close(to_maester);
    close(from_maester);

    return;
}


/*********************************************************************
*  
*   @Name: execute_pledge_mission
*   @Def: Envoy executes a pledge request mission.
*         Connects to target realm, sends sigil file with frame protocol.
*   @Arg:   In: mission = pointer to Mission struct with target realm info,
*           Out: result = pointer to Mission struct where result is stored.
*   @Ret: -
*
*********************************************************************/

void execute_pledge_mission(Mission *mission, Mission *result) {

    char *buffer = NULL;
    int bytes = 0;
    char *data = NULL;
    char *origin = NULL;
    
    int filesize = 0;
    char c;
    int fd = open(mission->data, O_RDONLY);
    if (fd < 0) {
        printg(NORMAL"\rERROR: Invalid sigil file. Please Pledge again.\n");
        printg(BOLD "$ ");
        return;
    }
    // Read exactly 1 byte at a time until EOF
    while (read(fd, &c, 1) > 0) {
        filesize++;
    }
    close(fd);
    
    int pledge_socket = connect_to_maester(mission->destinationIP, mission->destinationPort);
    if (pledge_socket < 0) {
        return;
    }
    
    Frame header;
    char *md5 = calculate_md5sum(mission->data);
    asprintf(&data, "%s&%s&%d&%s", mission->origin, mission->data, filesize, md5);
    asprintf(&origin, "%s:%d", mission->originIP, mission->originPort);  
    build_frame(&header, TYPE_ALLIANCE_REQ, origin, mission->destination, (uint16_t)strlen(data), data);
    send_frame(pledge_socket, &header);
    
    free(md5);
    free(data);
    

    Frame ack;
    int received = get_frame(pledge_socket, &ack);
    if (received < 0 || ack.type != TYPE_ACK_FILE) {
        bytes = asprintf(&buffer, NORMAL "\rERROR: No ACK received from realm %s\n", mission->destination);
        checkAndPrint(bytes, &buffer);
        printg(BOLD "$ ");
        free(origin);
        close(pledge_socket);
        return;
    }
    
    data = NULL;
    

    fd = open(mission->data, O_RDONLY);
    if (fd < 0) {
        printg(NORMAL "\rERROR: Could not open sigil file.\n");
        printg(BOLD "$ ");
        free(origin);
        close(pledge_socket);
        return;
    }
    
    int bytesRead = 0;
    char bufferRead[DATA_MAX_SIZE]; // static because it's the size in a Frame
    while ((bytesRead = read(fd, bufferRead, sizeof(bufferRead))) > 0) {
        Frame dataFrame;
        build_frame(&dataFrame, TYPE_SIGIL_DATA, origin, mission->destination, (uint16_t)bytesRead, bufferRead);
        send_frame(pledge_socket, &dataFrame);
    }
    close(fd);
    

    Frame md5_resp;
    if (get_frame(pledge_socket, &md5_resp) < 0 || md5_resp.type != TYPE_ACK_MD5) {
        printg(NORMAL "\rERROR: Did not receive ACK MD5.\n");
        printg(BOLD "$ ");
        free(origin);
        close(pledge_socket);
        return;
    }
    
    uint16_t hdrLen = ntohs(md5_resp.data_len);
    char *hdr = malloc(hdrLen + 1);
    if (!hdr) {
        free(origin);
        close(pledge_socket);
        return;
    }
    
    memcpy(hdr, md5_resp.data, hdrLen);
    hdr[hdrLen] = '\0';
    
    char *itsok = strtok(hdr, "&");
    char *deliveryRealm = strtok(NULL, "&");
    
    if (itsok) {

        if (strcmp(itsok, "CHECK_OK") == 0) {
            result->success = 1;
            strcpy(result->destination, deliveryRealm);
        }
        
        
    } else {
        printg(NORMAL "\rERROR: Could not pledge due to file corrupted.\n");
        printg(BOLD "$ ");
    }
    
    free(hdr);
    free(data);
    free(origin);
    close(pledge_socket);

    return;
}

/*********************************************************************
*  
*   @Name: execute_pledge_response_mission
*   @Def: Envoy responds to a pledge request from another realm.
*   @Arg:   In: mission = pointer to Mission (contains accept flag in data[0]),
*           Out: result = pointer to Mission.
*   @Ret: -
*
*********************************************************************/

void execute_pledge_response_mission(Mission *mission, Mission *result) {

    char *data = NULL;
    char *origin = NULL;
    
    int client = connect_to_maester(mission->destinationIP, mission->destinationPort);
    if (client < 0) {
        return;
    }
    
    Frame response;
    if (strcmp(mission->data, "1") == 0) {
        asprintf(&data, "ACCEPT&%s", mission->origin);
    } else {
        asprintf(&data, "REJECT&%s", mission->origin);
    }
    asprintf(&origin, "%s:%d", mission->originIP, mission->originPort);
    build_frame(&response, TYPE_ALLIANCE_RESP, origin, mission->destination, strlen(data), data);
    send_frame(client, &response);
    
    Frame ack;
    int received = get_frame(client, &ack);
    if (received == 0 && ack.type == TYPE_ACK_FILE) {

        result->success = 1;

        strcpy(result->destination, mission->destination);
        
        if (strcmp(mission->data, "1") == 0) {
            strcpy(result->data, "1");
        } 
        else {
            strcpy(result->data, "0");
        }
    }
    
    free(data);
    free(origin);
    close(client);
}

/*********************************************************************
*  
*   @Name: execute_list_products_mission
*   @Def: Envoy requests product list from target realm.
*   @Arg:   In: mission = pointer to Mission,
*           Out: result = pointer to Mission with product list.
*   @Ret: -
*
*********************************************************************/

void execute_list_products_mission(Mission *mission, Mission *result) {

    int list_socket = connect_to_maester(mission->destinationIP, mission->destinationPort);
    if (list_socket < 0) {
        return;
    }
    
    Frame f;
    char *data = NULL;
    asprintf(&data, "%s", mission->origin);
    char *origin = NULL;
    asprintf(&origin, "%s:%d", mission->originIP, mission->originPort);    
    build_frame(&f, TYPE_PROD_LIST_REQ, origin, mission->destination, (uint16_t)strlen(data), data);
    send_frame(list_socket, &f);
    
    Frame resp12;
    if (get_frame(list_socket, &resp12) < 0 || resp12.type != TYPE_PROD_LIST_RESP) {
        printg("ERROR: Did not receive product list header.\n");
        close(list_socket);
        free(data);
        free(origin);
        return;
    }
    
    // Parse the 0x12 header to get total products
    uint16_t hdrLen = ntohs(resp12.data_len);
    char *hdr = malloc(hdrLen + 1);
    if (!hdr) {
        close(list_socket);
        free(data);
        free(origin);
        return;
    }
    
    memcpy(hdr, resp12.data, hdrLen);
    hdr[hdrLen] = '\0';
    
    strtok(hdr, "&");
    char *fileSize = strtok(NULL, "&");
    char *md5 = strtok(NULL, "&");
    
    int fileSizeInt = atoi(fileSize);
    int totalProducts = fileSizeInt / sizeof(Product);
    char *md5_saved = strdup(md5);
    
    
    Frame ack;
    char *ack_data = NULL;
    asprintf(&ack_data, "OK&%s", mission->origin);
    build_frame(&ack, TYPE_ACK_FILE, "", "", (uint16_t)strlen(ack_data), ack_data);
    send_frame(list_socket, &ack);

    
    int fd = open(mission->data, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        printg("ERROR: Could not create new stock file.\n");
        free(hdr);
        free(data);
        free(origin);
        free(md5_saved);
        close(list_socket);
        return;
    }
    
    Frame chunk;
    int n = 0;

    while (n < totalProducts) {
        if (get_frame(list_socket, &chunk) < 0 || chunk.type != TYPE_PROD_LIST_DATA) {
            break;
        }
        
        Product prod;
        uint16_t chunk_len = ntohs(chunk.data_len);
        memcpy(&prod, chunk.data, chunk_len);
        
        write(fd, &prod, sizeof(Product));

        n++;
    }
    
    close(fd);
    
    //send md5
    Frame ack_md;
    char *supposed_md5 = calculate_md5sum(mission->data);
    
    
    char *ackmd5_data = NULL;
    
    if (strcmp(supposed_md5, md5_saved) == 0) {
        
        result->success = 1;

        strcpy(result->destination, mission->destination);
        
        char *result_data = NULL;
        asprintf(&result_data, "%s&%d", mission->data, totalProducts);
        strcpy(result->data, result_data);
        free(result_data);

        asprintf(&ackmd5_data, "CHECK_OK&%s", mission->origin);
    } 
    else {
        asprintf(&ackmd5_data, "CHECK_KO&%s", mission->origin);                
    }
    
    
    build_frame(&ack_md, TYPE_ACK_MD5, origin, mission->destination, (uint16_t)strlen(ackmd5_data), ackmd5_data);
    send_frame(list_socket, &ack_md);
    
    free(supposed_md5);
    free(md5_saved);
    free(data);
    free(ack_data);
    free(ackmd5_data);
    free(hdr);
    free(origin);
    close(list_socket);

    return;
}

/*********************************************************************
*  
*   @Name: execute_trade_mission
*   @Def: Envoy sends a trade order to target realm.
*   @Arg:   In: mission = pointer to Mission (data contains order filename),
*           Out: result = pointer to Mission with trade response.
*   @Ret: -
*
*********************************************************************/

void execute_trade_mission(Mission *mission, Mission *result) {

    char mission_data[DATA_MAX_SIZE];
    strcpy(mission_data, mission->data);

    char *filename = strtok(mission_data, "&");


    char *buffer = NULL;
    int bytes = 0;
    
    int trade_socket = connect_to_maester(mission->destinationIP, mission->destinationPort);
    if (trade_socket < 0) {
        return;
    }
    
    int filesize = 0;
    char c;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printg("ERROR: Could not open shopping list file.\n");
        close(trade_socket);
        return;
    }
    
    while (read(fd, &c, 1) > 0) {
        filesize++;
    }
    close(fd);
    
    char *md5 = calculate_md5sum(filename);
    if (!md5) {
        printg("ERROR: Could not compute MD5 of shopping list.\n");
        close(trade_socket);
        return;
    }
    
    Frame f;
    char *data = NULL;
    asprintf(&data, "%s&%d&%s", filename, filesize, md5);
    char *origin = NULL;
    asprintf(&origin, "%s:%d", mission->originIP, mission->originPort);
    build_frame(&f, TYPE_ORDER_REQ, origin, mission->destination, (uint16_t)strlen(data), data);
    send_frame(trade_socket, &f);
    
    free(data);
    free(md5);
    
    Frame ack31;
    if (get_frame(trade_socket, &ack31) < 0 || ack31.type != TYPE_ACK_FILE) {
        printg("ERROR: Did not receive ACK FILE.\n");
        free(origin);
        close(trade_socket);
        return;
    }
    
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printg("ERROR: Could not reopen shopping list for sending.\n");
        free(origin);
        close(trade_socket);
        return;
    }
    
    char chunk[DATA_MAX_SIZE]; // fixed-size buffer for reading file chunks, since max data size is defined in the protocol
    int n;
    while ((n = read(fd, chunk, DATA_MAX_SIZE)) > 0) {
        Frame data_frame;
        build_frame(&data_frame, TYPE_ORDER_DATA, origin, mission->destination, (uint16_t)n, chunk);
        send_frame(trade_socket, &data_frame);
    }
    
    close(fd);
    free(origin);
    
    Frame ack_md5;
    if (get_frame(trade_socket, &ack_md5) < 0 || ack_md5.type != TYPE_ACK_MD5) {
        printg("ERROR: Did not receive ACK MD5 for order transfer.\n");
        close(trade_socket);
        return;
    }
    
    Frame order_resp;
    if (get_frame(trade_socket, &order_resp) < 0 || order_resp.type != TYPE_ORDER_RESP) {
        printg("ERROR: Did not receive an order response from realm.\n");
        close(trade_socket);
        return;
    }
    
    if (strcasecmp(order_resp.data, "OK") == 0) {
        
        result->success = 1;

        strcpy(result->destination, mission->destination);

        strcpy(result->data, mission->data);

    } 
    else {

        uint16_t data_len = ntohs(order_resp.data_len);
        char *data_copy = malloc(data_len + 1);
        if (!data_copy) {
            return;
        } 

        memcpy(data_copy, order_resp.data, data_len);
        data_copy[data_len] = '\0';

        strtok(data_copy, "&");
        char *reason = strtok(NULL, "&");

        bytes = asprintf(&buffer, NORMAL "Trade order turned down for %s.\n", reason);
        checkAndPrint(bytes, &buffer);

        free(data_copy);

    }
    
    close(trade_socket);
}



//////////////
// MAESTER
//////////////

/*********************************************************************
*  
*   @Name: createEnvoys
*   @Def: Initializes the Envoy processes and their communication pipes.
*   @Arg:   In: maester = pointer to Maester struct with number of envoys.
*           Out: maester->ptEnvoys = pointer to array of initialized Envoy structs.
*   @Ret: 1 on success, 0 on failure.
*
*********************************************************************/

int createEnvoys(Maester *maester) {
    
    Envoy *temp = malloc(sizeof(Envoy) * maester->nNumEnvoys);
    if (!temp) {
        printg("ERROR: Could not allocate memory.\n");
        return 0;
    }

    memset(temp, 0, sizeof(Envoy) * maester->nNumEnvoys);
    maester->ptEnvoys = temp;

    for (int i = 0; i < maester->nNumEnvoys; i++) {

        int to_envoy[2];
        int from_envoy[2];
        pid_t pid;
        
        if (pipe(to_envoy) < 0 || pipe(from_envoy) < 0) {
            printg("ERROR: Could not create pipes for envoy.\n");
            return 0;
        }


        pid = fork();
        if (pid < 0) {
            printg("ERROR: Could not fork envoy process.\n");
            close(to_envoy[0]);
            close(to_envoy[1]);
            close(from_envoy[0]);
            close(from_envoy[1]);
            return 0;
        }

        if (pid == 0) {
            // Child process: Envoy
            close(to_envoy[1]);       // Child reads from to_envoy[0]
            close(from_envoy[0]);      // Child writes to from_envoy[1]
            
            envoyProcess(from_envoy[1], to_envoy[0]);
        } 
        else {
            // Parent process: Maester
            close(to_envoy[0]);        // Parent writes to to_envoy[1]
            close(from_envoy[1]);       // Parent reads from from_envoy[0]
            
            maester->ptEnvoys[i].to_envoy[1] = to_envoy[1];
            maester->ptEnvoys[i].from_envoy[0] = from_envoy[0];
            maester->ptEnvoys[i].pid = pid;
            maester->ptEnvoys[i].isFree = 1;
            maester->ptEnvoys[i].id = i + 1;
            maester->ptEnvoys[i].destination = NULL;
        }

    }

    return 1;
}


/*********************************************************************
*  
*   @Name: assignMission
*   @Def: Assigns a mission to a free Envoy. If all envoys are busy, informs the user.
*   @Arg:   In: maester = pointer to Maester struct with envoys,
*               mission = Mission struct with mission details to assign.
*           Out: -
*   @Ret: 1 on success, 0 on failure.
*
*********************************************************************/

void assignMission(Maester *maester, Mission mission) {

    Envoy *envoy = findFreeEnvoy(maester);
    if (!envoy) {
        printg("All envoys are occupied. Your command must wait.\n");
        return;
    }

    envoy->isFree = 0;
    envoy->mission = mission.type;




    mission.envoy_pid = envoy->pid;

    int written = write(envoy->to_envoy[1], &mission, sizeof(Mission));
    if (written <= 0) {
        printg("ERROR: Could not assign mission to envoy, not written in pipe.\n");
        envoy->isFree = 1;
        return;
    } 
}


/*********************************************************************
*  
*   @Name: envoy_thread
*   @Def: Thread function for the Maester to listen for Envoy responses.
*   @Arg:   In: arg = pointer to ServerComm struct containing Maester reference and synchronization primitives,
*           Out: -
*   @Ret: -
*
*********************************************************************/

void* envoy_thread(void* arg) {
    ServerComm *args = (ServerComm*)arg;
    
    Maester *me = args->maester;

    while (1) {

        pthread_mutex_lock(&args->mutex_exit);
        int exit = args->exit;
        pthread_mutex_unlock(&args->mutex_exit);
        
        if (exit) {
            break;
        }

        fd_set set;
        FD_ZERO(&set);

        int max_fd = -1;

        for (int i = 0; i < me->nNumEnvoys; i++) {

            int fd = me->ptEnvoys[i].from_envoy[0];

            FD_SET(fd, &set);

            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        if (max_fd == -1) {
            return NULL;
        }

        if (me->ptEnvoys == NULL) {
            return NULL;
        }

        struct timeval timeout;
        timeout.tv_sec = 1;   // revisa cada segon per no gastar tant la CPU
        timeout.tv_usec = 0;

        int ready = select(max_fd + 1, &set, NULL, NULL, &timeout);

        if (ready > 0) {

            for (int i = 0; i < me->nNumEnvoys; i++) {

                Envoy *envoy = &me->ptEnvoys[i];

                int fd = envoy->from_envoy[0];

                if (fd >= 0 && FD_ISSET(fd, &set)) {

                    Mission result;
                    memset(&result, 0, sizeof(Mission));

                    int ok = read(envoy->from_envoy[0], &result, sizeof(Mission));

                    if (ok > 0) {
                        pthread_mutex_lock(&me->mutex_envoys);
                        check_envoyResponses(me, envoy, result);
                        pthread_mutex_unlock(&me->mutex_envoys);
                    }
                }
            }            
        }
    }

    return NULL;
}


/*********************************************************************
*  
*   @Name: check_envoyResponses
*   @Def: Checks the responses received from Envoys and updates Maester state accordingly.
*   @Arg:   In: maester = pointer to Maester struct,
*               envoy = pointer to Envoy struct that sent the response,
*               result = Mission struct with the response details from the Envoy.
*           Out: -
*   @Ret: -
*
*********************************************************************/

void check_envoyResponses(Maester *maester, Envoy *envoy, Mission result) {

    char *buffer = NULL;
    int bytes = 0;


    switch (result.type) {

        case MISSION_PLEDGE_REQUEST:

            if (result.success) {
                
                pthread_mutex_lock(&maester->mutex_alliances);

                envoy->destination = malloc(strlen(result.destination) + 1);
                if (!envoy->destination) {
                    printg("ERROR: Could not allocate memory.\n");
                    return ;
                }
                strcpy(envoy->destination, result.destination);


                createAlliance(maester, result.destination);
                updateAlliance(maester, result.destination, NULL, -1, ALLIANCE_SENT, time(NULL));

                bytes = asprintf(&buffer, NORMAL "\rPledge sent to %s. Envoy %d is on its way.\n", result.destination, envoy->id);
                checkAndPrint(bytes, &buffer);
                printg(BOLD "$ ");                
                
                pthread_mutex_unlock(&maester->mutex_alliances);


                envoy->isFree = 0;
            }
            else {
                envoy->isFree = 1;
                envoy->mission = MISSION_NONE;
                if (envoy->destination) {
                    free(envoy->destination);
                    envoy->destination = NULL;
                }
            }

            break;
        
        case MISSION_PLEDGE_RESPONSE:


            if (result.success) {

                pthread_mutex_lock(&maester->mutex_alliances);

                if (strcmp(result.data, "1") == 0) {
                    updateAlliance(maester, result.destination, NULL, -1, ALLIANCE_ACCEPTED, NO_TIME);

                    bytes = asprintf(&buffer, NORMAL "\rAlliance with %s established.\n", result.destination);
                    checkAndPrint(bytes, &buffer);
                    printg(BOLD "$ ");
                }
                else {
                    updateAlliance(maester, result.destination, "RESET", 0, ALLIANCE_REJECTED, NO_TIME);

                    bytes= asprintf(&buffer, NORMAL "\rAlliance with %s rejected.\n", result.destination);
                    checkAndPrint(bytes, &buffer);
                    printg(BOLD "$ ");
                }

                pthread_mutex_unlock(&maester->mutex_alliances);
            }

            envoy->isFree = 1;
            envoy->mission = MISSION_NONE;
            if (envoy->destination) {
                free(envoy->destination);
                envoy->destination = NULL;
            }

            break;

        case MISSION_LIST_PRODUCTS_REALM:

            if (result.success) {

                pthread_mutex_lock(&maester->mutex_products);

                char result_data[DATA_MAX_SIZE];
                strcpy(result_data,result.data);

                char *filename = strtok(result_data, "&");
                char *numProductsStr = strtok(NULL, "&");
                int numProducts = atoi(numProductsStr);


                int fd = open(filename, O_RDONLY);
                if (fd < 0) {
                    printg(NORMAL "\rERROR: Could not open product file\n");
                    return;
                }

                bytes = asprintf(&buffer, NORMAL "\rListing products from %s:\n\n", result.destination);
                checkAndPrint(bytes, &buffer);

                Alliance *ally = findAlliance(maester, result.destination);
                if (!ally) {
                    printg("ERROR: Alliance not found.\n");
                    close(fd);
                    return;
                }

                // Free old products if any
                if (ally->products) {
                    free(ally->products);
                    ally->products = NULL;
                }
                // Allocate array for all products
                ally->products = malloc(numProducts * sizeof(Product));
                ally->numProducts = numProducts;


                for (int i = 0; i < ally->numProducts; i++) {

                    Product prod;
                    read(fd, &prod, sizeof(Product));
                    ally->products[i] = prod;


                    bytes = asprintf(&buffer, "%d. %s (%d units)\n", i+1, ally->products[i].sNameProduct, ally->products[i].nAmount);
                    checkAndPrint(bytes, &buffer);
                }
                printg(BOLD "\n$ ");
                close(fd);

                pthread_mutex_unlock(&maester->mutex_products);
            }

            envoy->isFree = 1;
            envoy->mission = MISSION_NONE;
            if (envoy->destination) {
                free(envoy->destination);
                envoy->destination = NULL;
            }

            break;

        case MISSION_START_TRADE:

            if (result.success) {

                pthread_mutex_lock(&maester->mutex_products);

                char result_data[DATA_MAX_SIZE];
                strcpy(result_data,result.data);

                char *filename = strtok(result_data, "&");
                char *numListProductsStr = strtok(NULL, "&");
                int numListProducts = atoi(numListProductsStr);


                int fd = open(filename, O_RDONLY);
                if (fd < 0) {
                    printg(NORMAL "\rERROR: Could not open product file\n");
                    return;
                }


                Alliance *ally = findAlliance(maester, result.destination);

                for (int i = 0; i < numListProducts; i++) {

                    char *line = readLineFile(fd);

                    char *productName = strtok(line, "&");
                    char *amountStr = strtok(NULL, "&");
                    int amount = 0;
                    if (amountStr) {
                        amount = atoi(amountStr);
                    }

                    for (int j = 0; j < ally->numProducts; j++) {

                        if (strcasecmp(productName, ally->products[j].sNameProduct) == 0) {
                            ally->products[j].nAmount -= amount;
                            break;
                        }
                    }

                    free(line);
                }



                bytes = asprintf(&buffer, NORMAL "Trade list sent to %s.\n", result.destination);
                checkAndPrint(bytes, &buffer);
                printg(BOLD "$ ");

                pthread_mutex_unlock(&maester->mutex_products);
            }    

            envoy->isFree = 1;
            envoy->mission = MISSION_NONE;
            if (envoy->destination) {
                free(envoy->destination);
                envoy->destination = NULL;
            }

            break;

        default:
            break;
    }
    
    return;
}


