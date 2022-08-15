#include "class.cpp"
#include <cstring>

bool debug = false;
bool verbose = false;

sched_type schedType;

void print_ready_q(list<Process *> ready_q)
{
    if (!debug)
        return;
    printf("Ready Queue: \n");
    for (list<Process *>::iterator iter = ready_q.begin(); iter != ready_q.end(); ++iter)
    {
        printf("pid = <%d>; prio = <%d> ;\n", (*iter)->proc_id, (*iter)->dynamic_prio);
    }
    printf("\n");
}

void print_evt_q(list<Event *> event_queue)
{
    if (!debug)
        return;
    printf("Evt queue: \n");
    for (list<Event *>::iterator iter = event_queue.begin(); iter != event_queue.end(); ++iter)
    {
        printf("evt pid = <%d>, ts = <%d>, ", (*iter)->process->proc_id, (*iter)->time_stamp);
        if ((*iter)->state == CREATED)
            printf("state = CREATED");
        else if ((*iter)->state == TRANS_READY)
            printf("state = TRANS_TO_READY");
        else if ((*iter)->state == TRANS_RUNNING)
            printf("state = TRANS_TO_RUNNING");
        else if ((*iter)->state == TRANS_BLOCKED)
            printf("state = TRANS_TO_BLOCKED");
        else if ((*iter)->state == TRANS_PREEMPT)
            printf("state = TRANS_TO_PREEMPT");
        printf("\n");
    }
    printf("\n");
}

void put_event(int timestamp, Process *proc, trans state, list<Event *> &event_queue)
{
    Event *event = new Event(timestamp, proc, state);
    bool flag = false;
    list<Event *>::iterator it;
    for (it = event_queue.begin(); it != event_queue.end(); it++)
    {
        if (event->time_stamp >= (*it)->time_stamp)
            continue;
        else
        {
            event_queue.insert(it, event);
            flag = true;
            break;
        }
    }
    if (!flag)
        event_queue.push_back(event);
    return;
}

int get_random_number(int burst, int &ofs, vector<int> rand_vals)
{
    int total_vals = rand_vals[0];
    if (ofs > total_vals)
        ofs = 1;
    return 1 + (rand_vals[ofs++] % burst);
}

Event *get_event(list<Event *> &event_queue)
{
    if (event_queue.empty())
        return nullptr;
    Event *event = event_queue.front();
    event_queue.pop_front();
    return event;
}

int get_next_event_time(list<Event *> event_queue)
{
    if (event_queue.empty())
        return 0;
    int time = event_queue.front()->time_stamp;
    return time;
}

