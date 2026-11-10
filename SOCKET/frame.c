/************************************************
*
* @File : frame.c
* @Purpose : Define the structures and function prototypes needed to handle frames in the network communication.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

#include "frame.h"

/*********************************************************************
*  
*   @Name: serialize_frame
*   @Def: Serializes a Frame struct into a byte buffer for sending over the network, following the defined frame structure and field offsets.
*   @Arg:   In: -
*           Out: f: pointer to the frame with the checksum field set to 0 for calculation, buffer: pointer to a byte array of size FRAME_SIZE where the serialized frame will be stored.
*   @Ret: -
*
*********************************************************************/

void serialize_frame(Frame *f, uint8_t *buffer) {
    // Frame struct → buffer de 320 bytes per enviar

    memset(buffer, 0, FRAME_SIZE);
    //offset 0,  1 byte
    buffer[0] = f->type;

    //offset 1, 20 bytes
    memcpy(buffer + 1, f->origin, ORIGIN_SIZE);

    //offset 21, 20 bytes
    memcpy(buffer + 21, f->dest, DEST_SIZE);

    //offset 41, 2 bytes
    uint16_t data_length = htons(f->data_len);
    memcpy(buffer + 41, &data_length, 2);

    //offset 43, 275 bytes
    memcpy(buffer + 43,  f->data, DATA_MAX_SIZE);

    //offset 318, 2 bytes
    uint16_t checksum = htons(f->checksum);
    memcpy(buffer + 318, &checksum, 2);
}

/*********************************************************************
*  
*   @Name: deserialize_frame
*   @Def: Deserializes a byte buffer into a Frame struct, extracting the fields
*   @Arg:   In: -
*           Out: f: pointer to the frame with the checksum field set to 0 for calculation, buffer: pointer to a byte array of size FRAME_SIZE where the serialized frame will be stored.
*                buffer: pointer to a byte array of size FRAME_SIZE containing the serialized frame data received from the network.
*   @Ret: -
*
*********************************************************************/

void deserialize_frame(Frame *f, uint8_t *buffer) {
    // Buffer de 320 bytes rebut → Frame struct

    memset(f, 0, sizeof(Frame));

    f->type = buffer[0];

    memcpy(f->origin, buffer + 1,   ORIGIN_SIZE);
    
    memcpy(f->dest,   buffer + 21,  DEST_SIZE);

    uint16_t data_length;
    memcpy(&data_length, buffer + 41, 2);
    f->data_len = ntohs(data_length);

    memcpy(f->data, buffer + 43, DATA_MAX_SIZE);

    uint16_t checksum;
    memcpy(&checksum, buffer + 318, 2);
    f->checksum = ntohs(checksum);
}

/*********************************************************************
*  
*   @Name: calculate_checksum
*   @Def: Calculates the checksum for a given frame
*   @Arg:   In: -
*           Out: f: pointer to the frame with the checksum field set to 0 for calculation
*   @Ret: The calculated checksum value as a 16-bit unsigned integer.
*
*********************************************************************/

uint16_t calculate_checksum(Frame *f) {

    uint8_t buffer[FRAME_SIZE];
    serialize_frame(f, buffer);

    // Posem checksum a 0 per al càlcul
    buffer[318] = 0;
    buffer[319] = 0;

    uint32_t sum = 0;

    for (int i = 0; i < FRAME_SIZE - 2; i++) {
        sum += buffer[i];
    }

    return (uint16_t)(sum % 65536);
}


/*********************************************************************
*  
*   @Name: validate_checksum
*   @Def: Validates the checksum of a given frame
*   @Arg:   In: -
*           Out: f: pointer to the frame to validate, with the checksum field containing the received checksum value
*   @Ret: 1 if the checksum is valid, 0 otherwise
*
*********************************************************************/

int validate_checksum(Frame *f) {

    uint16_t received = htons(f->checksum);

    uint16_t calculated = calculate_checksum(f);


    if (received == calculated) {
        return 1;
    } 

    printg(NORMAL "\rERROR: Checksum mismatch.\n");
    return 0;
}

/*********************************************************************
*  
*   @Name: build_frame
*   @Def: Builds a frame with the given parameters, including calculating and setting the checksum.
*   @Arg:   In: type: the type of the frame (e.g., alliance request, product list request, etc.), data_len: the length of the data payload.
*           Out: f: pointer to the frame to be built, with all fields set according to the provided parameters and the checksum calculated, 
                origin: the origin IP and port in the format "ip:port", 
                destination: the destination realm name, 
                data: pointer to the data payload to be included in the frame.
*   @Ret: -
*
*********************************************************************/

