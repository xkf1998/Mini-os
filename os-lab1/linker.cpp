#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <map>
#include <vector>

using namespace std;

//global pos variable
int linenum = 1;
int lineoffset = 1;
int startnum, startoffset, emptynum, emptyoffset;

//input file
ifstream inputFile;

void __parseerror(int errcode)
{
    static string errstr[] = {
        "NUM_EXPECTED",           // Number expect, anything >= 2^30 is not a number either
        "SYM_EXPECTED",           // Symbol Expected
        "ADDR_EXPECTED",          // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };
    printf("Parse Error line %d offset %d: %s\n", startnum, startoffset, errstr[errcode].c_str());
    exit(0);
}

bool is_delimiter(char c)
{
    if (c == ' ' || c == '\t' || c == '\n')
        return true;
    else
        return false;
}

string readWord()
{
    char c;
    string res = "";
    bool flag = false;
    if (inputFile.is_open())
    {
        while (inputFile.get(c))
        {
            if (is_delimiter(c))
            {
                emptynum = linenum;
                emptyoffset = lineoffset;
                if (c == '\n')
                {
                    linenum += 1;
                    lineoffset = 1;
                }
                else
                    lineoffset += 1;

                if (flag)
                {
                    break;
                }
            }
            else
            {
                res += c;
                if (!flag)
                {
                    flag = true;
                    startnum = linenum;
                    startoffset = lineoffset;
                }
                lineoffset += 1;
            }
        }
    }
    else
    {
        return res;
    }
    if (inputFile.eof())
    {
        inputFile.close();
        if (res == "")
        {
            startnum = emptynum;
            startoffset = emptyoffset;
        }
    }
    // cout << res << ' ' << startnum << ' ' << startoffset << endl;
    return res;
}

string readIAER()
{
    string s = readWord();
    if (s.length() != 1 || (s[0] != 'I' && s[0] != 'A' && s[0] != 'E' && s[0] != 'R'))
        __parseerror(2);
    return s;
}

string readSym()
{
    string s = readWord();
    if (s.length() > 16)
        __parseerror(3);
    if (!isalpha(s[0]))
        __parseerror(1);
    for (int i = 1; i < s.length(); i++)
    {
        if (!isalnum(s[i]))
            __parseerror(1);
    }
    return s;
}

int readInt()
{
    string s = readWord();
    if (s.length() == 0)
        return 0;
    for (int i = 0; i < s.length(); i++)
    {
        if (!isdigit(s[i]))
        {
            __parseerror(0);
        }
    }
    long long res = stoll(s);
    if (res >= pow(2, 30))
    {
        __parseerror(0);
    }
    return (int)res;
}

string convertIdx(int index)
{
    string res = to_string(index);
    while (res.length() < 3)
        res = "0" + res;
    return res;
}

map<string, int> Pass1()
{
    map<string, int> symbol_table;
    vector<string> arr;
    int instr_cnt = 0;
    int module_cnt = 0;
    map<string, bool> redefine_map;
    while (!inputFile.eof())
    {
        vector<string> tmp_arr;
        module_cnt++;
        int defcount = readInt();
        if (defcount > 16)
            __parseerror(4);
        for (int i = 0; i < defcount; i++)
        {
            string sym = readSym();
            int val = readInt();
            if (symbol_table.find(sym) == symbol_table.end())
            {
                symbol_table[sym] = val + instr_cnt;
                arr.push_back(sym);
                tmp_arr.push_back(sym);
                redefine_map[sym] = false;
            }
            //error raise
            else
                redefine_map[sym] = true;
        }
        int usecount = readInt();
        if (usecount > 16)
            __parseerror(5);
        for (int i = 0; i < usecount; i++)
        {
            string sym = readSym();
        }
        int instcount = readInt();
        instr_cnt += instcount;
        for (int i = 0; i < tmp_arr.size(); i++)
        {
            if (symbol_table[tmp_arr[i]] > instr_cnt - 1)
            {
                printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", module_cnt, tmp_arr[i].c_str(), symbol_table[tmp_arr[i]] + instcount - instr_cnt, instcount - 1);
                symbol_table[tmp_arr[i]] = instr_cnt - instcount;
            }
        }
        if (instr_cnt > 512)
            __parseerror(6);
        for (int i = 0; i < instcount; i++)
        {
            string addressmode = readIAER();
            int operand = readInt();
        }
    }
    cout << "Symbol Table" << endl;
    for (int i = 0; i < arr.size(); i++)
    {
        cout << arr[i] << "=" << symbol_table[arr[i]];
        if (redefine_map[arr[i]])
            cout << ' ' << "Error: This variable is multiple times defined; first value used" << endl;
        else
            cout << endl;
    }
    cout << endl;
    return symbol_table;
}

void Pass2(map<string, int> symbol_table)
{
    int instr_cnt = 0;
    int module_cnt = 0;
    int instr_index = 0;
    vector<string> arr;
    map<string, int> sym_used_global;
    cout << "Memory Map" << endl;
    while (!inputFile.eof())
    {
        map<string, int> sym_used_local;
        vector<string> pos_sym(256);
        vector<string> tmp_arr;
        int pos_sym_size = 0;
        module_cnt++;
        int defcount = readInt();
        for (int i = 0; i < defcount; i++)
        {
            string sym = readSym();
            int val = readInt();
            if (sym_used_global.find(sym) == sym_used_global.end())
                arr.push_back(sym);
            sym_used_global[sym] = module_cnt;
        }
        int usecount = readInt();
        for (int i = 0; i < usecount; i++)
        {
            string sym = readSym();
            pos_sym[i] = sym;
            pos_sym_size++;
            if (sym_used_local.find(sym) == sym_used_local.end())
                tmp_arr.push_back(sym);
            sym_used_local[sym] = 1;
        }
        int instcount = readInt();

        for (int i = 0; i < instcount; i++)
        {
            string addressmode = readIAER();
            int operand = readInt();
            int opcode = operand / 1000;
            operand = operand % 1000;
            bool op_err = false;
            if (opcode >= 10)
                op_err = true;
            if (addressmode == "I")
            {
                bool flag = false;
                string command = to_string(opcode) + convertIdx(operand);
                if (stoi(command) >= 10000)
                {
                    flag = true;
                    command = to_string(9999);
                }
                string index = convertIdx(instr_index) + ": ";
                cout << index << command;
                if (flag)
                    cout << " Error: Illegal immediate value; treated as 9999";
                cout << endl;
            }
            else if (addressmode == "A")
            {
                if (op_err)
                {
                    string command = to_string(9999);
                    string index = convertIdx(instr_index) + ": ";
                    cout << index << command << " Error: Illegal opcode; treated as 9999";
                    cout << endl;
                }
                else
                {
                    bool flag = false;
                    if (operand > 512)
                    {
                        operand = 0;
                        flag = true;
                    }
                    string command = to_string(opcode) + convertIdx(operand);
                    string index = convertIdx(instr_index) + ": ";
                    cout << index << command;
                    if (flag)
                        cout << ' ' << "Error: Absolute address exceeds machine size; zero used";
                    cout << endl;
                }
            }
            else if (addressmode == "R")
            {
                if (op_err)
                {
                    string command = to_string(9999);
                    string index = convertIdx(instr_index) + ": ";
                    cout << index << command << " Error: Illegal opcode; treated as 9999";
                    cout << endl;
                }
                else
                {
                    bool flag = false;
                    if (operand > instcount - 1)
                    {
                        operand = 0;
                        flag = true;
                    }
                    string command = to_string(opcode) + convertIdx(operand + instr_cnt);
                    string index = convertIdx(instr_index) + ": ";
                    cout << index << command;
                    if (flag)
                        cout << ' ' << "Error: Relative address exceeds module size; zero used";
                    cout << endl;
                }
            }
            else if (addressmode == "E")
            {
                if (op_err)
                {
                    string command = to_string(9999);
                    string index = convertIdx(instr_index) + ": ";
                    cout << index << command << " Error: Illegal opcode; treated as 9999";
                    cout << endl;
                }
                else
                {
                    if (operand >= pos_sym_size)
                    {
                        bool flag = false;
                        string command = to_string(opcode) + convertIdx(operand);
                        if (stoi(command) >= 10000)
                        {
                            flag = true;
                            command = to_string(9999);
                        }
                        string index = convertIdx(instr_index) + ": ";
                        cout << index << command << " Error: External address exceeds length of uselist; treated as immediate";
                        if (flag)
                            cout << " Error: Illegal immediate value; treated as 9999";
                        cout << endl;
                    }
                    else
                    {
                        string tmp_sym = pos_sym[operand];
                        int address;
                        bool sym_not_find = false;
                        if (symbol_table.find(tmp_sym) == symbol_table.end())
                        {
                            address = 0;
                            sym_not_find = true;
                            sym_used_local[tmp_sym] = 0;
                        }
                        else
                        {
                            address = symbol_table[tmp_sym];
                            sym_used_global[tmp_sym] = 0;
                            sym_used_local[tmp_sym] = 0;
                        }
                        string command = to_string(opcode) + convertIdx(address);
                        string index = convertIdx(instr_index) + ": ";
                        cout << index << command;
                        if (sym_not_find)
                            printf(" Error: %s is not defined; zero used", tmp_sym.c_str());
                        cout << endl;
                    }
                }
            }
            instr_index++;
        }
        for (int i = 0; i < tmp_arr.size(); i++)
        {
            if (sym_used_local[tmp_arr[i]] != 0)
            {
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", module_cnt, tmp_arr[i].c_str());
            }
        }
        instr_cnt += instcount;
    }
    bool warning_flag = true;
    for (int i = 0; i < arr.size(); i++)
    {
        string sym = arr[i];
        if (warning_flag)
        {
            cout << endl;
            warning_flag = false;
        }
        if (sym_used_global[sym] != 0)
            printf("Warning: Module %d: %s was defined but never used\n", sym_used_global[sym], sym.c_str());
    }
    return;
}

int main(int argc, char *argv[])
{
    string path = argv[1];
    path = "./" + path;
    inputFile.open(path.c_str(), ios::in);
    map<string, int> sym_table = Pass1();
    if (inputFile.is_open())
        inputFile.close();
    inputFile.open(path.c_str(), ios::in);
    Pass2(sym_table);
    if (inputFile.is_open())
        inputFile.close();
    return (0);
}