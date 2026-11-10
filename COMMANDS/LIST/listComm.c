/************************************************
*
* @File : listComm.c
* @Purpose : Is in charge of running all list commands the user might imput, and do its proper operations for each one.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "listComm.h"

/*********************************************************************
*  
*   @Name: listProducts
*   @Def: Displays the option of List Products in the screen for the user to see.
*   @Arg:   In: -
*           Out: ptProducts = double pointer to typedef structure of Products, pnNumProducts = pointer to number of products.
*   @Ret: -
*
*********************************************************************/

void listProducts (Product *ptProducts, int *pnNumProducts) {
    char *psBuffer    = NULL;
    int nBytes     = 0;
    int i   = 0;

    printg("--- Trade Ledger ---\n");
    printg("Item                    | Value (Gold)  | Weight (Stone)\n");
    printg("--------------------------------------------------------\n");

    // Loop through the product array to display formatted stock information
    for (i = 0; i < (*pnNumProducts); i++) {
        // Using numbers in asprinf to ensure the bars | are aligned.
        nBytes = asprintf(&psBuffer, "%-23s | %-13d | %-13.1f\n",ptProducts[i].sNameProduct, ptProducts[i].nAmount, ptProducts[i].fWeight);
        checkAndPrint(nBytes, &psBuffer);
    }
    printg("--------------------------------------------------------\n");
    
    nBytes = asprintf(&psBuffer, "Total Entries: %d\n\n", *pnNumProducts);
    checkAndPrint(nBytes, &psBuffer);
}


/*********************************************************************
*  
*   @Name: listRealms
*   @Def: Displays the option of List Realms in the screen for the user to see.
*   @Arg:   In: -
*           Out: ptMaester = pointer to typedef strucutre of Maester.
*   @Ret: -
*
*********************************************************************/

void listRealms(Maester *ptMaester) {
    char *psBuffer    = NULL;
    int nBytes    = 0;
    int i   = 0;

    //Skip the own name (DEFAULT) and iterate through the known routes
    for (i = 1; i < ptMaester->nNumRoutes; i++) { 
        if (strcmp(ptMaester->ptRoutes[i].psNameRoute, ptMaester->psNameRealm)) {
            nBytes = asprintf(&psBuffer, "- %s\n", ptMaester->ptRoutes[i].psNameRoute);
            checkAndPrint(nBytes, &psBuffer);
        }
    }
}


/*********************************************************************
*  
*   @Name: listProductsRealm
*   @Def: Displays the option of List Products of a realm in the screen for the user to see, if the realm is an ally, otherwise shows an error message.
*   @Arg:   In: -
*           Out: ptMaester = pointer to typedef structure of Maester, 
                destination = name of the realm to list products from.
*   @Ret: -
*
*********************************************************************/