void build_frame(Frame *f, uint8_t type, char *origin, char *destination, uint16_t data_len, char *data) {
    memset(f, 0, FRAME_SIZE);  // padding zeros
    
    f->type = type;
    
    if (origin != NULL) {
        strncpy(f->origin, origin, ORIGIN_SIZE - 1);
        f->origin[ORIGIN_SIZE - 1] = '\0';
    }

    if (destination != NULL) {
        strncpy(f->dest, destination, DEST_SIZE   - 1);
        f->dest[DEST_SIZE - 1] = '\0';
    }


    if (data_len > DATA_MAX_SIZE) {
        data_len = DATA_MAX_SIZE;
    }
    
    f->data_len = htons(data_len);


    if (data && data_len > 0) {
        memcpy(f->data, data, data_len); 
    }
    
    f->checksum = 0;

    uint16_t checksum = calculate_checksum(f);
    f->checksum = htons(checksum); 
}


/*********************************************************************
*  
*   @Name: send_frame
*   @Def: Sends a frame over a socket, ensuring that all 320 bytes of the frame are sent even if the socket write call sends fewer bytes in one go.
*   @Arg:   In: sockfd: the file descriptor of the socket to send the frame through
*           Out: f: pointer to the frame to be sent, which should already have all fields set and the checksum calculated.
*   @Ret: 0 if the frame is sent successfully, -1 otherwise.
*
*********************************************************************/

int send_frame(int sockfd, Frame *f) {

    uint8_t buffer[FRAME_SIZE];
    int total = 0; 

    serialize_frame(f, buffer);              

    while (total < FRAME_SIZE) {
        int n = write(sockfd, buffer + total, FRAME_SIZE - total);
        if (n <= 0) {  
            printg("ERROR: Writing error when sending frame.\n");
            return -1;
        }
        total += n; 
    }

    return 0; 
}


/*********************************************************************
*  
*   @Name: get_frame
*   @Def: Receives a frame from a socket, ensuring that all 320 bytes of the frame are received even if the socket read call receives fewer bytes in one go.
*   @Arg:   In: sockfd: the file descriptor of the socket to receive the frame from
*           Out: f: pointer to the frame to be received, which will have all fields set according to the received data and the checksum validated.
*   @Ret: 0 if the frame is received successfully, -1 otherwise.
*
*********************************************************************/

int get_frame(int sockfd, Frame *f) {
    
    uint8_t buffer[FRAME_SIZE];
    int total = 0;
    int n;

    memset(f, 0, FRAME_SIZE);

    while (total < FRAME_SIZE) {
        n = read(sockfd, buffer + total, FRAME_SIZE - total);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }

    deserialize_frame(f, buffer);

    return 0; 
}


/*********************************************************************
*  
*   @Name: send_nack
*   @Def: Sends a negative acknowledgment frame to the sender of a received frame.
*   @Arg:   In: f: pointer to the received frame
*           Out: me: pointer to the Maester struct
*   @Ret: -
*
*********************************************************************/

void send_nack(int sockfd, Maester *me) {

    char *data = NULL;
    asprintf(&data, "KO&%s", me->psNameRealm);

    Frame nack;
    build_frame(&nack, TYPE_NACK, "", "", (uint16_t)strlen(data), data);
    send_frame(sockfd, &nack);

    free(data);
}


/*********************************************************************
*  
*   @Name: send_nack_file
*   @Def: Sends a negative acknowledgment frame specifically for file transfer failures, including the realm name in the data payload.
*   @Arg:   In: -
*           Out: f: pointer to the received frame
*           me: pointer to the Maester struct
*   @Ret: -
*
*********************************************************************/

void send_nack_file(Frame *f, Maester *me) {
    char *data = NULL;
    asprintf(&data, "%s", me->psNameRealm);

    Frame ack;
    build_frame(&ack, TYPE_NACK, "", "", (uint16_t)strlen(data), data);

    char *buffer = f->origin;

    if (buffer[0] == '\0') {
        free(data);
        return;
    }

    int index = -1;
    for (int i = 0; i < 20; i++) {
        if (buffer[i] == ':') { 
            index = i; 
            break; 
        }
    }

    if (index < 0) {
        free(data);
        return;
    }

    char *originIP = malloc(index + 1);
    if (!originIP) { 
        free(data); 
        return; 
    }
    memcpy(originIP, buffer, index);
    originIP[index] = '\0';

    int start = index + 1;
    int end = start;

    while (end < 20 && buffer[end] != '\0' && buffer[end] != ' ') {
        end++;
    }

    int port_len = end - start;
    if (port_len <= 0) { 
        free(originIP); 
        free(data); 
        return;
    }

    char *originPort = malloc(port_len + 1);
    if (!originPort) { 
        free(originIP); 
        free(data); 
        return; 
    }
    memcpy(originPort, buffer + start, port_len);
    originPort[port_len] = '\0';
        
    char *originName = NULL;
        
    for (int i = 0; i < me->nNumRoutes; i++) {
        if (strcmp(me->ptRoutes[i].psIP, originIP) == 0 && me->ptRoutes[i].nPort == atoi(originPort)) {
            originName = me->ptRoutes[i].psNameRoute;
        }
    }

    Route *r = findRoute(me, originName);
    if (!r) {
        free(data);
        return;
    }

    int sock = connect_to_maester(r->psIP, r->nPort);
    if (sock < 0) {
        free(data);
        return;
    }

    send_frame(sock, &ack);
    close(sock);
    free(originPort);
    free(originIP);
    free(data);
}

