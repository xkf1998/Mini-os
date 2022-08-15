#include "class.cpp"
#include <cstring>

queue<frame_t *> freeFrameList;
Process *current_process = nullptr;

ifstream inputFile;
ifstream randomFile; // two file

bool O_flag = false, P_flag = false, F_flag = false, S_flag = false;

unsigned long context_switch = 0, process_exit = 0;
unsigned long long cost = 0;
unsigned long reads = 0, writes = 0;

Pager *pager = nullptr;

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

frame_t *allocate_frame_from_free_list()
{
    if (freeFrameList.empty())
        return nullptr;
    else
    {
        frame_t *res = freeFrameList.front();
        freeFrameList.pop();
        return res;
    }
}

frame_t *get_frame()
{
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == nullptr)
        frame = pager->select_victim_frame();
    return frame;
}

void free_list_init()
{
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        frame_table[i] = FTE(i, -1, -1);
        freeFrameList.push(&frame_table[i]);
    }
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

bool get_next_instruction(char &operation, int &vpage, ifstream &inputFile)
{
    string res = getNextLine(inputFile);
    if (res == "eof") //end of the file and terminate simulation
        return false;
    operation = res[0];
    string num;
    if (res.length() > 2)
        num = res.substr(2);
    if (num != "")
        vpage = stoi(num);
    return true;
}

void resetFrame(int frameid)
{
    frame_table[frameid].age = 0;
    frame_table[frameid].processId = -1;
    frame_table[frameid].ProcPgId = -1;
    freeFrameList.push(&frame_table[frameid]);
    return;
}
void unmapPTE(Process *proc, int pageTableId)
{
    proc->unmaps++;
    if (O_flag)
        cout << " UNMAP " << proc->pid << ":" << pageTableId << endl;
    cost += unmapsCost;
    proc->page_table[pageTableId].frameid = 0;
    proc->page_table[pageTableId].PRESENT = 0;
    return;
}

bool checkVmaValid(Process *proc, int pageNum)
{
    if (proc->page_table[pageNum].vmaValid == 1)
    {
        return true;
    } // faster way to check vma valid
    for (int i = 0; i < proc->vma_size; i++)
    {
        if (proc->vma_table[i].starting_virtual_page <= pageNum && proc->vma_table[i].ending_virtual_page >= pageNum)
        {
            proc->page_table[pageNum].WRITE_PROTECT = proc->vma_table[i].write_protected;
            proc->page_table[pageNum].FILEMAPPED = proc->vma_table[i].file_mapped;
            proc->page_table[pageNum].vmaValid = 1;
            return true;
        }
    }
    return false;
}

bool pagefaultHandler(Process *proc, int pageNum)
{
    bool vma_res = checkVmaValid(proc, pageNum); // check if vma valid and set related bits in the first time
    if (!vma_res)
    {
        proc->segv++;
        cost += segvCost;
        if (O_flag)
        {
            cout << " SEGV" << endl;
        }
        return false;
    }
    frame_t *frame = get_frame();
    frame->age = 0;
    frame->lastVisitTime = currInstr;
    if (frame->processId != -1 && frame->ProcPgId != -1)
    {
        Process *old_process = process_vector[frame->processId];
        int oldPageNum = frame->ProcPgId;
        unmapPTE(old_process, oldPageNum);
        if (old_process->page_table[oldPageNum].MODIFIED == 1 && old_process->page_table[oldPageNum].FILEMAPPED == 0)
        {
            old_process->page_table[oldPageNum].PAGEDOUT = 1;
            old_process->outs++;
            cost += outsCost;
            if (O_flag)
                cout << " OUT" << endl;
        }
        if (old_process->page_table[oldPageNum].MODIFIED == 1 && old_process->page_table[oldPageNum].FILEMAPPED == 1)
        {
            old_process->fouts++;
            cost += foutsCost;
            if (O_flag)
                cout << " FOUT" << endl;
        }
        old_process->page_table[oldPageNum].MODIFIED = 0;
        old_process->page_table[oldPageNum].REFERENCED = 0;
    }

    //initialize pte
    proc->page_table[pageNum].PRESENT = 1;
    proc->page_table[pageNum].frameid = frame->phyFrameNum;
    proc->page_table[pageNum].MODIFIED = 0;
    proc->page_table[pageNum].REFERENCED = 0;

    //set frame property
    frame->processId = proc->pid;
    frame->ProcPgId = pageNum;

    if (proc->page_table[pageNum].FILEMAPPED)
    {
        proc->fins++;
        cost += finsCost;
        if (O_flag)
        {
            cout << " FIN" << endl;
        }
    }
    else if (proc->page_table[pageNum].PAGEDOUT)
    {
        proc->ins++;
        cost += insCost;
        if (O_flag)
        {
            cout << " IN" << endl;
        }
    }
    else
    {
        proc->zeros++;
        cost += zerosCost;
        if (O_flag)
        {
            cout << " ZERO" << endl;
        }
    }
    if (O_flag)
        cout << " MAP " << frame->phyFrameNum << endl;
    proc->maps++;
    cost += mapsCost;
    return true;
}