void simulation(list<Event *> event_queue, list<Process *> ready_queue, int ofs, vector<int> rand_vals, int quantum)
{
    string sched_algo;
    Scheduler *scheduler;
    switch (schedType)
    {
    case F:
    {
        sched_algo = "FCFS";
        scheduler = new FCFS();
        break;
    }
    case L:
    {
        sched_algo = "LCFS";
        scheduler = new LCFS();
        break;
    }

    case S:
    {
        sched_algo = "SRTF";
        scheduler = new SRTF();
        break;
    }

    case R:
    {
        sched_algo = "RR";
        scheduler = new RoundRobin();
        break;
    }

    case P:
    {
        sched_algo = "PRIO";
        scheduler = new Prio();
        break;
    }

    case E:
    {
        sched_algo = "PREPRIO";
        scheduler = new PrePrio();
        break;
    }

    default:
        break;
    }

    Event *evt;
    list<Event *> finished_processes;
    Process *current_process = nullptr;
    int current_time = 0;
    int block_cnt = 0, block_time = 0;
    int duration = 0, cb_cnt = 0;
    bool CALL_SCHEDULER = false;
    int cb, io;

    while ((evt = get_event(event_queue)))
    {
        //print_evt_q(event_queue);
        Process *proc = evt->process;
        duration = evt->time_stamp - current_time;
        current_time = evt->time_stamp;
        int state_time = current_time - proc->state_ts;
        if (block_cnt > 0)
        {
            block_time += duration;
        }
        if (debug)
        {
            printf("current time=%d, pid=%d, state ts=%d\n", current_time, proc->proc_id, proc->state_ts);
        }
        if (evt->state != TRANS_PREEMPT)
            proc->state_ts = current_time;

        if (evt->state == CREATED)
        {

            if (verbose)
                printf("%d %d %d: CREATED -> READY\n", evt->time_stamp, proc->proc_id, state_time);
            evt->state = TRANS_READY;
        }

        switch (evt->state)
        {
        case TRANS_READY:
        {
            if (proc->prev_state == TRANS_RUNNING)
            {
                current_process = nullptr;
                proc->TC = proc->TC - state_time;
                proc->current_cb -= state_time; // real running time
                proc->total_cb += state_time;
                if (verbose)
                {
                    printf("%d %d %d: RUNNG -> READY  cb=%d rem=%d prio=%d\n", evt->time_stamp, proc->proc_id,
                           state_time, proc->current_cb, proc->TC, proc->dynamic_prio);
                }
                if (schedType == P || schedType == E)
                    proc->dynamic_prio--;
            }
            if (proc->prev_state == TRANS_BLOCKED)
            {
                proc->current_cb = 0;
                if (verbose)
                    printf("%d %d %d: BLOCK -> READY\n", evt->time_stamp, proc->proc_id, state_time);
                block_cnt--;
                proc->dynamic_prio = proc->static_prio - 1;
            }
            if (schedType == E && current_process != nullptr)
            {
                if (current_process->dynamic_prio < proc->dynamic_prio)
                {
                    //e_sched_evt_queue(event_list, CURRENT_RUNNING_PROCESS, evt, current_time);
                    list<Event *>::iterator it;
                    bool is_preempt = true;

                    for (it = event_queue.begin(); it != event_queue.end();)
                    {
                        if ((*it)->time_stamp == current_time && current_process->proc_id == (*it)->process->proc_id)
                        {
                            is_preempt = false;
                            break;
                        }
                        else if ((*it)->time_stamp != current_time && current_process->proc_id == (*it)->process->proc_id)
                        {
                            it = event_queue.erase(it);
                        }
                        else
                            it++;
                    }
                    if (is_preempt)
                    {
                        put_event(current_time, current_process, TRANS_PREEMPT, event_queue);
                    }
                }
            }
            //proc->ready_start_time = current_time;
            proc->prev_state = TRANS_READY;
            scheduler->add_process(proc, ready_queue);
            //print_evt_q(event_queue);
            print_ready_q(ready_queue);

            CALL_SCHEDULER = true;
            break;
        }

        case TRANS_RUNNING:
        {
            if (proc->current_cb == 0)
                proc->current_cb = get_random_number(proc->get_CB(), ofs, rand_vals);
            if (proc->current_cb > proc->TC)
            {
                proc->current_cb = proc->TC;
            }
            if (verbose)
                printf("%d %d %d: READY -> RUNNG cb=%d rem=%d prio=%d\n", evt->time_stamp, proc->proc_id,
                       state_time, proc->current_cb, proc->TC, proc->dynamic_prio);
            int running_time = proc->current_cb;
            trans state = TRANS_BLOCKED;
            if (proc->current_cb > quantum)
            {
                running_time = quantum;
                state = TRANS_READY;
            }
            proc->CW += state_time;
            proc->prev_state = TRANS_RUNNING;
            put_event(current_time + running_time, proc, state, event_queue);
            break;
        }

        case TRANS_BLOCKED:
        {
            current_process = nullptr;
            if (proc->prev_state == TRANS_RUNNING)
            {
                proc->TC = proc->TC - state_time;
                proc->current_cb -= state_time; // real running time
                proc->total_cb += state_time;
            }
            CALL_SCHEDULER = true;
            proc->prev_state = TRANS_BLOCKED;

            if (proc->TC == 0)
            {
                if (verbose)
                    printf("%d %d %d: Done\n", current_time, proc->proc_id, proc->current_cb + state_time);
                proc->FT = current_time;
                proc->TT = current_time - proc->AT;
                put_event(proc->proc_id, proc, TRANS_READY, finished_processes);
            }
            else
            {
                block_cnt++;
                io = get_random_number(proc->get_IO(), ofs, rand_vals);
                proc->IT += io;
                if (verbose)
                    printf("%d %d %d: RUNNG -> BLOCK  ib=%d rem=%d\n", evt->time_stamp, proc->proc_id,
                           state_time, io, proc->TC);
                put_event(current_time + io, proc, TRANS_READY, event_queue);
            }
            break;
        }
        case TRANS_PREEMPT:
        {
            current_process = nullptr;
            //proc->current_cb = 0;
            //Event *new_evt = new Event(current_time, proc, TRANS_READY);
            //event_queue.push_front(new_evt);
            put_event(current_time, proc, TRANS_READY, event_queue);
            CALL_SCHEDULER = true;
            break;
        }

        default:
            break;
        }
        if (evt != nullptr)
        {
            delete (evt);
            evt = nullptr;
        }

        if (CALL_SCHEDULER)
        {
            if (get_next_event_time(event_queue) == current_time)
            {
                continue;
            }
            CALL_SCHEDULER = false;
            if (current_process == nullptr)
            {
                current_process = scheduler->get_next_process(ready_queue);
                if (current_process == nullptr)
                    continue;
                current_process->prev_state = TRANS_READY;
                put_event(current_time, current_process, TRANS_RUNNING, event_queue);
            }
        }
        print_evt_q(event_queue);
    }
    double total_cpu_util = 0, total_io_util = 0, total_tt = 0, total_cw = 0;
    int fin_time = 0, cnt = 0;
    list<Event *>::iterator it;
    if (quantum == 10000)
        cout << sched_algo << endl;
    else
        cout << sched_algo << ' ' << quantum << endl;
    for (it = finished_processes.begin(); it != finished_processes.end(); it++)
    {
        Process *proc = (*it)->process;
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
               cnt++, proc->AT, proc->static_TC, proc->CB, proc->IO,
               proc->static_prio, proc->FT, proc->TT, proc->IT, proc->CW);
        if (proc->FT > fin_time)
            fin_time = proc->FT;
        total_cpu_util += proc->total_cb;
        total_io_util += proc->IT;
        total_tt += proc->TT;
        total_cw += proc->CW;
    }
    int nums_processes = finished_processes.size();
    double throuput = 100.0 * (nums_processes / (double)fin_time);
    double cpu_util = 100.0 * (total_cpu_util / (double)fin_time);
    double io_util = 100.0 * (1.0 * block_time / (double)fin_time);

    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
           fin_time, cpu_util, io_util, total_tt / nums_processes, total_cw / nums_processes, throuput);
}

