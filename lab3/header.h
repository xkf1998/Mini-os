#include <iostream>

#include <fstream>
#include <unistd.h> //getopt
#include <getopt.h>
#include <vector>
#include <deque>
#include <queue>
#include <cstring>
#include <list>
#include <climits>

#define TAU 50
#define DaemonTime 50
#define MAX_VPAGES 64
#define MAX_VMAS 10

#define mapsCost 300;
#define unmapsCost 400;
#define insCost 3100;
#define outsCost 2700;
#define finsCost 2800;
#define foutsCost 2400;
#define zerosCost 140;
#define segvCost 340;
#define segprotCost 420;

#define context_switchCost 130;
#define process_exitCost 1250;

using namespace std;
int MAX_FRAMES = 128;