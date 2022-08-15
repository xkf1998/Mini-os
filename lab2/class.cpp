#include "header.h"

class Process
{
public:
    int proc_id;
    int AT, TC, CB, IO; // arrival time, total cpu time, cpu burst, io burst
    int static_prio;
    int FT, TT, CW, IT;
    int total_cb, state_ts;
    //int ready_start_time;
    int static_TC, current_cb, dynamic_prio;
    trans prev_state;
    Process(int proc_id, int AT, int TC, int CB, int IO, int prio)
    {
        FT = 0;
        TT = 0;
        CW = 0;
        IT = 0;
        total_cb = 0;
        state_ts = 0;
        prev_state = CREATED;
        this->proc_id = proc_id;
        this->AT = AT;
        this->TC = TC;
        this->CB = CB;
        this->IO = IO;
        this->static_prio = prio;
        dynamic_prio = static_prio - 1;
        static_TC = TC;
        state_ts = AT; // start timestamp of a state trans
        current_cb = 0;
    }
    int get_AT() { return AT; }
    int get_TC() { return TC; }
    int get_CB() { return CB; }
    int get_IO() { return IO; }
};

class Event
{
public:
    int time_stamp;
    Process *process;
    trans state, prev_state;
    Event(int ts, Process *process, trans state)
    {
        this->time_stamp = ts;
        this->process = process;
        this->state = state;
    }
};

class Scheduler
{
public:
    virtual Process *get_next_process(list<Process *> &ready_queue)
    {
        if (ready_queue.empty())
            return nullptr;
        Process *proc = ready_queue.front();
        ready_queue.pop_front();
        return proc;
    }
    virtual void add_process(Process *proc, list<Process *> &ready_queue)
    {
        return;
    }
    virtual bool test_preempt(Process *proc, int curtime, list<Process *> &ready_queue)
    {
        return false;
    }
};

class FCFS : public Scheduler
{
public:
    void add_process(Process *proc, list<Process *> &ready_queue)
    {
        ready_queue.push_back(proc);
    }
};

class LCFS : public Scheduler
{
public:
    void add_process(Process *proc, list<Process *> &ready_queue)
    {
        ready_queue.push_front(proc);
    }
};

class SRTF : public Scheduler
{
public:
    void add_process(Process *proc, list<Process *> &ready_queue)
    {
        bool flag = false;
        list<Process *>::iterator it;
        for (it = ready_queue.begin(); it != ready_queue.end(); it++)
        {
            if (proc->TC >= (*it)->TC)
                continue;
            else
            {
                ready_queue.insert(it, proc);
                flag = true;
                break;
            }
        }
        if (!flag)
        {
            ready_queue.insert(ready_queue.end(), proc);
        }
    }
};

class RoundRobin : public Scheduler
{
public:
    void add_process(Process *proc, list<Process *> &ready_queue)
    {
        ready_queue.push_back(proc);
    }
};

class Prio : public Scheduler
{
public:
    list<Process *> expired;
    void add_process(Process *proc, list<Process *> &active)
    {
        if (proc->dynamic_prio == -1)
        {
            proc->dynamic_prio = proc->static_prio - 1;
            list<Process *>::iterator it;
            for (it = expired.begin(); it != expired.end(); it++)
            {
                if (proc->dynamic_prio > (*it)->dynamic_prio)
                {
                    expired.insert(it, proc);
                    return;
                }
            }
            expired.insert(expired.end(), proc);
            return;
        }
        else
        {
            list<Process *>::iterator it;
            for (it = active.begin(); it != active.end(); it++)
            {
                if (proc->dynamic_prio > (*it)->dynamic_prio)
                {
                    active.insert(it, proc);
                    return;
                }
            }
            active.insert(active.end(), proc);
            return;
        }
    }
    Process *get_next_process(list<Process *> &active)
    {
        if (active.empty())
            active.swap(expired);
        if (active.empty())
            return nullptr;
        Process *proc = active.front();
        active.pop_front();
        return proc;
    }
};

class PrePrio : public Scheduler
{
public:
    list<Process *> expired;
    void add_process(Process *proc, list<Process *> &active)
    {
        if (proc->dynamic_prio == -1)
        {
            proc->dynamic_prio = proc->static_prio - 1;
            list<Process *>::iterator it;
            for (it = expired.begin(); it != expired.end(); it++)
            {
                if (proc->dynamic_prio > (*it)->dynamic_prio)
                {
                    expired.insert(it, proc);
                    return;
                }
            }
            expired.insert(expired.end(), proc);
            return;
        }
        else
        {
            list<Process *>::iterator it;
            for (it = active.begin(); it != active.end(); it++)
            {
                if (proc->dynamic_prio > (*it)->dynamic_prio)
                {
                    active.insert(it, proc);
                    return;
                }
            }
            active.insert(active.end(), proc);
            return;
        }
    }
    Process *get_next_process(list<Process *> &active)
    {
        if (active.empty())
            active.swap(expired);
        if (active.empty())
            return nullptr;
        Process *proc = active.front();
        active.pop_front();
        return proc;
    }
};