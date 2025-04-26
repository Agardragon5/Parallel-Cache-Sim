#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

// Describes a DRAM request.
typedef struct _memReq {
    int procNum;
    uint64_t addr;
    int squelch;
    void (*callback)(int, uint64_t);
} memReq;

typedef struct _procEntry {
    int proc;
    struct _procEntry *next;
} procEntry;

typedef struct _directoryEntry {
    int dirty_bit;
    procEntry *procs;
} directoryEntry;

#endif // MEMORY_INTERNAL_H