/*********************************************************************
*  
*   @Name: send_ack_frame
*   @Def: Sends an acknowledgment frame to the sender of a received frame.
*   @Arg:   In: f: pointer to the received frame
*           Out: me: pointer to the Maester struct
*   @Ret: -
*
*********************************************************************/

void send_ack_frame(Frame *f, Maester *me) {

    char *data = NULL;
    asprintf(&data, "OK&%s", me->psNameRealm);

    Frame ack;
    build_frame(&ack, TYPE_ACK_FILE, "", "", (uint16_t)strlen(data), data);

    char *buffer = f->origin;
    int index = -1;
    char *IP = NULL;
    char *port = NULL;

    for (int i = 0; i < 20; i++) {
        if (buffer[i] == ':') {
            index = i;
            break;
        }
    }

    IP = malloc(index + 1);
    if (!IP) {
        return;
    }
    memcpy(IP, buffer, index);
    IP[index] = '\0';      

    int start = index + 1;
    int end = start;

    // 20 because IP size always
    while (end < 20 && buffer[end] != '\0' && buffer[end] != ' ') {
        end++;
    }
        
    int port_len = end - start;

    port = malloc(port_len + 1);
    if (!port) {
        free(port);
        return;
    }
    
    memcpy(port, buffer + start, port_len);
    port[port_len] = '\0';
    

    char *originName = NULL;
    
    for (int i = 0; i < me->nNumRoutes; i++) {
        if (strcmp(me->ptRoutes[i].psIP, IP) == 0 && me->ptRoutes[i].nPort == atoi(port)) {
            originName = me->ptRoutes[i].psNameRoute;
        }
    }
    

    Route *r = findRoute(me, originName);
    if (!r) {
        free(data);
        return;
    }

    int sock = connect_to_maester(r->psIP, r->nPort);
    if (sock < 0) {
        free(data);
        return;
    }

    send_frame(sock, &ack);
    close(sock);
    free(IP);
    free(port);
    free(data);

}


/*********************************************************************
*  
*   @Name: send_ack
*   @Def: Sends an acknowledgment frame to the sender of a received frame.
*   @Arg:   In: sockfd: socket file descriptor
*           Out: me: pointer to the Maester struct
*   @Ret: -
*
*********************************************************************/

void send_ack(int sockfd, Maester *me) {

    char *data = NULL;
    asprintf(&data, "OK&%s", me->psNameRealm);

    Frame ack;
    build_frame(&ack, TYPE_ACK_FILE, "", "", (uint16_t)strlen(data), data);
    send_frame(sockfd, &ack);

    free(data);
}


/*********************************************************************
*  
*   @Name: calculate_md5sum
*   @Def: Calculates the MD5 sum of a file.
*   @Arg:   In: pathSigil: path to the file for which to calculate the MD5 sum
*           Out: -
*   @Ret: Pointer to the calculated MD5 sum string, or NULL if an error occurs
*
*********************************************************************/

char *calculate_md5sum(char *pathSigil) {
    int fd[2];
    if (pipe(fd) == -1) {
        printg("ERROR: Could not create pipe.\n");
        return NULL;
    }

    int child_md5 = fork();

    if (child_md5 == -1) {
        printg("ERROR: Could not calculate MD5SUM due to a fork problem.\n");
        return NULL;

    } 
    else if (child_md5 == 0) {
        close(fd[0]); 
        dup2(fd[1], 1); 
        close(fd[1]); 

        /*
        utilitzar a matagalls:  /usr/bin/md5sum
        utilitzar a macOS:      /sbin/md5sum 
        */
		char *args[] = {"/usr/bin/md5sum", pathSigil, NULL};
		
        if (execv("/usr/bin/md5sum", args) == -1) {
            printg("ERROR: Execv failed.\n");
			return NULL;
		}

    } 
    
    else {
        close(fd[1]); 

        wait(NULL);

        int i = 0;
        char ch;

        char *md5sum = NULL;
        md5sum = malloc(sizeof(int) * 33); 
        if (!md5sum) {
            close(fd[0]);

            return NULL;
        } 
        
        while (i < 32) {
            int r = read(fd[0], &ch, 1);

            if (r <= 0) {
                printg("\rERROR: Failed to read MD5SUM output.\n");
                printg(BOLD "$ ");
                free(md5sum);
                close(fd[0]);
                return NULL;
            }

            md5sum[i++] = ch;
        }

        md5sum[32] = '\0';

        close(fd[0]);

        return md5sum;
    }

    return NULL;
}