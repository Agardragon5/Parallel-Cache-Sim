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
            printf("INVALID -> INVALID\n");
            return INVALID;
        case SHARED_STATE:
            // data is avail, send wr req if necessary
            if (is_read) {
                *permAvail = 1;
                printf("SHARED -> SHARED \n");;
                return SHARED_STATE;
            } else {
                *permAvail = 0;
                printf("SHARED -> SHARED_MODIFIED \n");
                // sendData(addr, procNum); 
                sendBusWr(addr, procNum);
                return SHARED_MODIFIED;
            }
        case MODIFIED:
            // do nothing
            printf("MODIFIED -> MODIFIED \n");
            *permAvail = 1;
            return MODIFIED;
        // all the following mean something has gone wrong
        case INVALID_MODIFIED:
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n"); 
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
        printf("INVALID_SHARED -> INVALID_SHARED \n");
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            *permAvail = 0;
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            printf("SHARED_MODIFIED -> SHARED_MODIFIED \n");
            *permAvail = 0;
            return SHARED_MODIFIED;
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
                printf("INVALID -> INVALID_SHARED_EXCLUSIVE\n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                printf("INVALID -> INVALID_MODIFIED\n");
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            printf("INVALID -> INVALID\n");
            return INVALID;
        case SHARED_STATE: // same as with MSI
            if (is_read) {
                *permAvail = 1;
                printf("SHARED -> SHARED\n");
                return SHARED_STATE;
            } else {
                *permAvail = 0;
                printf("SHARED -> SHARED_MODIFIED\n");
                sendBusWr(addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE: // stay in current state if read, otherwise go to M silently
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE\n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED\n");
            return MODIFIED;
        case MODIFIED: // same as with MSI
            printf("MODIFIED -> MODIFIED\n");
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED: // everything down here shouldn't be happening
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            *permAvail = 0;
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            *permAvail = 0;
            printf("INVALID_SHARED_EXCLUSIVE -> INVALID_SHARED_EXCLUSIVE \n");
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            printf("SHARED_MODIFIED -> SHARED_MODIFIED\n");
            *permAvail = 0;
            return SHARED_MODIFIED;
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
                printf("INVALID -> INVALID_EXCLUSIVE_FORWARD \n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            }
            printf("INVALID -> INVALID_MODIFIED \n");
            sendBusWr(addr, procNum);
            return INVALID_MODIFIED;
        case SHARED_STATE: // same as with MSI
            if (is_read) {
                *permAvail = 1;
                printf("SHARED -> SHARED \n");
                return SHARED_STATE;
            } else {
                *permAvail = 0;
                printf("SHARED -> SHARED_MODIFIED \n");
                sendBusWr(addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE:
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE \n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED \n");
            return MODIFIED;
        case FORWARD:
            *permAvail = 1;
            if (is_read) {
                printf("FORWARD -> FORWARD \n");
                return FORWARD;
            }
            *permAvail = 0;
            printf("FORWARD -> FORWARD_MODIFIED \n"); //edited
            sendBusWr(addr, procNum);
            return FORWARD_MODIFIED;
        case MODIFIED:
            printf("MODIFIED -> MODIFIED \n");
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED: 
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            printf("INVALID_EXCLUSIVE_FORWARD -> INVALID_EXCLUSIVE_FORWARD \n");
            *permAvail = 0;
            return INVALID_SHARED;
        case SHARED_MODIFIED:
            printf("SHARED_MODIFIED -> SHARED_MODIFIED\n");
            *permAvail = 0;
            return SHARED_MODIFIED;
        case FORWARD_MODIFIED:
            printf("FORWARD_MODIFIED -> FORWARD_MODIFIED\n");
            *permAvail = 0;
            return FORWARD_MODIFIED;
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
                printf("INVALID -> INVALID_SHARED_EXCLUSIVE\n");
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                printf("INVALID -> INVALID_MODIFIED\n");
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            printf("INVALID -> INVALID\n");
            return INVALID;
        case SHARED_STATE: // same as with MSI
            if (is_read) {
                *permAvail = 1;
                printf("SHARED -> SHARED \n");
                return SHARED_STATE;
            } else {
                *permAvail = 0;
                printf("SHARED -> SHARED_MODIFIED \n");
                // sendData(addr, procNum); 
                sendBusWr(addr, procNum);
                return SHARED_MODIFIED;
            }
        case EXCLUSIVE: // stay in current state if read, otherwise go to M silently
            *permAvail = 1;
            if (is_read) {
                printf("EXCLUSIVE -> EXCLUSIVE\n");
                return EXCLUSIVE;
            }
            printf("EXCLUSIVE -> MODIFIED\n");
            return MODIFIED;
        case MODIFIED: // same as with MSI
            printf("MODIFIED -> MODIFIED\n");
            *permAvail = 1;
            return MODIFIED;
        case OWNED:
            *permAvail = 1;
            printf("OWNED -> OWNED\n");
            return OWNED;
        case INVALID_MODIFIED: // everything down here shouldn't be happening
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    // is_read);
            printf("INVALID_MODIFIED -> INVALID_MODIFIED\n");
            *permAvail = 0;
            return INVALID_MODIFIED;
        case INVALID_SHARED:
            // fprintf(stderr, "IM state on %lx, but request %d\n", addr,
            //         is_read);
            printf("INVALID_SHARED_EXCLUSIVE -> INVALID_SHARED_EXCLUSIVE\n");
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
            printf("INVALID -> INVALID\n"); 
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
            if (reqType == BUSWR) {
                printf("SHARED -> INVALID \n"); 
                // if another processor tries to write, invalidate line
                *ca = INVALIDATE;
                return INVALID;
            } 
            printf("SHARED -> SHARED \n"); 
            // otherwise, just keep going
            return SHARED_STATE;
        case MODIFIED:
            if (reqType == BUSRD) {
                printf("MODIFIED -> SHARED \n");
                return SHARED_STATE; // go to shared state
            } 
            sendData(addr, procNum);
            printf("MODIFIED -> INVALID \n");
            *ca = INVALIDATE;
            return INVALID;
        case INVALID_SHARED: 
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_SHARED -> SHARED \n");
                *ca = DATA_RECV; // if inbound request is data, recieve and move to shared
                return SHARED;
            }
            printf("INVALID_SHARED -> INVALID_SHARED \n");
            return INVALID_SHARED;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV; // stay in these intermediate states until the data is arriving
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        case SHARED_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                printf("SHARED_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            indicateShared(addr, procNum);
            printf("SHARED_MODIFIED -> SHARED_MODIFIED \n");
            return SHARED_MODIFIED;
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
            printf("INVALID -> INVALID\n");
            return INVALID;
        // same as with MSI, except now we use indicateShared to let snoops know if data
        // will be shared
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED -> INVALID\n");
                *ca = INVALIDATE;
                return INVALID;
            }
            printf("SHARED -> SHARED\n");
            indicateShared(addr, procNum);
            return SHARED_STATE;
        // if a busread comes in while in exclusive, send data
        // if it's a read, go to shared, otherwise invalidate the line
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> SHARED\n");
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                printf("EXCLUSIVE -> INVALID\n");
                *ca = INVALIDATE;
                return INVALID; 
            }
            printf("EXCLUSIVE -> EXCLUSIVE\n");
            return EXCLUSIVE;

        // same as with MSI, except with indicateShared 
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> SHARED\n");
                indicateShared(addr, procNum);
                return SHARED_STATE;
            }
            printf("MODIFIED -> INVALID\n");
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        // waiting for inbound data, this is why we use indicateShared if data will remain in cache
        // if it's DATA, that means no other processors should have it
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID_SHARED_EXCLUSIVE -> EXCLUSIVE\n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID_SHARED_EXCLUSIVE -> SHARED\n");
                *ca = DATA_RECV;
                return SHARED;
            }
            printf("INVALID_SHARED_EXCLUSIVE -> INVALID_SHARED_EXCLUSIVE\n");
            return INVALID_SHARED;
            
        case INVALID_MODIFIED: // same as with MESI
            if (reqType == DATA) {
                printf("INVALID_MODIFIED -> MODIFIED\n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED\n");
            return INVALID_MODIFIED;
        case SHARED_MODIFIED:
            if (reqType == DATA) { // edited
                printf("SHARED_MODIFIED -> MODIFIED\n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("SHARED_MODIFIED -> SHARED_MODIFIED\n");
            return SHARED_MODIFIED;
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
            printf("INVALID -> INVALID \n");
            return INVALID;
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID_EXCLUSIVE_FORWARD -> EXCLUSIVE \n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID_EXCLUSIVE_FORWARD -> FORWARD \n");
                *ca = DATA_RECV;
                return FORWARD;
            }
            printf("INVALID_EXCLUSIVE_FORWARD -> INVALID_EXCLUSIVE_FORWARD \n");
            return INVALID_SHARED;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED \n");
            return INVALID_MODIFIED;
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED -> INVALID \n");
                *ca = INVALIDATE;
                return INVALID;
            } 
            printf("SHARED -> SHARED \n");
            return SHARED_STATE;
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> SHARED \n");
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                *ca = INVALIDATE;
                printf("EXCLUSIVE -> INVALID \n");
                return INVALID;
            }
            printf("EXCLUSIVE -> EXCLUSIVE \n");
            return EXCLUSIVE;
        case FORWARD:
            if (reqType == BUSWR) {
                printf("FORWARD -> INVALID \n");
                *ca = INVALIDATE;
                sendData(addr, procNum);
                return INVALID;
            }
            printf("FORWARD -> SHARED \n");
            indicateShared(addr, procNum);
            return SHARED;
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> SHARED \n");
                indicateShared(addr, procNum);
                return SHARED_STATE;
            }
            indicateShared(addr, procNum);
            printf("MODIFIED -> INVALID \n");
            *ca = INVALIDATE;
            return INVALID;
        case SHARED_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                printf("SHARED_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("SHARED_MODIFIED -> SHARED_MODIFIED \n");
            return SHARED_MODIFIED;
        case FORWARD_MODIFIED:
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                printf("FORWARD_MODIFIED -> SHARED_MODIFIED \n");
                return SHARED_MODIFIED;
            }
            else if (reqType == DATA) {
                printf("FORWARD_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("FORWARD_MODIFIED -> FORWARD_MODIFIED \n");
            return FORWARD_MODIFIED;
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
            printf("INVALID -> INVALID\n");
            return INVALID;
        // same as with MSI, except now we use indicateShared to let snoops know if data
        // will be shared
        case SHARED_STATE:
            if (reqType == BUSWR) {
                printf("SHARED -> INVALID\n");
                *ca = INVALIDATE;
                return INVALID;
            }
            printf("SHARED -> SHARED\n");
            indicateShared(addr, procNum);
            return SHARED_STATE;
        // if a busread comes in while in exclusive, send data
        // if it's a read, go to shared, otherwise invalidate the line
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                printf("EXCLUSIVE -> SHARED\n");
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                printf("EXCLUSIVE -> INVALID\n");
                *ca = INVALIDATE;
                return INVALID; 
            }
            printf("EXCLUSIVE -> EXCLUSIVE\n");
            return EXCLUSIVE;

        // same as with MSI, except with indicateShared 
        case MODIFIED:
            if (reqType == BUSRD) { 
                printf("MODIFIED -> OWNED\n");
                indicateShared(addr, procNum);
                return OWNED;
            }
            printf("MODIFIED -> INVALID\n");
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        case OWNED:
            if (reqType == BUSWR) {
                printf("OWNED -> INVALID\n");
                return INVALID;
            }
            printf("OWNED -> OWNED\n");
            indicateShared(addr, procNum);
            return OWNED;
        // waiting for inbound data, this is why we use indicateShared if data will remain in cache
        // if it's DATA, that means no other processors should have it
        case INVALID_SHARED:
            if (reqType == DATA) {
                printf("INVALID_SHARED_EXCLUSIVE -> EXCLUSIVE\n");
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                printf("INVALID_SHARED_EXCLUSIVE -> SHARED\n");
                *ca = DATA_RECV;
                return SHARED;
            } 
            printf("INVALID_SHARED_EXCLUSIVE -> INVALID_SHARED_EXCLUSIVE\n");
            return INVALID_SHARED;
            
        case INVALID_MODIFIED: // same as with MESI
            if (reqType == DATA || reqType == SHARED) {
                printf("INVALID_MODIFIED -> MODIFIED\n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("INVALID_MODIFIED -> INVALID_MODIFIED\n");
            return INVALID_MODIFIED;
        case SHARED_MODIFIED:
            if (reqType == DATA) {
                printf("SHARED_MODIFIED -> MODIFIED \n");
                *ca = DATA_RECV;
                return MODIFIED;
            }
            printf("SHARED_MODIFIED -> SHARED_MODIFIED \n");
            return SHARED_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
    return INVALID;
}