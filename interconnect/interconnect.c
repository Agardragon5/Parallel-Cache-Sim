#include <getopt.h>
#include <stdio.h>

#include <memory.h>
#include <interconnect.h>

typedef enum _bus_req_state
{
    NONE,
    QUEUED,
    TRANSFERING_CACHE,
    TRANSFERING_MEMORY,
    WAITING_CACHE,
    WAITING_MEMORY,
    INVALIDATING_LINE,
    REQUESTING_CACHE,
    REQUESTING_AND_INVALIDATING
} bus_req_state;

typedef struct _bus_req {
    bus_req_type brt;
    bus_req_state currentState;
    uint64_t addr;
    int procNum;
    uint8_t shared;
    uint8_t data;
    uint8_t dataAvail;
    directory_action dirAction;
    struct _bus_req* next;
} bus_req;

bus_req* pendingRequest = NULL;
bus_req** queuedRequests;

interconn* self;
coher* coherComp;
memory* memComp;

int CADSS_VERBOSE = 0;
int processorCount = 1;
int isCrossBar = 0;

static const char* req_state_map[] = {
    [NONE] = "None",
    [QUEUED] = "Queued",
    [TRANSFERING_CACHE] = "Cache-to-Cache Transfer",
    [TRANSFERING_MEMORY] = "Memory Transfer",
    [WAITING_CACHE] = "Waiting for Cache",
    [WAITING_MEMORY] = "Waiting for Memory",
};

static const char* req_type_map[]
    = {[NO_REQ] = "None", [BUSRD] = "BusRd",   [BUSWR] = "BusRdX",
       [DATA] = "Data",   [SHARED] = "Shared", [MEMORY] = "Memory"};

const int CACHE_DELAY = 10;
const int CACHE_TRANSFER = 10;
const int INVALIDATION_DELAY = 10;

void registerCoher(coher* cc);
void busReq(bus_req_type brt, uint64_t addr, int procNum);
int busReqCacheTransfer(uint64_t addr, int procNum);
void printInterconnState(void);
void interconnNotifyState(void);

// Helper methods for per-processor request queues.
static void enqBusRequest(bus_req* pr, int procNum)
{
    bus_req* iter;
    // No items in the queue.
    if (!queuedRequests[procNum])
    {
        queuedRequests[procNum] = pr;
        return;
    }

    // Add request to the end of the queue.
    iter = queuedRequests[procNum];
    while (iter->next)
    {
        iter = iter->next;
    }

    pr->next = NULL;
    iter->next = pr;
}

static bus_req* deqBusRequest(int procNum)
{
    bus_req* ret;

    ret = queuedRequests[procNum];

    // Move the head to the next request (if there is one).
    if (ret)
    {
        queuedRequests[procNum] = ret->next;
    }

    return ret;
}

static int busRequestQueueSize(int procNum)
{
    int count = 0;
    bus_req* iter;

    if (!queuedRequests[procNum])
    {
        return 0;
    }

    iter = queuedRequests[procNum];
    while (iter)
    {
        iter = iter->next;
        count++;
    }

    return count;
}

interconn* init(inter_sim_args* isa)
{
    int op;

    while ((op = getopt(isa->arg_count, isa->arg_list, "c")) != -1)
    {
        switch (op)
        {
            case 'c':
                isCrossBar = 1;
                break;
            default:
                break;
        }
    }   


    queuedRequests = malloc(sizeof(bus_req*) * processorCount);
    for (int i = 0; i < processorCount; i++)
    {
        queuedRequests[i] = NULL;
    }

    self = malloc(sizeof(interconn));
    self->busReq = busReq;
    self->registerCoher = registerCoher;
    self->busReqCacheTransfer = busReqCacheTransfer;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;

    memComp = isa->memory;
    memComp->registerInterconnect(self);

    return self;
}

int countDown = 0;
int lastProc = 0; // for round robin arbitration

void registerCoher(coher* cc)
{
    coherComp = cc;
}

void memReqCallback(int procNum, uint64_t addr)
{
    if (!pendingRequest)
    {
        return;
    }

    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
    {
        pendingRequest->dataAvail = 1;
    }
}

