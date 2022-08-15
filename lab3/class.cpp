#include "header.h"

unsigned long currInstr = 0; // the instr counter and a simulated timer

struct FTE
{
public:
    int phyFrameNum = -1;
    int processId = -1;
    int ProcPgId = -1;

    unsigned int age = 0;  // for aging
    int lastVisitTime = 0; // for last visit use

    FTE(){};
    FTE(int phyframenum, int processid, int pagenum)
    {
        phyFrameNum = phyframenum;
        processId = processid;
        ProcPgId = pagenum;
    };
};

typedef struct FTE frame_t;
frame_t *frame_table = (frame_t *)calloc(MAX_FRAMES, sizeof(frame_t));

struct PTE
{
public:
    unsigned int vmaValid : 1;
    unsigned int FILEMAPPED : 1;

    unsigned int PRESENT : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;
    unsigned int frameid : 7;

    PTE()
    {
        PRESENT = 0;
        REFERENCED = 0;
        MODIFIED = 0;
        WRITE_PROTECT = 0;
        PAGEDOUT = 0;
        frameid = 0;

        FILEMAPPED = 0;
        vmaValid = 0;
    }
};

typedef struct PTE pte_t;

struct VMA
{
public:
    int starting_virtual_page;
    int ending_virtual_page;
    int write_protected;
    int file_mapped;

    VMA() {}
    VMA(int start, int end, int write, int filemap)
    {
        starting_virtual_page = start;
        ending_virtual_page = end;
        write_protected = write;
        file_mapped = filemap;
    }
};

typedef struct VMA vma_t;

class Process
{
public:
    int pid;
    int vma_size;
    pte_t page_table[MAX_VPAGES];
    vma_t vma_table[MAX_VMAS];

    unsigned long maps = 0;
    unsigned long ins = 0;
    unsigned long unmaps = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segprot = 0;
    unsigned long outs = 0;
    unsigned long segv = 0;
    unsigned long fins = 0;

    Process() {}
    Process(int id)
    {
        pid = id;
    }
};

vector<Process *> process_vector;
vector<int> randvals;

class Pager
{
public:
    int frameptr;
    virtual frame_t *select_victim_frame() = 0;
};

class FIFO : public Pager
{
public:
    FIFO()
    {
        frameptr = -1;
    }

    frame_t *select_victim_frame()
    {
        frameptr++;
        frameptr %= MAX_FRAMES;
        return &frame_table[frameptr];
    }
};

class RANDOM : public Pager
{
public:
    int ofs = 0;
    RANDOM()
    {
        frameptr = 0;
    }
    frame_t *select_victim_frame()
    {
        if (ofs >= randvals.size())
            ofs %= randvals.size();
        frameptr = randvals[ofs++] % MAX_FRAMES;
        return &frame_table[frameptr];
    }
};

class CLOCK : public Pager
{
public:
    CLOCK()
    {
        frameptr = -1;
    }

    frame_t *select_victim_frame()
    {
        int n = MAX_FRAMES * 2;
        while (n > 0)
        {
            frameptr++;
            if (frameptr >= MAX_FRAMES)
                frameptr %= MAX_FRAMES;
            int procId = frame_table[frameptr].processId;
            int procPgId = frame_table[frameptr].ProcPgId;
            if (process_vector[procId]->page_table[procPgId].REFERENCED == 0)
                break;
            else
            {
                process_vector[procId]->page_table[procPgId].REFERENCED = 0;
            }
            n--;
        }
        return &frame_table[frameptr];
    }
};

class NRU : public Pager
{
public:
    int prevInstrCount;
    NRU()
    {
        frameptr = -1;
        prevInstrCount = 0;
    }
    frame_t *select_victim_frame()
    {
        frameptr++;
        bool flag = currInstr - prevInstrCount >= DaemonTime;

        int lowestIndex = INT_MAX;
        int minptr = -1;
        int n = MAX_FRAMES;
        while (n > 0)
        {
            frameptr %= MAX_FRAMES;
            int procId = frame_table[frameptr].processId;
            int procPgId = frame_table[frameptr].ProcPgId;
            unsigned int modify = process_vector[procId]->page_table[procPgId].MODIFIED;
            unsigned int ref = process_vector[procId]->page_table[procPgId].REFERENCED;
            int class_index = 2 * ref + modify;
            if (class_index < lowestIndex)
            {
                lowestIndex = class_index;
                minptr = frameptr;
            }
            if (!flag && class_index == 0) // we don't need to update the reference and already find the lowest class index
                break;
            if (flag)
                process_vector[procId]->page_table[procPgId].REFERENCED = 0; // need to reset all the referenced bit
            frameptr++;
            n--;
        }
        if (flag)
            prevInstrCount = currInstr;
        frameptr = minptr;
        return &frame_table[frameptr];
    }
};

class AGING : public Pager
{
public:
    AGING()
    {
        frameptr = -1;
    }
    frame_t *select_victim_frame()
    {
        frameptr++;
        long long int minAge = INT64_MAX;
        int minptr = -1;
        int procId, procPgId;
        int n = MAX_FRAMES;
        while (n > 0)
        {
            frameptr %= MAX_FRAMES;
            procId = frame_table[frameptr].processId;
            procPgId = frame_table[frameptr].ProcPgId;
            frame_table[frameptr].age >>= 1;

            unsigned int ref = process_vector[procId]->page_table[procPgId].REFERENCED;
            ref <<= 31;
            frame_table[frameptr].age |= ref;

            if (frame_table[frameptr].age < minAge)
            {
                minAge = frame_table[frameptr].age;
                minptr = frameptr;
            }
            if (process_vector[procId]->page_table[procPgId].REFERENCED)
                process_vector[procId]->page_table[procPgId].REFERENCED = 0;
            frameptr++;
            n--;
        }
        frameptr = minptr;
        frame_table[minptr].age = 0;
        return &frame_table[frameptr];
    }
};

class WORKINGSET : public Pager
{
public:
    WORKINGSET()
    {
        frameptr = -1;
    }

    frame_t *select_victim_frame()
    {
        frameptr++;
        int maxInteval = INT_MIN;
        int minptr = -1;
        int procId, procPgId;

        int n = MAX_FRAMES * 2;
        while (n > 0)
        {
            frameptr %= MAX_FRAMES;
            int intevalTime = currInstr - frame_table[frameptr].lastVisitTime;

            procId = frame_table[frameptr].processId;
            procPgId = frame_table[frameptr].ProcPgId;
            unsigned int ref = process_vector[procId]->page_table[procPgId].REFERENCED;
            if (ref)
            {
                process_vector[procId]->page_table[procPgId].REFERENCED = 0;
                frame_table[frameptr].lastVisitTime = currInstr;
            }
            else if (intevalTime >= TAU)
            {
                frame_table[frameptr].lastVisitTime = currInstr;
                return &frame_table[frameptr];
            }
            else if (intevalTime > maxInteval)
            {
                maxInteval = intevalTime;
                minptr = frameptr;
            }
            frameptr++;
            n--;
        }
        frameptr = minptr;
        frame_table[frameptr].lastVisitTime = currInstr;
        return &frame_table[frameptr];
    }
};