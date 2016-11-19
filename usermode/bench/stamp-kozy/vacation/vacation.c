/* =============================================================================
 *
 * vacation.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "client.h"
#include "customer.h"
#include "list.h"
#include "manager.h"
#include "map.h"
#include "memory.h"
#include "operation.h"
#include "random.h"
#include "reservation.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "types.h"
#include "utility.h"
#include "pvar.h"

enum param_types {
    PARAM_CLIENTS      = (unsigned char)'c',
    PARAM_NUMBER       = (unsigned char)'n',
    PARAM_QUERIES      = (unsigned char)'q',
    PARAM_RELATIONS    = (unsigned char)'r',
    PARAM_TRANSACTIONS = (unsigned char)'t',
    PARAM_USER         = (unsigned char)'u',
    PARAM_TRACE        = (unsigned char)'e',
};

#define PARAM_DEFAULT_CLIENTS      (1)
#define PARAM_DEFAULT_NUMBER       (10)
#define PARAM_DEFAULT_QUERIES      (90)
#define PARAM_DEFAULT_RELATIONS    ((1<<22) + (1<<20) + (1<<19))
#define PARAM_DEFAULT_TRANSACTIONS (1 << 17)
#define PARAM_DEFAULT_USER         (80)
#define PARAM_DEFAULT_TRACE        (0)

double global_params[256]; /* 256 = ascii limit */
struct timeval v_time;
int init_user = 0;
unsigned long long v_mem_total = 0;
unsigned long long nv_mem_total = 0;
unsigned long v_free = 0, nv_free = 0;

/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    fprintf(OUT, "Usage: %s [options]\n", appName);
    fprintf(OUT,"\nOptions:                                             (defaults)\n");
    fprintf(OUT, "    c <UINT>   Number of [c]lients                   (%i)\n",
           PARAM_DEFAULT_CLIENTS);
    fprintf(OUT, "    n <UINT>   [n]umber of user queries/transaction  (%i)\n",
           PARAM_DEFAULT_NUMBER);
    fprintf(OUT, "    q <UINT>   Percentage of relations [q]ueried     (%i)\n",
           PARAM_DEFAULT_QUERIES);
    fprintf(OUT, "    t <UINT>   Number of [t]ransactions              (%i)\n",
           PARAM_DEFAULT_TRANSACTIONS);
    fprintf(OUT, "    u <UINT>   Percentage of [u]ser transactions     (%i)\n",
           PARAM_DEFAULT_USER);
    fprintf(OUT, "    e <UINT>   Enable trac[e] collection             (%i)\n",
           PARAM_DEFAULT_TRACE);
    exit(1);
}


/* =============================================================================
 * setDefaultParams
 * =============================================================================
 */
static void
setDefaultParams ()
{
    global_params[PARAM_CLIENTS]      = PARAM_DEFAULT_CLIENTS;
    global_params[PARAM_NUMBER]       = PARAM_DEFAULT_NUMBER;
    global_params[PARAM_QUERIES]      = PARAM_DEFAULT_QUERIES;
    global_params[PARAM_RELATIONS]    = PARAM_DEFAULT_RELATIONS;
    global_params[PARAM_TRANSACTIONS] = PARAM_DEFAULT_TRANSACTIONS;
    global_params[PARAM_USER]         = PARAM_DEFAULT_USER;
    global_params[PARAM_TRACE]        = PARAM_DEFAULT_TRACE;
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char* const argv[])
{
    long i;
    long opt;

    opterr = 0;

    setDefaultParams();

    while ((opt = getopt(argc, argv, "c:n:q:r:t:u:e:")) != -1) {
        switch (opt) {
            case 'c':
            case 'n':
            case 'q':
            case 'r':
            case 't':
            case 'u':
            case 'e':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case '?':
            default:
                opterr++;
                break;
        }
    }

    for (i = optind; i < argc; i++) {
        fprintf(OUT, "Non-option argument: %s\n", argv[i]);
        opterr++;
    }

    if (opterr) {
        displayUsage(argv[0]);
    }
}


/* =============================================================================
 * addCustomer
 * -- Wrapper function
 * =============================================================================
 */