void busReq(bus_req_type brt, uint64_t addr, int procNum)
{

    int isRead = (brt == BUSRD);

   
    if (pendingRequest == NULL)
    {
        assert(brt != SHARED);
        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = WAITING_CACHE;
        nextReq->addr = addr;
        nextReq->procNum = procNum;
        nextReq->dataAvail = 0;
        if (isCrossBar) {
            nextReq->dirAction = memComp->reqDirectory(addr, procNum, isRead);
        }

        pendingRequest = nextReq;
        
        countDown = CACHE_DELAY;

        if (isCrossBar) {
            switch (pendingRequest->dirAction)
            {
            case GO_TO_MEMORY:
                break;
            case INVALIDATE_AND_GO_TO_MEMORY:
                pendingRequest->currentState = INVALIDATING_LINE;
                break;
            case GO_TO_CACHE:
                pendingRequest->currentState = REQUESTING_CACHE;
                break;
            case INVALIDATE_AND_GO_TO_CACHE:
                pendingRequest->currentState = REQUESTING_AND_INVALIDATING;
                break;
            default:
                fprintf(stderr, "INVALID ACTION FROM DIRECTORY, ABORTING\n");
                assert(0);
                break;
            }
        }
        
        return;
    }
    else if (brt == SHARED && pendingRequest->addr == addr)
    {
        pendingRequest->shared = 1;
        return;
    }
    else if (brt == DATA && pendingRequest->addr == addr)
    {
        pendingRequest->data = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        countDown = CACHE_TRANSFER;
        return;
    }
    else
    {
        assert(brt != SHARED);
        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = QUEUED;
        nextReq->addr = addr;
        nextReq->procNum = procNum;
        nextReq->dataAvail = 0;
        
        if (isCrossBar) {
            nextReq->dirAction = memComp->reqDirectory(addr, procNum, isRead);
        }

        enqBusRequest(nextReq, procNum);
    }
}


/**
 * @brief called if -c flag passed into interconnect module
 * @note unless -d and -s 5 are passed into coherence / memory this
 *       will not work correctly
 */
int crossBarTick() 
{
    memComp->si.tick();

    if (self->dbgEnv.cadssDbgWatchedComp && !self->dbgEnv.cadssDbgNotifyState)
    {
        // printInterconnState();
    }
    if (countDown > 0)
    {
        assert(pendingRequest != NULL);
        countDown--;

        // If the count-down has elapsed (or there hasn't been a
        // cache-to-cache transfer, the memory will respond with
        // the data.
        if (pendingRequest->dataAvail)
        { 

            pendingRequest->currentState = TRANSFERING_MEMORY;
            countDown = 0;
        }

        if (countDown == 0)
        {
            
            if (pendingRequest->currentState == WAITING_CACHE) {
                countDown
                    = memComp->busReq(pendingRequest->addr,
                                      pendingRequest->procNum, memReqCallback);

                                      pendingRequest->currentState = WAITING_MEMORY;
                // do not notify all caches of this, however

            } else if (pendingRequest->currentState == REQUESTING_CACHE) {
                int proc = memComp->getProcForRequest(pendingRequest->addr);
                coherComp->busReq(BUSRD, pendingRequest->addr, proc);
                if (pendingRequest->data == 1)
                {
                    pendingRequest->brt = DATA;
                } else {
                    printf("SOMETHING HAS GONE WRONG\n");
                    assert(0);
                }
            } else if (pendingRequest->currentState == REQUESTING_AND_INVALIDATING) {
                int proc = memComp->removeProc(pendingRequest->addr);
                assert(proc != -1);
                coherComp->busReq(BUSRD, pendingRequest->addr, proc);
                countDown = memComp->busReq(pendingRequest->addr, 
                                            pendingRequest->procNum, 
                                            memReqCallback);
                while ((proc = memComp->removeProc(pendingRequest->addr)) != -1) {
                    coherComp->busReq(BUSWR, pendingRequest->addr, proc);
                }

                if (pendingRequest->data == 1)
                {
                    pendingRequest->brt = DATA;
                } else {
                    printf("SOMETHING HAS GONE WRONG\n");
                    assert(0);
                }
            }
            else if (pendingRequest->currentState == TRANSFERING_MEMORY)
            {
                bus_req_type brt
                    = (pendingRequest->shared == 1) ? SHARED : DATA;
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);
                memComp->updateDirectory(pendingRequest->addr, pendingRequest->procNum);
                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            } 
            else if (pendingRequest->currentState == INVALIDATING_LINE) 
            {
                int proc;
                while ((proc = memComp->removeProc(pendingRequest->addr)) != -1) {
                    coherComp->busReq(BUSWR, pendingRequest->addr, proc);
                }
                countDown = memComp->busReq(pendingRequest->addr, 
                                            pendingRequest->procNum, 
                                            memReqCallback);
                pendingRequest->currentState = WAITING_MEMORY;
            } 
            else if (pendingRequest->currentState == TRANSFERING_CACHE)
            {
                bus_req_type brt = pendingRequest->brt;
                if (pendingRequest->shared == 1)
                    brt = SHARED;
                
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);
                memComp->updateDirectory(pendingRequest->addr, pendingRequest->procNum);
                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            }
        }
    }
    else if (countDown == 0)
    {
        for (int i = 0; i < processorCount; i++) // this selects the next request I believe
        {
            int pos = (i + lastProc) % processorCount;
            if (queuedRequests[pos] != NULL)
            {
                
                pendingRequest = deqBusRequest(pos);
                countDown = CACHE_DELAY;
                pendingRequest->currentState = WAITING_CACHE;
                if (isCrossBar) {
                    switch (pendingRequest->dirAction)
                    {
                        case GO_TO_MEMORY:
                            break;
                        case INVALIDATE_AND_GO_TO_MEMORY:
                            pendingRequest->currentState = INVALIDATING_LINE;
                            break;
                        case GO_TO_CACHE:
                            pendingRequest->currentState = REQUESTING_CACHE;
                            break;
                        case INVALIDATE_AND_GO_TO_CACHE:
                            pendingRequest->currentState = REQUESTING_AND_INVALIDATING;
                            break;
                        default:
                            fprintf(stderr, "INVALID ACTION FROM DIRECTORY, ABORTING\n");
                            assert(0);
                            break;
                    }
                }
                lastProc = (pos + 1) % processorCount;
                break;
            }
        }
    }

    return 0;
}