int main(int argc, char **argv)
{
    int arg_cnt = 1;
    int quantum = 0;
    int max_prio = 0;
    char type;
    Scheduler *scheduler;
    int option;
    string rand_ints_file_path = "./lab2_assign/rfile";
    string file_path = "./lab2_assign/input3";

    if (argc > 1)
    {
        while ((option = getopt(argc, argv, "vtes:")) != -1)
        {
            arg_cnt++;
            switch (option)
            {
            case 'v':
                verbose = true;
                break;
            case 't':
                debug = true;
                break;
            case 'e':
                debug = true;
                break;
            case 's':
            {
                quantum = 10000;
                max_prio = 4;

                int len = strlen(optarg);
                if (len == 1)
                {
                    switch (optarg[0])
                    {
                    case 'F':
                        schedType = F;
                        break;
                    case 'L':
                        schedType = L;
                        break;
                    case 'S':
                        schedType = S;
                        break;

                    default:
                        break;
                    }
                }
                else if (len > 1)
                {
                    string str_optarg = optarg;
                    if (str_optarg.find(":") == -1)
                    {
                        sscanf(optarg, "%c%d", &type, &quantum);
                    }
                    else
                    {
                        sscanf(optarg, "%c%d:%d", &type, &quantum, &max_prio);
                    }
                    switch (type)
                    {
                    case 'R':
                        schedType = R;
                        break;
                    case 'P':
                        schedType = P;
                        break;
                    case 'E':
                        schedType = E;
                        break;
                    default:
                        break;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
        rand_ints_file_path = argv[arg_cnt + 1];
        file_path = argv[arg_cnt];
    }
    else
    {
        verbose = true;
        schedType = E;
        quantum = 4;
        max_prio = 4;
    }

    int ofs = 1;
    int rand;
    vector<int> rand_vals;
    FILE *ptr1 = fopen(rand_ints_file_path.c_str(), "r");
    while (fscanf(ptr1, "%d", &rand) > 0)
        rand_vals.push_back(rand);

    int static_prio = 0;
    int AT, TC, CB, IO;
    int proc_num = 0;
    list<Process *> ready_queue;
    list<Event *> event_queue;
    list<Process *> proc_table;
    list<Event *> finished_processes;

    event_queue.clear();
    FILE *ptr2 = fopen(file_path.c_str(), "r");

    while (fscanf(ptr2, "%d %d %d %d", &AT, &TC, &CB, &IO) > 0)
    {
        static_prio = get_random_number(max_prio, ofs, rand_vals);
        Process *process = new Process(proc_num, AT, TC, CB, IO, static_prio);
        put_event(AT, process, CREATED, event_queue);
        proc_table.push_back(process);
        proc_num++;
    }
    simulation(event_queue, ready_queue, ofs, rand_vals, quantum);
}