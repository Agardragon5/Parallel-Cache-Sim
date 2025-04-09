#include "coher_internal.h"

void sendBusRd(uint64_t addr, int procNum)
{
    inter_sim->busReq(BUSRD, addr, procNum);
}

void sendBusWr(uint64_t addr, int procNum)
{
    inter_sim->busReq(BUSWR, addr, procNum);
}

void sendData(uint64_t addr, int procNum)
{
    inter_sim->busReq(DATA, addr, procNum);
}

void indicateShared(uint64_t addr, int procNum)
{
    inter_sim->busReq(SHARED, addr, procNum);
}

coherence_states
cacheMI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            sendBusWr(addr, procNum);
            return INVALID_MODIFIED;
        case MODIFIED:
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            // indicateShared(addr, procNum); // Needed for E state
            *ca = INVALIDATE;
            return INVALID;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                return MODIFIED;
            }

            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheMSI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
    uint64_t addr, int procNum) 
{
    switch(currentState)
    {
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
        case SHARED_STATE: 
            *permAvail = 1;
            if (is_read) {
                return SHARED_STATE;
            } else {
                sendBusWr(addr, procNum);
                return MODIFIED;
            }
        case MODIFIED:
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IS state on %lx, but request %d\n", addr,
                is_read);
            *permAvail = 0;
            return INVALID_SHARED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
    return INVALID;
}


coherence_states
snoopMSI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
            return INVALID;
        case SHARED_STATE:
            if (reqType == BUSRD) {
                return SHARED_STATE; 
            } else {
                *ca = INVALIDATE;
                return INVALID;
            }
        case MODIFIED:
            sendData(addr, procNum);

            if (reqType == BUSRD) {
                return SHARED_STATE;
            } else {
                *ca = INVALIDATE;
                return INVALID;
            }
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                return MODIFIED;
            }
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            if (reqType == DATA || reqType == SHARED) 
            {
                *ca = DATA_RECV;
                return SHARED_STATE;
            }
            return INVALID_SHARED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}