#include "header.h"

int CURRENT_TIME = 0;
int CURRENT_TRACK = 0;
int direction = 1; // 1 is the pos


class Request{
    public:
    int arrival_time;
    int track_id;
    int start_time;
    int end_time;
    Request(int timestamp, int track)
    {
        arrival_time = timestamp;
        track_id = track;
    }
};

Request* active_req = nullptr;
deque<Request*> req_queue;
deque<Request*> active_queue;
deque<Request*> add_queue; // special for FLOOK

class Scheduler
{
    public:
    virtual Request* select_req() = 0;
};

class FIFO: public Scheduler
{
    Request* select_req()
    {
        if(active_queue.empty()) return nullptr;
        else
        {
            Request* req = active_queue.front();
            active_queue.pop_front();
            return req;
        }
    }
};

class SSTF: public Scheduler
{
    Request* select_req()
    {
        if(active_queue.empty()) return nullptr;
        else
        {
            int min_dist = INT_MAX;
            int ptr;
            for(int i = 0; i < active_queue.size(); i++)
            {
                int dist = abs(active_queue[i] -> track_id - CURRENT_TRACK);
                if(dist < min_dist)
                {
                    min_dist = dist;
                    ptr = i;
                }
            }
            Request* req = active_queue[ptr];
            active_queue.erase(active_queue.begin() + ptr);
            return req;
        }
    }
};

class LOOK: public Scheduler
{
    Request* select_req()
    {
        if(active_queue.empty()) return nullptr;
        else{
            int ptr;
            int min_dist = INT_MAX;
            for(int i = 0; i < active_queue.size(); i++)
            {
                int dist = active_queue[i] -> track_id - CURRENT_TRACK;
                if(direction == 1 && dist >= 0)
                {
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        ptr = i;
                    }
                }
                if(direction == -1 && dist <= 0)
                {
                    dist = -1 * dist;
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        ptr = i;
                    }
                }
            }
            if(min_dist == INT_MAX)
            {
                direction *= -1;
                return this -> select_req();
            }
            Request* req = active_queue[ptr];
            active_queue.erase(active_queue.begin() + ptr);
            return req;
        }
    }
    
};

class CLOOK: public Scheduler
{
    Request* select_req()
    {
        if(active_queue.empty()) return nullptr;
        else
        {
            int ptr;
           int min_dist = INT_MAX;
            for(int i = 0; i < active_queue.size(); i++)
            {
                int dist = active_queue[i] -> track_id - CURRENT_TRACK;
                if(dist >= 0)
                {
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        ptr = i;
                    }
                }
            }
            if(min_dist == INT_MAX)
            {
                for(int i = 0; i < active_queue.size(); i++)
                {
                   int dist = active_queue[i] -> track_id;
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        ptr =i;
                    }
                }
            }
            Request* req = active_queue[ptr];
            active_queue.erase(active_queue.begin() + ptr);
            return req;
        }
    }
};

class FLOOK: public Scheduler
{
    Request* select_req()
    {
        if(active_queue.empty())
        {
            swap(add_queue, active_queue);
        }

        if(active_queue.empty())
        {
            return nullptr;
        }

        else
        {
            int ptr;
            int min_dist = INT_MAX;
            for(int i = 0; i < active_queue.size(); i++)
            {
                int dist = active_queue[i] -> track_id - CURRENT_TRACK;
                if(direction == 1 && dist >= 0)
                {
                    if(dist < min_dist)
                    {
                        min_dist = dist;
                        ptr = i;
                    }
                }
                if(direction == -1 && dist <= 0)
                {
                dist = -1 * dist;
                    if(dist < min_dist)
                   {   
                        min_dist = dist;
                        ptr = i;
                    }
                }
            }
            if(min_dist == INT_MAX)
            {
                direction *= -1;
                return this -> select_req();
            }
            Request* req = active_queue[ptr];
            active_queue.erase(active_queue.begin() + ptr);
            return req;
        }
    }
};