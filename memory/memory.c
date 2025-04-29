#include <stdlib.h>
#include<getopt.h>
#include <memory.h>
#include <interconnect.h>
#include <stdio.h>

#include "memory_internal.h"
#include "stree.h"

void registerInterconnect(interconn* interconnect);
int busReq(uint64_t addr, int procNum, void (*callback)(int, uint64_t));
directory_action reqDirectory(uint64_t addr, int procNum, int isRead);
void updateDirectory(uint64_t addr, int procNum);
int removeProc(uint64_t addr);
int getProcForRequest(uint64_t addr);

memory* self = NULL;
memReq* pendingRequest = NULL;
tree_t *directory = NULL;
interconn* interComp;
int useDirectory = 0;
int countDown = 0;
int verbose_mode = 0;

// This is the same as "BUS_TIME".
const int DRAM_FETCH_TICKS = 90;


memory* init(memory_sim_args* args)
{
    int op;
    int d;
    optind = 0;
    while ((op = getopt(args->arg_count, args->arg_list, "dv")) != -1)
    {
        switch (op)
        {
            case 'd':
                printf("using directory\n");
                useDirectory = 1;
                break;
            case 'v':
                printf("verbose mode on\n");
                verbose_mode = 1;
                break;
        }
    }

    directory = tree_new();
 
    
    self = calloc(1, sizeof(memory));
    assert(self);
 

    self->registerInterconnect = registerInterconnect;
    self->busReq = busReq;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;
    pendingRequest = NULL;
    if (useDirectory) {
        self->reqDirectory = reqDirectory;
        self->updateDirectory = updateDirectory;
        self->removeProc = removeProc;
        self->getProcForRequest = getProcForRequest;
    }
    return self;
}

void registerInterconnect(interconn* interconnect)
{
    assert(self);
    assert(interconnect);

    interComp = interconnect;
}

/** 
 * @brief Helper function to print out the directory
 * @param node Node inside of directory tree
 * 
 */
void print_directory_node(node_t *node) {
    if (node == NULL) {
        return;
    }

    directoryEntry *entry = (directoryEntry *)(node->record);
    if (entry->procs != NULL) { // only print out nonempty entries
        printf("|---------------- Entry for addr %lx ----------------|\n", node->key);
        printf("|Dirty bit: %d                                         |\n", entry->dirty_bit);
        printf("|Currently held by procs : ");
        int lineLen = 25;
        procEntry *curr = entry->procs;
        while (curr->next) {
            printf("%d, ", curr->proc);
            curr = curr->next;
            lineLen -= 3;
        }
        printf("%d", curr->proc);
        while (lineLen-- > 0) {
            printf(" ");
        }

        printf("|\n");
    }
    print_directory_node(node->left);
    print_directory_node(node->right);
}

/** 
 * @brief Prints out the directory, will run upon insertion if -h flag is passed into 
 */

void print_directory() {
    printf("\n\n\n______________________ DIRECTORY ______________________\n");
    print_directory_node(directory->root);
    printf("_______________________________________________________\n\n\n");
}

/** 
 * @brief inserts a processor into the front of a directory entry
 * @note inserts to front of LL so that it'll be queried next if another processer reads
 */
void insert_proc(directoryEntry *entry, int procNum) {
    procEntry *curr = calloc(1, sizeof(procEntry));
    curr->proc = procNum;
    curr->next = entry->procs;
    entry->procs = curr;
    return;
} 

/**
 * @brief removes a processor from LL, called from interconnect
 * @returns the processor that was removed
 */
int removeProc(uint64_t addr) {
    directoryEntry *entry = (directoryEntry *)tree_find(directory, addr);
    assert(entry);
    if (entry->procs == NULL) {
        return -1;
    }

    int res = entry->procs->proc;
    procEntry *temp = entry->procs;
    entry->procs = entry->procs->next;
    free(temp);
    return res;
}

/**
 * @brief Returns the processor that last queried a line
 */
int getProcForRequest(uint64_t addr) {
    directoryEntry *entry = (directoryEntry *)tree_find(directory, addr);

    assert(entry);
    assert(entry->procs);
    procEntry *curr = entry->procs;
    while (curr->next) {
        curr = curr->next;
    }
    return curr->proc;
}

/**
 * @brief updates directory once a request has been serviced
 */
void updateDirectory(uint64_t addr, int procNum) {
    directoryEntry *entry = (directoryEntry *)tree_find(directory, addr);
    assert(entry);
    insert_proc(entry, procNum);

    tree_insert(directory, addr, (void *)entry);
    if (verbose_mode) print_directory();
}

/**
 * @brief Queury directory for action to perform for a given bus request
 * @returns action to be performed, names are self explanatory
 */
directory_action 
reqDirectory(uint64_t addr, int procNum, int isRead) {

    directoryEntry *entry = (directoryEntry *)tree_find(directory, addr);
    
    if (entry == NULL) {
        entry = calloc(1, sizeof(directoryEntry));
        entry->procs = NULL;
        entry->dirty_bit = 0;
    }

    directory_action res = DO_NOTHING;

    // case 1, write to modified line
    if ((!isRead) && (entry->dirty_bit)) {
        res = INVALIDATE_AND_GO_TO_CACHE;
    } // case 2: reading modified line
    else if (entry->dirty_bit) {
        res = GO_TO_CACHE;
    } // case 3: write to unmodified line
    else if (!isRead) {
        entry->dirty_bit = 1;
        res = INVALIDATE_AND_GO_TO_MEMORY;
    } else {
        res = GO_TO_MEMORY;
    }
    assert(res != DO_NOTHING);
    tree_insert(directory, addr, (void *)entry);

    return res;
}

int busReq(uint64_t addr, int procNum, void (*callback)(int, uint64_t))
{
    assert(pendingRequest == NULL);
    pendingRequest = calloc(1, sizeof(memReq));
    pendingRequest->addr = addr;
    pendingRequest->procNum = procNum;
    pendingRequest->squelch = 0;
    pendingRequest->callback = callback;
    
    countDown = DRAM_FETCH_TICKS;

    return countDown;
}



int tick()
{
    if (countDown > 0)
    {
        assert(pendingRequest);
        // Check if one of the caches responded to the request that we are
        // processing. If that's the case, we "squelch" the response and
        // make ourselves available for the next request.
        if (interComp->busReqCacheTransfer(pendingRequest->addr,
                                           pendingRequest->procNum))
        {
            pendingRequest->squelch = 1;
            countDown = 0;
            goto done;
        }

        countDown--;
    }

done:
    if (pendingRequest && countDown == 0)
    {
        if (!pendingRequest->squelch)
        {
            
            pendingRequest->callback(pendingRequest->procNum,
                                     pendingRequest->addr);
        }

        free(pendingRequest);
        pendingRequest = NULL;
    }

    return countDown;
}

int finish(int outFd)
{
    return 0;
}


void free_directory_entry(void *val) {
    directoryEntry *entry = (directoryEntry *)val;

    procEntry *curr = entry->procs;
    while (curr) {
        procEntry *nxt = curr->next;
        free(curr);
        curr = nxt;
    }
    free(entry);
}

int destroy(void)
{
    tree_show(directory, 1);
    free(self);
    free(pendingRequest);
    tree_free(directory, &free_directory_entry);

    return 0;
}
