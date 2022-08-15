#include "class.cpp"
#include <cstring>


ifstream inputFile;

bool v_flag = false, f_flag = false, q_flag=false;
int tot_movement = 0, max_waittime = INT_MIN;
double turnaround = 0, waittime = 0;
char type;


Scheduler* scheduler = nullptr;

string getNextLine(ifstream &inputFile)
{
    string res = "";
    getline(inputFile, res);
    if (inputFile.eof())
    {
        inputFile.close();
        return "eof";
    }
    while (res[0] == '#') // see whether this line is begin with #
    {
        getline(inputFile, res);
        if (inputFile.eof())
        {
            inputFile.close();
            return "eof";
        }
    }
    return res;
}

vector<int> read_space_num(string str)
{
    vector<int> a;
    int temp = 0;
    for (int i = 0; i != str.size(); ++i)
    {
        if (isdigit(str[i]))
        {
            temp = temp * 10 + (str[i] - 48);
        }

        else
        {
            a.push_back(temp);
            temp = 0;
        }
        if (i == (str.size() - 1))
        {
            a.push_back(temp);
        }
    }
    return a;
}

void simulation()
{
    while(true)
    {
        if(!req_queue.empty() && CURRENT_TIME == req_queue.front() -> arrival_time)
        {
            Request* req = req_queue.front();
            req_queue.pop_front();
            if(type != 'f')
                active_queue.push_back(req);
            else
                add_queue.push_back(req);            
        }
        if(active_req != nullptr && active_req -> track_id == CURRENT_TRACK)
        {
            active_req -> end_time = CURRENT_TIME;
            turnaround += active_req -> end_time - active_req -> arrival_time;
            waittime += active_req -> start_time - active_req -> arrival_time;
            max_waittime = max_waittime < (active_req -> start_time - active_req -> arrival_time)? (active_req -> start_time - active_req -> arrival_time) : max_waittime;
            active_req = nullptr;
        }
        if(active_req == nullptr)
        {
            if(req_queue.empty() && active_queue.empty() && add_queue.empty()) break;
            else if(!active_queue.empty() || !add_queue.empty())
            {
                active_req = scheduler -> select_req();
                active_req -> start_time = CURRENT_TIME;
                continue;
            }
        }
        if(active_req != nullptr)
        {
            direction = CURRENT_TRACK < active_req -> track_id? 1 : -1;
            CURRENT_TRACK += direction;
            tot_movement++;
        }
        CURRENT_TIME++;
    }
    return;
}

int main(int argc, char *argv[])
{
    int n;
    char c;
    while ((c = getopt(argc, argv, "s:qvf")) != -1)
    {
        switch (c)
        {
        case 'f':
            f_flag = true;
            break;
        case 's':
            sscanf(optarg, "%c", &type);
            break;
        case 'v':
            v_flag = true;
            break;
        case 'q':
            q_flag = true;
            break;
        case '?':
            printf("error optopt: %c \n", optopt);
            printf("error opterr: %c \n", opterr);
        default:
            exit(1);
            break;
        }
    }
    char *inputFilePath = argv[optind++];

    inputFile.open(inputFilePath);

    string res;
    res = getNextLine(inputFile);
    while(res != "eof")
    {
        vector<int> tmp;
        tmp = read_space_num(res);
        int time = tmp[0];
        int track = tmp[1];
        Request* tmp_req = new Request(time, track);
        req_queue.push_back(tmp_req);
        res = getNextLine(inputFile);
    }
    n = req_queue.size();
    deque<Request*> copy_queue;
    copy_queue = req_queue;

    switch (type)
    {
        case 'i':
            scheduler = new FIFO();
            break;
        case 'j':
            scheduler = new SSTF();
            break;
        case 's':
            scheduler = new LOOK();
            break;
        case 'c':
            scheduler = new CLOOK();
            break;
        case 'f':
            scheduler = new FLOOK();
            break;
        default:
            break;
    }


    simulation();

    for(int i = 0; i < n; i++)
    {
        printf("%5d: %5d %5d %5d\n",i, copy_queue[i]->arrival_time, copy_queue[i]->start_time, copy_queue[i]->end_time);
    }
    double avg_turnaround = turnaround / (double) n;
    double avg_waittime = waittime / (double) n;
    printf("SUM: %d %d %.2lf %.2lf %d\n", CURRENT_TIME, tot_movement, avg_turnaround, avg_waittime, max_waittime);


    return 0;
}