#ifndef MEMORY_H
#define MEMORY_H

struct _interconn;

#include "common.h"
#include "interconnect.h"

typedef struct _memory_sim_args {
    int arg_count;
    char** arg_list;
} memory_sim_args;

typedef enum _directory_action {
    DO_NOTHING,
    GO_TO_MEMORY,
    GO_TO_CACHE,
    INVALIDATE_AND_GO_TO_MEMORY,
    INVALIDATE_AND_GO_TO_CACHE
} directory_action;

typedef struct _memory {
    sim_interface si;
    directory_action (*reqDirectory)(uint64_t addr, int procNum, int isRead);
    void (*updateDirectory)(uint64_t addr, int procNum);
    int (*removeProc)(uint64_t addr);
    int (*getProcForRequest)(uint64_t addr);
    int (*busReq)(uint64_t addr, int procNum, void (*callback)(int, uint64_t));
    void (*registerInterconnect)(struct _interconn* interconnect);
    debug_env_vars dbgEnv;
} memory;


#endif /* !MEMORY_H */