TM_ATTR
static bool_t
addCustomer (manager_t* managerPtr, long id, long num, long price)
{
    /* 
	FOR PERSISTENCE, we don't care about seq routines that use 
	TM_MALLOC and TM_FREE
    */
    // return manager_addCustomer_seq(managerPtr, id);
    return manager_addCustomer(managerPtr, id);
}


/* =============================================================================
 * initializeManager
 * =============================================================================
 */
static manager_t*
initializeManager ()
{
    manager_t* managerPtr;
    long i;
    long numRelation;
    random_t* randomPtr;
    long* ids;
    int v_glb_mgr_initialized = 0;
    /*
    FOR PERSISTENCE, we don't care about these sequential routines
    that use malloc and free. We want TM_MALLOC and TM_FREE
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        &manager_addCar_seq,
        &manager_addFlight_seq,
        &manager_addRoom_seq,
        &addCustomer
    };
    */
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        manager_addCar,
        manager_addFlight,
        manager_addRoom,
        addCustomer
    };

    long t;
    long numTable = sizeof(manager_add) / sizeof(manager_add[0]);

    fprintf(OUT, "Initializing manager... \n");
    fflush(stdout);

    randomPtr = random_alloc();
    assert(randomPtr != NULL);

    managerPtr = manager_alloc();
    assert(managerPtr != NULL);

    TM_BEGIN();
	    // Read-only transaction 
	    // Transaction is necessary even though there is only one thread
  	    // Since TM library may choose to implement a redo-log in which
	    // case only the TM library is aware of the location of the latest version
	    v_glb_mgr_initialized = PGET(glb_mgr_initialized);
    TM_END();

    if(v_glb_mgr_initialized)
    {
	fprintf(OUT, "\n***************************************************\n");
        fprintf(OUT, "\nRe-using tables from previous incarnation...\n");
        fprintf(OUT, "\nPersistent table pointers.\n");
        fprintf(OUT, "\n%s = %p\n%s = %p\n%s = %p\n%s = %p\n",					\
						"Car Table     ", managerPtr->carTablePtr, 	\
                                                "Room Table    ", managerPtr->roomTablePtr, 	\
                                                "Flight Table  ", managerPtr->flightTablePtr, 	\
                                                "Customer Table", managerPtr->customerTablePtr);
	fprintf(OUT, "\n***************************************************\n");

        return managerPtr;
    }

    numRelation = (long)global_params[PARAM_RELATIONS];
    ids = (long*)malloc(numRelation * sizeof(long));
    for (i = 0; i < numRelation; i++) {
        ids[i] = i + 1;
    }

    for (t = 0; t < numTable; t++) {

	fprintf(OUT, "POPULATING TABLE NO. %lu\n", t);
        /* Shuffle ids */
        for (i = 0; i < numRelation; i++) {
            long x = random_generate(randomPtr) % numRelation;
            long y = random_generate(randomPtr) % numRelation;
            long tmp = ids[x];
            ids[x] = ids[y];
            ids[y] = tmp;
        }

        /* Populate table */
        for (i = 0; i < numRelation; i++) {
	
	    if(i % 100000 == 0)
	    	fprintf(OUT, "Table no. %lu : Completed %lu tuples\n", t, i);
            bool_t status;
            long id = ids[i];
            long num = ((random_generate(randomPtr) % 5) + 1) * 100;
            long price = ((random_generate(randomPtr) % 5) * 10) + 50;
	    TM_BEGIN();
		/* 
			FOR PERSISTENCE, NOT FOR SYNCHRONIZATION 
			Do we need the overhead of a transaction when
			there is exactly one thread ?
			Yes, for crash consistency !
			But the side-effect is the overhead of locking
			Do we need locking ? Can we disable lock mgmt ?
			This act increases the time to initialize, but only the first time.
		*/
            	status = manager_add[t](managerPtr, id, num, price);
	    TM_END();
            assert(status);
        }

    } /* for t */

    puts("done.");
    fflush(stdout);

    random_free(randomPtr);
    free(ids);

    TM_BEGIN();
	    PSET(glb_mgr_initialized, 1);
    TM_END();
    
    return managerPtr;
}


/* =============================================================================
 * initializeClients
 * =============================================================================
 */