int tick()
{
    if (isCrossBar) {
        return crossBarTick();
    }

    memComp->si.tick();

    if (self->dbgEnv.cadssDbgWatchedComp && !self->dbgEnv.cadssDbgNotifyState)
    {
        // printInterconnState();
    }

    if (countDown > 0)
    {
        assert(pendingRequest != NULL);
        countDown--;

        // If the count-down has elapsed (or there hasn't been a
        // cache-to-cache transfer, the memory will respond with
        // the data.
        if (pendingRequest->dataAvail)
        {
            pendingRequest->currentState = TRANSFERING_MEMORY;
            countDown = 0;
        }

        if (countDown == 0)
        {
            if (pendingRequest->currentState == WAITING_CACHE)
            {
                // Make a request to memory.
                countDown
                    = memComp->busReq(pendingRequest->addr,
                                      pendingRequest->procNum, memReqCallback);

                pendingRequest->currentState = WAITING_MEMORY;

                // The processors will snoop for this request as well.
                for (int i = 0; i < processorCount; i++)
                {
                    if (pendingRequest->procNum != i)
                    {
                        coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, i);
                    }
                }

                if (pendingRequest->data == 1)
                {
                    pendingRequest->brt = DATA;
                }
            }
            else if (pendingRequest->currentState == TRANSFERING_MEMORY)
            {
                bus_req_type brt
                    = (pendingRequest->shared == 1) ? SHARED : DATA;
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);

                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            }
            else if (pendingRequest->currentState == TRANSFERING_CACHE)
            {
                bus_req_type brt = pendingRequest->brt;
                if (pendingRequest->shared == 1)
                    brt = SHARED;

                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);

                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            }
        }
    }
    else if (countDown == 0)
    {
        for (int i = 0; i < processorCount; i++) // this selects the next request I believe
        {
            int pos = (i + lastProc) % processorCount;
            if (queuedRequests[pos] != NULL)
            {
                pendingRequest = deqBusRequest(pos);
                countDown = CACHE_DELAY;
                pendingRequest->currentState = WAITING_CACHE;

                lastProc = (pos + 1) % processorCount;
                break;
            }
        }
    }

    return 0;
}

void printInterconnState(void)
{
    if (!pendingRequest)
    {
        return;
    }

    printf("--- Interconnect Debug State (Processors: %d) ---\n"
           "       Current Request: \n"
           "             Processor: %d\n"
           "               Address: 0x%016lx\n"
           "                  Type: %s\n"
           "                 State: %s\n"
           "         Shared / Data: %s\n"
           "                  Next: %p\n"
           "             Countdown: %d\n"
           "    Request Queue Size: \n",
           processorCount, pendingRequest->procNum, pendingRequest->addr,
           req_type_map[pendingRequest->brt],
           req_state_map[pendingRequest->currentState],
           pendingRequest->shared ? "Shared" : "Data", pendingRequest->next,
           countDown);

    for (int p = 0; p < processorCount; p++)
    {
        printf("       - Processor[%02d]: %d\n", p, busRequestQueueSize(p));
    }
}

void interconnNotifyState(void)
{
    if (!pendingRequest)
        return;

    if (self->dbgEnv.cadssDbgExternBreak)
    {
        printInterconnState();
        raise(SIGTRAP);
        return;
    }

    if (self->dbgEnv.cadssDbgWatchedComp && self->dbgEnv.cadssDbgNotifyState)
    {
        self->dbgEnv.cadssDbgNotifyState = 0;
        printInterconnState();
    }
}

// Return a non-zero value if the current request
// was satisfied by a cache-to-cache transfer.
int busReqCacheTransfer(uint64_t addr, int procNum)
{
    assert(pendingRequest);

    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
        return (pendingRequest->currentState == TRANSFERING_CACHE);

    return 0;
}

int finish(int outFd)
{
    memComp->si.finish(outFd);
    return 0;
}

int destroy(void)
{
    
    // TODO
    memComp->si.destroy();
    return 0;
}
