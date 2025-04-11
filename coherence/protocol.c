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
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            return INVALID;
        case SHARED_STATE:
            // data is avail, send wr req if necessary
            *permAvail = 1;
            if (is_read) {
                return SHARED_STATE;
            } else {
                sendBusWr(addr, procNum);
                return MODIFIED;
            }
        case MODIFIED:
            // do nothing
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
                sendBusRd(addr, procNum);
                return INVALID_SHARED;
            } else {
                sendBusWr(addr, procNum);
                return INVALID_MODIFIED;
            }
            return INVALID;
        case SHARED_STATE: // same as with MSI
            *permAvail = 1;
            if (is_read) {
                return SHARED_STATE;
            }
            sendBusWr(addr, procNum);
            return MODIFIED;
        case EXCLUSIVE: // stay in current state if read, otherwise go to M silently
            *permAvail = 1;
            if (is_read) {
                return EXCLUSIVE;
            }
            return MODIFIED;
        case MODIFIED: // same as with MSI
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

// handle bus requests for MSI state
coherence_states
snoopMSI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
    uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState) 
    {
        case INVALID: // ignore if line isn't in cache 
            return INVALID;
        case SHARED_STATE: // indicate that data is shared
            sendData(addr, procNum);
            if (reqType == BUSWR) {
                // if another processor tries to write, invalidate line
                *ca = INVALIDATE;
                return INVALID;
            } 
            // otherwise, just keep going
            return SHARED_STATE;
        case MODIFIED:
            sendData(addr, procNum); // send data to cache requesting it
            if (reqType == BUSRD) {
                return SHARED_STATE; // go to shared state
            } 
            *ca = INVALIDATE;
            return INVALID;
        case INVALID_SHARED: 
            if (reqType == DATA || reqType == SHARED) {
                *ca = DATA_RECV; // if inbound request is data, recieve and move to shared
                return SHARED;
            }
            return INVALID_SHARED;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED) {
                *ca = DATA_RECV; // stay in these intermediate states until the data is arriving
                return MODIFIED;
            }
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
        
    }
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
            return INVALID;
        // same as with MSI, except now we use indicateShared to let snoops know if data
        // will be shared
        case SHARED_STATE:
            if (reqType == BUSWR) {
                sendData(addr, procNum); // note that sendData is used because the line will be invalidated
                *ca = INVALIDATE;
                return INVALID;
            }
            indicateShared(addr, procNum);
            return SHARED_STATE;
        // if a busread comes in while in exclusive, send data
        // if it's a read, go to shared, otherwise invalidate the line
        case EXCLUSIVE:
            if (reqType == BUSRD) {
                indicateShared(addr, procNum);
                return SHARED;
            } else if (reqType == BUSWR) {
                sendData(addr, procNum);
                *ca = INVALIDATE;
                return INVALID; 
            }
            return EXCLUSIVE;

        // same as with MSI, except with indicateShared 
        case MODIFIED:
            if (reqType == BUSRD) { 
                indicateShared(addr, procNum);
                return SHARED_STATE;
            }
            sendData(addr, procNum);
            *ca = INVALIDATE;
            return INVALID;
        // waiting for inbound data, this is why we use indicateShared if data will remain in cache
        // if it's DATA, that means no other processors should have it
        case INVALID_SHARED:
            if (reqType == DATA) {
                *ca = DATA_RECV;
                return EXCLUSIVE;
            } else if (reqType == SHARED) {
                *ca = DATA_RECV;
                return SHARED;
            } 
            return INVALID_SHARED;
            
        case INVALID_MODIFIED: // same as with MESI
            if (reqType == DATA || reqType == SHARED) {
                *ca = DATA_RECV;
                return MODIFIED;
            }
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
}