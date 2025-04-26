#ifndef DIRECTORY_H
#define DIRECTORY_H



typedef struct _procEntry {
    int proc;
    struct _procEntry *next;
} procEntry;

typedef struct _directoryEntry {
    int dirty_bit;
    procEntry *procs;
} directoryEntry;

#endif