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

// process cache requests for MSI protocol
coherence_states
cacheMSI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
    uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID: 
            // tell cache data isn't ready, send out rd/wr request
            *permAvail = 0;
            if (is_read) {
                printf("INVALID -> INVALID_SHARED \n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                printf("INVALID -> INVALID_MODIFIED \n");
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            printf("INVALID -> INVALID \n");
            return INVALID;
        case SHARED_STATE:
            // data is avail, send wr req if necessary
            *permAvail = 1;
            if (is_read) {
                printf("SHARED_STATE -> SHARED_STATE \n");
                return SHARED_STATE;
            } else {
                printf("SHARED_STATE -> MODIFIED \n");
                sendBusWr(addr, procNum);
                return MODIFIED;
            }
        case MODIFIED:
            // do nothing
            printf("MODIFIED -> MODIFIED \n");
            *permAvail = 1;
            return MODIFIED;
        // all the following mean something has gone wrong
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
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

// handle cache requests for MESI
coherence_states 
cacheMESI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
    uint64_t addr, int procNum)
{
    switch (currentState) 
    {
        case INVALID: // same as with MSI
            *permAvail = 0;
            if (is_read) {
                printf("INVALID -> INVALID_SHARED \n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                printf("INVALID -> INVALID_MODIFIED \n");
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            printf("INVALID -> INVALID \n");
            return INVALID;
        case SHARED_STATE: // same as with MSI
            *permAvail = 1;
            if (is_read) {
                printf("SHARED_STATE -> SHARED_STATE \n");
                return SHARED_STATE;
            }
            printf("SHARED_STATE -> MODIFIED \n");
            sendBusWr(addr, procNum);
            return MODIFIED;
        case EXCLUSIVE: // stay in current state if read, otherwise go to M silently
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE \n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED \n");
            return MODIFIED;
        case MODIFIED: // same as with MSI
            printf("MODIFIED -> MODIFIED \n");
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED: // everything down here shouldn't be happening
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
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
cacheMESIF(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
    uint64_t addr, int procNum)
{
    switch (currentState) 
    {
        case INVALID: 
            *permAvail = 0;
            if (is_read) {
                printf("INVALID -> INVALID_SHARED\n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            }
            printf("INVALID -> INVALID_MODIFIED\n");
            sendBusWr(addr, procNum);
            return INVALID_MODIFIED;
        case SHARED_STATE:
            *permAvail = 1;
            if (is_read) {
                printf("SHARED -> SHARED\n");
                return SHARED_STATE;
            }
            printf("SHARED -> MODIFIED\n");
            sendBusWr(addr, procNum);
            return MODIFIED;
        case EXCLUSIVE:
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE\n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED\n");
            return MODIFIED;
        case FORWARD:
            *permAvail = 1;
            if (is_read) {
                printf("FORWARD -> FORWARD\n");
                return FORWARD;
            }
            printf("FORWARD -> MODIFIED\n");
            sendBusWr(addr, procNum);
            return MODIFIED;
        case MODIFIED:
            printf("MODIFIED -> MODIFIED\n");
            *permAvail = 1;
            return MODIFIED;
            case INVALID_MODIFIED: // everything down here shouldn't be happening
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
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
cacheMOESI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
    uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID: // same as with MSI
            *permAvail = 0;
            if (is_read) {
                printf("INVALID -> INVALID_SHARED \n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                printf("INVALID -> INVALID_MODIFIED \n");
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            printf("INVALID -> INVALID \n");
            return INVALID;
        case SHARED_STATE: // same as with MSI
            *permAvail = 1;
            if (is_read) {
                printf("SHARED_STATE -> SHARED_STATE \n");
                return SHARED_STATE;
            }
            printf("SHARED_STATE -> MODIFIED \n");
            sendBusWr(addr, procNum);
            return MODIFIED;
        case EXCLUSIVE: // stay in current state if read, otherwise go to M silently
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE \n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED \n");
            return MODIFIED;
        case MODIFIED: // same as with MSI
            printf("MODIFIED -> MODIFIED \n");
            *permAvail = 1;
            return MODIFIED;
        case OWNED:
            *permAvail = 1;
            printf("OWNED -> OWNED \n");
            return OWNED;
        case INVALID_MODIFIED: // everything down here shouldn't be happening
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
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
snoopMI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
            printf("INVALID -> INVALID \n"); 
            return INVALID;
        case MODIFIED: 
            sendData(addr, procNum);
            indicateShared(addr, procNum); // Needed for E state
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

// handle bus requests for MSI state
coherence_states
snoopMSI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
    uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState) 
    {
        case INVALID: // ignore if line isn't in cache 
            printf("INVALID -> INVALID \n"); 
            return INVALID;
        case SHARED_STATE: // indicate that data is shared 
            sendData(addr, procNum);
            if (reqType == BUSWR) {
                printf("SHARED_STATE -> INVALID \n"); 
                // if another processor tries to write, invalidate line
                *ca = INVALIDATE;
                return INVALID;
            } 
            printf("SHARED_STATE -> SHARED_STATE \n"); 
            // otherwise, just keep going
            return SHARED_STATE;
        case MODIFIED:
            sendData(addr, procNum); // send data to cache requesting it
            if (reqType == BUSRD) {
                printf("MODIFIED -> SHARED_STATE \n");
                return SHARED_STATE; // go to shared state
            } 
            printf("MODIFIED -> INVALID \n");
            *ca = INVALIDATE;
            return INVALID;
        case INVALID_SHARED: 
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_SHARED -> SHARED \n ");
                *ca = DATA_RECV; // if inbound request is data, recieve and move to shared
                return SHARED;
            }
            printf("INVALID_SHARED -> INVALID SHARED \n ");
            return INVALID_SHARED;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV; // stay in these intermediate states until the data is arriving
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
        
    }
    return INVALID;
}

// handle bus requests for MESI
coherence_states 
snoopMESI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
    uint64_t addr, int procNum) 
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID: // same as with MSI
            printf("INVALID -> INVALID \n");
            return INVALID;
        // same as with MSI, except now we use indicateShared to let snoops know if data
        // will be shared
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED_STATE -> INVALID \n");
                sendData(addr, procNum); // note that sendData is used because the line will be invalidated
                *ca = INVALIDATE;
                return INVALID;
            }
            printf("SHARED_STATE -> SHARED_STATE \n");
            indicateShared(addr, procNum);
            return SHARED_STATE;
        // if a busread comes in while in exclusive, send data
        // if it's a read, go to shared, otherwise invalidate the line
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> SHARED \n");
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                printf("EXCLUSIVE -> INVALID \n");
                sendData(addr, procNum);
                *ca = INVALIDATE;
                return INVALID; 
            }
            printf("EXCLUSIVE -> EXCLUSIVE \n");
            return EXCLUSIVE;

        // same as with MSI, except with indicateShared 
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> SHARED_STATE \n");
                indicateShared(addr, procNum);
                return SHARED_STATE;
            }
            printf("MODIFIED -> INVALID \n");
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        // waiting for inbound data, this is why we use indicateShared if data will remain in cache
        // if it's DATA, that means no other processors should have it
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID_SHARED -> EXCLUSIVE \n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID_SHARED -> SHARED \n");
                *ca = DATA_RECV;
                return SHARED;
            } 
            printf("INVALID_SHARED -> INVALID_SHARED \n");
            return INVALID_SHARED;
            
        case INVALID_MODIFIED: // same as with MESI
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
    return INVALID;
}

coherence_states
snoopMESIF(bus_req_type reqType, cache_action* ca, coherence_states currentState,
    uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
            printf("INVALID -> INVALID\n");
            return INVALID;
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID SHARED -> EXCLUSIVE\n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID SHARED -> SHARED\n");
                *ca = DATA_RECV;
                return SHARED_STATE;
            }
            printf("INVALID SHARED -> INVALID SHARED\n");
            return INVALID_SHARED;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == MODIFIED) {
                printf("INVALID MODIFIED -> MODIFIED\n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID MODIFIED -> INVALID MODIFIED\n");
            return INVALID_MODIFIED;
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED -> INVALID\n");
                *ca = INVALIDATE;
                return INVALID;
            }
            printf("SHARED -> SHARED");
            return SHARED_STATE;
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> FORWARD\n");
                indicateShared(addr, procNum);
                return FORWARD;
            } else if (reqType == BUSWR) {
                *ca = INVALIDATE;
                printf("EXCLUSIVE -> INVALID\n");
                sendData(addr, procNum);
                return INVALID;
            }
            printf("EXCLUSIVE -> EXCLUSIVE\n");
            return EXCLUSIVE;
        case FORWARD:
            if (reqType == BUSWR) {
                printf("FORWARD -> INVALID\n");
                *ca = INVALIDATE;
                sendData(addr, procNum);
                return INVALID;
            }
            printf("FORWARD -> FORWARD\n");
            indicateShared(addr, procNum);
            return FORWARD;
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> SHARED_STATE \n");
                indicateShared(addr, procNum);
                return SHARED_STATE;
            }
            printf("MODIFIED -> INVALID \n");
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;      
    }
    return INVALID;
}