static client_t**
initializeClients (manager_t* managerPtr)
{
    random_t* randomPtr;
    client_t** clients;
    long i;
    long numClient = (long)global_params[PARAM_CLIENTS];
    long numTransaction = (long)global_params[PARAM_TRANSACTIONS];
    long numTransactionPerClient;
    long numQueryPerTransaction = (long)global_params[PARAM_NUMBER];
    long numRelation = (long)global_params[PARAM_RELATIONS];
    long percentQuery = (long)global_params[PARAM_QUERIES];
    long queryRange;
    long percentUser = (long)global_params[PARAM_USER];

    fprintf(OUT, "Initializing clients... ");
    fflush(OUT);

    randomPtr = random_alloc();
    assert(randomPtr != NULL);

    clients = (client_t**)malloc(numClient * sizeof(client_t*));
    assert(clients != NULL);
    numTransactionPerClient = (long)((double)numTransaction / (double)numClient + 0.5);
    queryRange = (long)((double)percentQuery / 100.0 * (double)numRelation + 0.5);

    for (i = 0; i < numClient; i++) {
        clients[i] = client_alloc(i,
                                  managerPtr,
                                  numTransactionPerClient,
                                  numQueryPerTransaction,
                                  queryRange,
                                  percentUser);
        assert(clients[i]  != NULL);
    }

    puts("done.");
    /*
    printf("    Transactions        = %li\n", numTransaction);
    printf("    Clients             = %li\n", numClient);
    printf("    Transactions/client = %li\n", numTransactionPerClient);
    printf("    Queries/transaction = %li\n", numQueryPerTransaction);
    printf("    Relations           = %li\n", numRelation);
    printf("    Query percent       = %li\n", percentQuery);
    printf("    Query range         = %li\n", queryRange);
    printf("    Percent user        = %li\n", percentUser);
    */
    fflush(stdout);

    random_free(randomPtr);

    return clients;
}


/* =============================================================================
 * checkTables
 * -- some simple checks (not comprehensive)
 * -- dependent on tasks generated for clients in initializeClients()
 * =============================================================================
 */
void
checkTables (manager_t* managerPtr)
{
    long i;
    long numRelation = (long)global_params[PARAM_RELATIONS];
    MAP_T* customerTablePtr = managerPtr->customerTablePtr;
    MAP_T* tables[] = {
        managerPtr->carTablePtr,
        managerPtr->flightTablePtr,
        managerPtr->roomTablePtr,
    };
    long numTable = sizeof(tables) / sizeof(tables[0]);
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        &manager_addCar_seq,
        &manager_addFlight_seq,
        &manager_addRoom_seq
    };
    long t;

    printf("Checking tables... ");
    fflush(stdout);

    /* Check for unique customer IDs */
    long percentQuery = (long)global_params[PARAM_QUERIES];
    long queryRange = (long)((double)percentQuery / 100.0 * (double)numRelation + 0.5);
    long maxCustomerId = queryRange + 1;
    for (i = 1; i <= maxCustomerId; i++) {
        if (MAP_FIND(customerTablePtr, i)) {
            if (MAP_REMOVE(customerTablePtr, i)) {
                assert(!MAP_FIND(customerTablePtr, i));
            }
        }
    }

    /* Check reservation tables for consistency and unique ids */
    for (t = 0; t < numTable; t++) {
        MAP_T* tablePtr = tables[t];
        for (i = 1; i <= numRelation; i++) {
            if (MAP_FIND(tablePtr, i)) {
                assert(manager_add[t](managerPtr, i, 0, 0)); /* validate entry */
                if (MAP_REMOVE(tablePtr, i)) {
                    assert(!MAP_REMOVE(tablePtr, i));
                }
            }
        }
    }

    puts("done.");
    fflush(stdout);
}


/* =============================================================================
 * freeClients
 * =============================================================================
 */
