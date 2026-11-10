/************************************************
*
* @File : main
* @Purpose : Control the whole System, initialize it by getting commands from command line.
* @Author : Sara Gibert & Oriol Pros
* @Date : 17/05/2026
*
***********************************************/

// Own inclusions
#include "utils.h"
#include "CONFIG/localConfig.h"
#include "CONFIG/networkConfig.h"
#include "CONFIG/envoyConfig.h"


/*********************************************************************
*  
*   @Name: manageCtrlC
*   @Def: Handles the Control+C signal, using close(0), which will unable the program to read more, 
*         leading it back to the main menu, which follows by freeing all the structs.
*   @Arg:   In: nSig = number of signal of Control+C.
*           Out: -
*   @Ret: -
*
*********************************************************************/

void manageCtrlC(int nSig) {
    (void)nSig; 
    // 0 is STDIN_FILENO.
    close(0);   
}

/*********************************************************************
*  
*   @Name: main
*   @Def: Main function of the Citadel System, only initializes the system.
*   @Arg:   In: argc = number of arguments in command line, argv = array of strings which are the words in command line.
*           Out: -
*   @Ret: Since it's the main procedure, will always return 0.
*
*********************************************************************/

int main(int argc, char *argv[]) {
    Maester tMaester;
    Product *ptProducts = NULL;
    int nNumProducts    = 0;
    int nReadMaester    = 0;
    int nReadStock      = 0;
    int nEnvoysCreated  = 0;
    int listen_fd = -1;

    // Verify correct number of command line arguments
    if (3 != argc) { 
        printg("Usage: ./Maester <maester.dat> <stock.db> \n");
        return 1;
    }

    // Register signal handler for graceful interruption
    signal(SIGINT, manageCtrlC);

    // Load configuration and product data from files, and if no error, start system.
    nReadMaester = readMasterFile(argv[1], &tMaester);
    tMaester.stockName = strdup(argv[2]);
    nReadStock = readStockFile(argv[2], &ptProducts, &nNumProducts);

    nEnvoysCreated = createEnvoys(&tMaester);

    if (nReadMaester && nReadStock && nEnvoysCreated) {
        
        listen_fd = start_listen(tMaester.psIP, tMaester.nPort); //ho poso aqui i no dins del thread perque si aixo falla, el main ha d acabar, a lo ha de ser server si o si, 
        //i si falla al thread, el main no sabra que ha de acabar, i es quedara penjat esperant a que el thread acabi, pero el thread no pot acabar perque no pot escoltar, 
        //i es quedara penjat per sempre

        if (listen_fd < 0) {
            freeMaester(&tMaester);
            freeProduct(&ptProducts);
            return 1;
        } 

        ServerComm args;

        args.listen_fd = listen_fd;
        args.maester = &tMaester;
        args.products = ptProducts;
        args.numProducts = nNumProducts;

        args.exit = 0;
        pthread_mutex_init(&args.mutex_exit, NULL);

        pthread_mutex_init(&tMaester.mutex_alliances, NULL);
        pthread_mutex_init(&tMaester.mutex_products, NULL);
        pthread_mutex_init(&tMaester.mutex_envoys, NULL);
    
        pthread_t networkListener;
        pthread_t envoyListener;

        pthread_create(&networkListener, NULL, network_thread, &args);
        pthread_create(&envoyListener, NULL, envoy_thread, &args);

        startCitadelSystem(&tMaester, ptProducts, &nNumProducts);

        maesterDisconnection(&tMaester);
        
        pthread_mutex_lock(&args.mutex_exit);
        args.exit = 1;
        pthread_mutex_unlock(&args.mutex_exit);

        close(listen_fd);

        pthread_join(envoyListener, NULL); 
        pthread_join(networkListener, NULL); 
        

        pthread_mutex_destroy(&args.mutex_exit);
        pthread_mutex_destroy(&tMaester.mutex_alliances);
        pthread_mutex_destroy(&tMaester.mutex_products);
        pthread_mutex_destroy(&tMaester.mutex_envoys);
    }

    // Clean up dynamic memory before exiting
    freeMaester(&tMaester);
    freeProduct(&ptProducts);
    
    return 0;
}