void listProductsRealm(Maester *ptMaester, char *destination) {

    char *buffer = NULL;
    int nBytes = 0;

    Alliance *ally = findAlliance(ptMaester, destination);
    if (!ally || ally->tStatus != ALLIANCE_ACCEPTED) {
        nBytes = asprintf(&buffer, "ERROR: Could not list %s's products, not an ally.\n", destination);
        checkAndPrint(nBytes, &buffer);
        return;
    }

    char *productListFile = NULL;
    asprintf(&productListFile, "REALMS%s/%s_stock.db", ptMaester->psPathFolder, ally->sNameRealm);

    int fd = open(productListFile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        printg("ERROR: Could not create new stock file.\n");
        free(productListFile);
        return;
    }


    Mission mission;
    memset(&mission, 0, sizeof(Mission));

    mission.type = MISSION_LIST_PRODUCTS_REALM;
    strcpy(mission.origin, ptMaester->psNameRealm);
    strcpy(mission.originIP, ptMaester->psIP);
    mission.originPort = ptMaester->nPort;

    strcpy(mission.destination, destination);
    strcpy(mission.destinationIP, ally->sIp);
    mission.destinationPort = ally->nPort;

    strcpy(mission.data, productListFile);

    assignMission(ptMaester, mission);

    /*
    ////////////////

    int list_socket = connect_to_maester(ally->sIp, ally->nPort);
    if (list_socket < 0) {
        return;
    }

    Frame f;
    char *data = NULL;
    asprintf(&data, "%s", ptMaester->psNameRealm);
    char *origin = NULL;
    asprintf(&origin, "%s:%d", ptMaester->psIP, ptMaester->nPort);    
    build_frame(&f, TYPE_PROD_LIST_REQ, origin, destination, (uint16_t)strlen(data), data);
    send_frame(list_socket, &f);

    Frame resp12;
    if (get_frame(list_socket, &resp12) < 0 || resp12.type != TYPE_PROD_LIST_RESP) {
        printg("ERROR: Did not receive product list header.\n");
        close(list_socket);
        return;
    }

    // Parse the 0x12 header to get total products
    uint16_t hdrLen = ntohs(resp12.data_len);
    char *hdr = malloc(hdrLen + 1);
    if (!hdr) {
        close(list_socket);
        return;
    }
    
    memcpy(hdr, resp12.data, hdrLen);
    hdr[hdrLen] = '\0';

    strtok(hdr, "&");
    char *fileSize = strtok(NULL, "&"); 
    char *md5 = strtok(NULL, "&");

    int fileSizeInt  = atoi(fileSize);
    int totalProducts = fileSizeInt / sizeof(Product);
    char *md5_saved  = strdup(md5);
    

    send_ack(list_socket, ptMaester);


    
    ////////////        aixo

    // Free old products if any
    if (ally->products) {
        free(ally->products);
        ally->products = NULL;
    }
    // Allocate array for all products
    ally->products = malloc(totalProducts * sizeof(Product));
    ally->numProducts = totalProducts;


    // Receive 0x13 chunks until no more frames, load it into allies
    char *psBuffer = NULL;
    int n = 0;

    Frame chunk;
    int fd = open("received_stock.jpg", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        printg("ERROR: Could not create new stock file.\n");
        free(md5_saved);
        free(hdr);
        free(origin);
        free(data);
        close(list_socket);
        return;
    }


    while (n < totalProducts) {
        if (get_frame(list_socket, &chunk) < 0 || chunk.type != TYPE_PROD_LIST_DATA) {
            break;
        }

        Product prod;
        uint16_t chunk_len = ntohs(chunk.data_len);
        memcpy(&prod, chunk.data, chunk_len);

        // Store in ally
        ally->products[n] = prod;

        write(fd, &prod, sizeof(Product));

        n++;
    }

    close(fd);

    //send md5
    Frame ack_md;
    char *supposed_md5 = calculate_md5sum("received_stock.jpg");
    free(data);
    data = NULL;

    if (strcmp(supposed_md5, md5_saved) == 0) {
        asprintf(&data, "CHECK_OK&%s", ptMaester->psNameRealm);
        int i = 0;
        for (i = 0; i < ally->numProducts; i++) {
            nBytes = asprintf(&psBuffer, "%d. %s (%d units)\n", i+1, ally->products[i].sNameProduct, ally->products[i].nAmount);
            checkAndPrint(nBytes, &psBuffer);
        }
    }
    else {
        asprintf(&data, "CHECK_KO&%s", ptMaester->psNameRealm);
        //aqui vol dir que la data en realitat s'ha enviat malament, per tant hem de fer free de tots els productes de l'aliat
        free(ally->products);
        ally->products = NULL;
        ally->numProducts = 0;
    }
    free(supposed_md5);

    build_frame(&ack_md, TYPE_ACK_MD5, origin, destination, (uint16_t)strlen(data), data);
    send_frame(list_socket, &ack_md);
    printg("\n");


    free(md5_saved);
    free(data);
    free(origin);
    free(hdr);
    close(list_socket);

    ////////////////////
    */

    free(productListFile);

    return;
}