void simulation()
{
    char operation;
    int vpage;
    while (get_next_instruction(operation, vpage, inputFile))
    {
        currInstr++;
        if (O_flag)
            cout << currInstr - 1 << ": ==> " << operation << " " << vpage << endl;
        //handle c and e special cases
        if (operation == 'c')
        {
            current_process = process_vector[vpage];
            context_switch++;
            cost += context_switchCost;
            continue;
        }
        if (operation == 'e')
        {
            process_exit++;
            cost += process_exitCost;
            if (O_flag)
                cout << "EXIT current process " << current_process->pid << endl;
            for (int i = 0; i < MAX_VPAGES; i++)
            {
                if (current_process->page_table[i].PRESENT == 1)
                {
                    int frameid = current_process->page_table[i].frameid;
                    unmapPTE(current_process, i);
                    if (current_process->page_table[i].FILEMAPPED && current_process->page_table[i].MODIFIED)
                    {
                        current_process->fouts++;
                        cost += foutsCost;
                        if (O_flag)
                            cout << " FOUT" << endl;
                    }
                    resetFrame(frameid);
                }
                current_process->page_table[i].MODIFIED = 0;
                current_process->page_table[i].REFERENCED = 0;
                current_process->page_table[i].PAGEDOUT = 0;
                current_process->page_table[i].FILEMAPPED = 0;
            }
            current_process = nullptr;
            continue;
        }
        if (operation == 'r')
        {
            reads += 1;
            cost += 1;
            if (current_process->page_table[vpage].PRESENT == 0)
            {
                bool res = pagefaultHandler(current_process, vpage);
                if (!res)
                    continue;
            } // page fault
            current_process->page_table[vpage].REFERENCED = 1;
            continue;
        }
        if (operation == 'w')
        {
            writes += 1;
            cost += 1;
            if (current_process->page_table[vpage].PRESENT == 0)
            {
                bool res = pagefaultHandler(current_process, vpage);
                if (!res)
                    continue;
            } //page fault

            if (current_process->page_table[vpage].WRITE_PROTECT == 1)
            {
                current_process->page_table[vpage].REFERENCED = 1;
                current_process->segprot++;
                cost += segprotCost;
                if (O_flag)
                {
                    cout << " SEGPROT" << endl;
                }
                continue;
            }
            current_process->page_table[vpage].REFERENCED = 1;
            current_process->page_table[vpage].MODIFIED = 1;
            continue;
        }

        //cout << operation << vpage << endl;
    }
    return;
}

