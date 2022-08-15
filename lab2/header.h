#ifndef CPU_SCHEDULER_HEADER
#define CPU_SCHEDULER_HEADER
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <queue>
#include <list>
#include <getopt.h>

using namespace std;

enum trans
{
    CREATED,
    TRANS_READY,
    TRANS_RUNNING,
    TRANS_BLOCKED,
    TRANS_PREEMPT
};

enum sched_type
{
    F,
    L,
    S,
    R,
    P,
    E
};