static void
freeClients (client_t** clients)
{
    long i;
    long numClient = (long)global_params[PARAM_CLIENTS];

    for (i = 0; i < numClient; i++) {
        client_t* clientPtr = clients[i];
        client_free(clientPtr);
    }
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    manager_t* managerPtr;
    client_t** clients;
    TIMER_T start;
    TIMER_T stop;

    GOTO_REAL();

    /* Initialization */
    parseArgs(argc, argv);
    SIM_GET_NUM_CPU(global_params[PARAM_CLIENTS]);

    long numClient = (long)global_params[PARAM_CLIENTS];
    long numTransaction = (long)global_params[PARAM_TRANSACTIONS];
    long numTransactionPerClient = 0;
    long numQueryPerTransaction = (long)global_params[PARAM_NUMBER];
    long numRelation = (long)global_params[PARAM_RELATIONS];
    long percentQuery = (long)global_params[PARAM_QUERIES];
    long queryRange;
    long percentUser = (long)global_params[PARAM_USER];
    long enableTrace = (long)global_params[PARAM_TRACE];
    if(numClient != 0)
	    numTransactionPerClient = (long)((double)numTransaction / (double)numClient + 0.5);
    else
    {
	    numTransaction = 0;
	    init_user = 1;
    }
    queryRange = (long)((double)percentQuery / 100.0 * (double)numRelation + 0.5);
    mtm_enable_trace = (int)enableTrace;

    #if defined(MAP_USE_HASHTABLE)
    fprintf(OUT, "    Table Index         = HASHTABLE\n");
    #elif defined(MAP_USE_RBTREE)
    fprintf(OUT, "    Table Index         = RBTREE\n");
    #endif
    fprintf(OUT, "    Transactions        = %li\n", numTransaction);
    fprintf(OUT, "    Clients             = %li\n", numClient);
    fprintf(OUT, "    Transactions/client = %li\n", numTransactionPerClient);
    fprintf(OUT, "    Queries/transaction = %li\n", numQueryPerTransaction);
    fprintf(OUT, "    Relations           = %li\n", numRelation);
    fprintf(OUT, "    Query percent       = %li\n", percentQuery);
    fprintf(OUT, "    Query range         = %li\n", queryRange);
    fprintf(OUT, "    Percent user        = %li\n", percentUser);
    fprintf(OUT, "    Enable trace        = %li\n", enableTrace);
 
    managerPtr = initializeManager();
    assert(managerPtr != NULL);
    if(init_user == 1)
    {
	fprintf(OUT, "\nInit-phase\n");
	fprintf(OUT, "Total volatile memory consumption     = %llu bytes\n", v_mem_total);
	fprintf(OUT, "Total non-volatile memory consumption = %llu bytes\n", nv_mem_total);
	fprintf(OUT, "Total volatile memory free()'s        = %lu\n", v_free);
	fprintf(OUT, "Total non-volatile memory free()'s    = %lu\n", nv_free);
	MAIN_RETURN(0);
    }

    clients = initializeClients(managerPtr);
    assert(clients != NULL);
    long numThread = global_params[PARAM_CLIENTS];
    TM_STARTUP(numThread);
    P_MEMORY_STARTUP(numThread);
    thread_startup(numThread);

    /* Run transactions */
    fprintf(OUT, "\nRunning clients...\n\n");
    fflush(OUT);
    TIMER_READ(start);
    GOTO_SIM();
#ifdef OTM
#pragma omp parallel
    {
        client_run(clients);
    }
#else
    thread_start(client_run, (void*)clients);
#endif
    GOTO_REAL();
    TIMER_READ(stop);
    // mtm_enable_trace = (int)0;
    fprintf(OUT, "done.");
    fprintf(OUT, "Time = %0.6lf\n",
           TIMER_DIFF_SECONDS(start, stop));
    fflush(OUT);
#if 0
    /* FOR PERSISTENCE, don't remove entries from table */
    checkTables(managerPtr);
#endif

    /* Clean up */
    fprintf(OUT, "Deallocating memory... ");
    fflush(OUT);
    freeClients(clients);
#if 0
    /* FOR PERSISTENCE, don't clean-up */
    /*
     * TODO: The contents of the manager's table need to be deallocated.
     */
    manager_free(managerPtr);
#endif
    fprintf(OUT, "done.");
    fflush(OUT);
    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(0);
}


/* =============================================================================
 *
 * End of vacation.c
 *
 * =============================================================================
 */