coherence_states 
snoopMOESI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
    uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID: // same as with MSI
            printf("INVALID -> INVALID \n");
            return INVALID;
        // same as with MSI, except now we use indicateShared to let snoops know if data
        // will be shared
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED_STATE -> INVALID \n");
                sendData(addr, procNum); // note that sendData is used because the line will be invalidated
                *ca = INVALIDATE;
                return INVALID;
            }
            printf("SHARED_STATE -> SHARED_STATE \n");
            indicateShared(addr, procNum);
            return SHARED_STATE;
        // if a busread comes in while in exclusive, send data
        // if it's a read, go to shared, otherwise invalidate the line
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> SHARED \n");
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                printf("EXCLUSIVE -> INVALID \n");
                sendData(addr, procNum);
                *ca = INVALIDATE;
                return INVALID; 
            }
            printf("EXCLUSIVE -> EXCLUSIVE \n");
            return EXCLUSIVE;

        // same as with MSI, except with indicateShared 
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> OWNED \n");
                indicateShared(addr, procNum);
                return OWNED;
            }
            printf("MODIFIED -> INVALID \n");
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        case OWNED:
            if (reqType == BUSWR) {
                printf("OWNED -> INVALID\n");
                sendData(addr, procNum);
                return INVALID;
            }
            printf("OWNED -> OWNED\n");
            indicateShared(addr, procNum);
            return OWNED;
        // waiting for inbound data, this is why we use indicateShared if data will remain in cache
        // if it's DATA, that means no other processors should have it
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID_SHARED -> EXCLUSIVE \n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID_SHARED -> SHARED \n");
                *ca = DATA_RECV;
                return SHARED;
            } 
            printf("INVALID_SHARED -> INVALID_SHARED \n");
            return INVALID_SHARED;
            
        case INVALID_MODIFIED: // same as with MESI
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
    return INVALID;
}