void printP()
{
    for (int i = 0; i < process_vector.size(); i++)
    {
        cout << "PT[" << i << "]"
             << ":";
        for (int j = 0; j < MAX_VPAGES; j++)
        {
            if (process_vector[i]->page_table[j].PRESENT)
            {
                cout << " " << j << ":";
                if (process_vector[i]->page_table[j].REFERENCED)
                    cout << "R";
                else
                    cout << "-";

                if (process_vector[i]->page_table[j].MODIFIED)
                    cout << "M";
                else
                    cout << "-";
                if (process_vector[i]->page_table[j].PAGEDOUT)
                    cout << "S";
                else
                    cout << "-";
            }
            else if (process_vector[i]->page_table[j].PAGEDOUT)
                cout << " #";
            else
                cout << " *";
        }
        cout << endl;
    }
}

void printF()
{
    cout << "FT:";
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        cout << " ";
        if (frame_table[i].processId == -1 || frame_table->ProcPgId == -1)
            cout << "*";
        else
        {
            cout << frame_table[i].processId << ":" << frame_table[i].ProcPgId;
        }
    }
    cout << endl;
}

void printS()
{
    for (int i = 0; i < process_vector.size(); i++)
    {
        Process *proc = process_vector[i];
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
               proc->pid,
               proc->unmaps, proc->maps, proc->ins, proc->outs,
               proc->fins, proc->fouts, proc->zeros,
               proc->segv, proc->segprot);
    }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
           currInstr, context_switch, process_exit, cost, sizeof(pte_t));
}

int main(int argc, char *argv[])
{
    char type;
    int c;
    while ((c = getopt(argc, argv, "f:a:o:")) != -1)
    {
        switch (c)
        {
        case 'f':
            sscanf(optarg, "%d", &MAX_FRAMES);
            break;
        case 'a':
            sscanf(optarg, "%c", &type);
            break;
        case 'o':
            if (optarg[0] == 'O')
                O_flag = true;
            if (optarg[1] == 'P')
                P_flag = true;
            if (optarg[2] == 'F')
                F_flag = true;
            if (optarg[3] == 'S')
                S_flag = true;
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
    char *randomFilePath = argv[optind];

    inputFile.open(inputFilePath);

    string res;

    res = getNextLine(inputFile);
    int n;
    if (res != "")
        n = stoi(res);
    for (int i = 0; i < n; i++)
    {
        Process *proc = new Process(i);
        process_vector.push_back(proc);

        //init the page table of that process
        for (int j = 0; j < MAX_VPAGES; j++)
        {
            proc->page_table[j] = PTE();
        }
        res = getNextLine(inputFile);
        int numVmas;
        if (res != "")
            numVmas = stoi(res);
        int s, e, w, f;
        proc->vma_size = numVmas;
        //init vma table
        for (int j = 0; j < numVmas; j++)
        {
            res = getNextLine(inputFile);
            vector<int> tmp;
            tmp = read_space_num(res);
            s = tmp[0];
            e = tmp[1];
            w = tmp[2];
            f = tmp[3];
            //cout << s << ' ' << e << ' ' << w << ' ' << f << endl;
            vma_t vma(s, e, w, f);
            proc->vma_table[j] = vma;
        }

        //cout << endl;
    } //read the basic setting of process, not reading intrs

    randomFile.open(randomFilePath);

    res = getNextLine(randomFile); // number of random value in that file
    if (res != "")
        n = stoi(res);
    for (int i = 0; i < n; i++)
    {
        res = getNextLine(randomFile);
        int tmp;
        if (res != "")
            tmp = stoi(res);
        randvals.push_back(tmp);
    }
    randomFile.close();

    switch (type)
    {
    case 'f':
        pager = new FIFO();
        break;
    case 'c':
        pager = new CLOCK();
        break;
    case 'a':
        pager = new AGING();
        break;
    case 'e':
        pager = new NRU();
        break;
    case 'r':
        pager = new RANDOM();
        break;
    case 'w':
        pager = new WORKINGSET();
        break;
    default:
        break;
    }

    free_list_init();
    simulation();

    if (P_flag)
        printP();
    if (F_flag)
        printF();
    if (S_flag)
        printS();

    return 0;
}