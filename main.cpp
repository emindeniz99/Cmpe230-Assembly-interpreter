#include <stdio.h>
#include <bits/stdc++.h>
#include <iostream> // std::cout
#include <fstream>  // std::ifstream

using namespace std;

bool DebugMode = false; // to get real outputs, assign it false

// prototypes
template <class datatype>
void print_bits(datatype x);
template <class datatype>
void print_hex(datatype x);
void print_16bitregs();
void printflags();
void printmemo();
void printStack();
// short 16 bit
// char 8 bit

// global variables ( memory, registers and flags )
unsigned char memory[1 << 16]; // 64K memory
unsigned short ax = 0;
unsigned short bx = 0;
unsigned short cx = 0;
unsigned short dx = 0;

unsigned short di = 0;             // destination index
unsigned short si = 0;             // source index
unsigned short sp = (1 << 16) - 2; // stackpoint
unsigned short bp = 0;             // base pointer

unsigned short PC = 0; // Program Counter, it can be changed by Jump instructions

bool zf; // zero flag
bool sf; // sign flag
bool cf; // carry flag
bool af; // auxillary flag
bool of; // overflow flag

// initialize pointers
unsigned short *pax = &ax;
unsigned short *pbx = &bx;
unsigned short *pcx = &cx;
unsigned short *pdx = &dx;
unsigned short *pdi = &di;
unsigned short *psi = &si;
unsigned short *psp = &sp;
unsigned short *pbp = &bp;

// note that x86 uses little endian, that is, least significat byte is stored in lowest byte
unsigned char *pah = (unsigned char *)(((unsigned char *)&ax) + 1);
unsigned char *pal = (unsigned char *)&ax;
unsigned char *pbh = (unsigned char *)(((unsigned char *)&bx) + 1);
unsigned char *pbl = (unsigned char *)&bx;
unsigned char *pch = (unsigned char *)(((unsigned char *)&cx) + 1);
unsigned char *pcl = (unsigned char *)&cx;
unsigned char *pdh = (unsigned char *)(((unsigned char *)&dx) + 1);
unsigned char *pdl = (unsigned char *)&dx;

// labels <names, instruction index >
map<string, int> labels;

// instructions
vector<string> commands;

// variables <variable name, < type(db,dw), memory index> >
map<string, pair<string, int>> variables;

int variablestartpoint = 0; // it should be updated wrt insCounter, until int20h

// functions, you can find more info below

string strip(string &str);
void lowerCase(string &str);
bool mov(string a, string b);
void jnc(string a);
void jc(string a);
void jbe(string a);
void jnae(string a);
void jb(string a);
void jnb(string a);
void jae(string a);
void jnbe(string a);
void ja(string a);
void jnz(string a);
void jne(string a);
void jz(string a);
void je(string a);
bool cmp(string a, string b);
bool shr(string a, string b);
bool shl(string a, string b);
bool _not(string a);
bool _xor(string a, string b);
bool _or(string a, string b);
bool _and(string a, string b);
bool push(string operand);
bool pop(string operand);
bool rcr(string a, string b);
bool rcl(string a, string b);
bool _int(string operand);
bool mul(string operand);
bool div(string operand);
bool add(string a, string b);
bool sub(string a, string b);
int tointeger(string str); // 1205h -> 4555 , 'd'->24 str to int ascii....

// ERROR message
void error(string errormsg)
{
   cout << "Error" << endl;
   if (DebugMode) // in debug mode, it won't crash at error to get more info
   {
      cout << errormsg << endl;
   }
   else
   {
      exit(1); // comment it to avoid from stopping program
   }
}

int insCounter = -1; // wait until see "code segment block"
int main(int argc, char *argv[])
{
   // reads filename as a argument
   if (argv[1] == NULL)
   {
      cerr << "Please, give filename as an argument" << endl;
      return 0;
   }
   if (DebugMode)
   {
      cout << "Loaded filename: " << argv[1] << endl;
   }

   // ifstream file("./" + (string)argv[1]); // for filename in same directory
   ifstream file((string)argv[1]); // for full path

   if (file.peek() == EOF) // checks if file exists or not
   {
      cerr << "Your file isn't found. Please, give exists filename as an argument" << endl;
      return 0;
   }
   string line;

   bool vardefstart = false;

   // 1. Read lines of file
   while (getline(file, line, '\n'))
   {
      strip(line); // get rid of return carriage
      // ref: https://stackoverflow.com/questions/14295420/c-cout-overwriting-itself-while-in-for-loop

      if (line == "") // skip empty lines
         continue;

      if (line.find("code segment") != string::npos)
      {
         // cout << "code segment" << endl;
         insCounter = 0; // starts reading instructions
         continue;
      }

      // wait until code segment is seen
      if (insCounter < 0)
         continue;

      // break if code ends
      if (line.find("code ends") != string::npos)
      {
         // cout << "file reading is finished " << line.find("code ends") << endl;
         break;
      }

      if (line.find("int 20h") != string::npos)
      {
         // cout << "ins fin" << line.find("code ends") << endl;
         variablestartpoint = 6 * (insCounter + 2); // +2 due to own and next empty index of array
         vardefstart = true;
         continue;
      }

      // Variable parser
      if (vardefstart == true)
      {
         // cout << "var:" << line << endl;
         stringstream ls(line);     //linestream
         string varname, type, val; // name, dw or db , value
         ls >> varname >> type;
         getline(ls, val);
         strip(val);
         int intvalue = tointeger(val);

         if (variables.count(varname) > 0)
         {
            cout << "duplicated var name" << endl;
            exit(1);
         }

         // cout << "VariableAdress: (int)" << intvalue << endl;
         if (type == "db")
         {
            memory[variablestartpoint] = intvalue;

            variables[varname] = make_pair("db", variablestartpoint);

            // cout << "memo[" << variablestartpoint << "] = " << (0 + memory[variablestartpoint]) << endl;
            variablestartpoint++;
         }
         else if (type == "dw")
         {                                                  // https://piazza.com/class/k6aep8s1v8v50g?cid=65 top low memo
            memory[variablestartpoint] = intvalue;          // modulo 1<<8 is not necessary, memory is char arr, it casts
            memory[variablestartpoint + 1] = intvalue >> 8; // get upper via shift 8 bit right

            variables[varname] = make_pair("dw", variablestartpoint);

            // cout << "memo[" << variablestartpoint << "] = " << (0 + memory[variablestartpoint]) << endl;
            // cout << "memo[" << variablestartpoint + 1 << "] = " << (0 + memory[variablestartpoint + 1]) << endl;
            variablestartpoint += 2;
         }
         else
         {
            cout << "Error, wrong variable type" << endl;
            return 0; // break program, error
            break;
         }

         continue;
      }

      // create line stream to read line as "cin"
      stringstream ss(line); // MOV AX,[ 0abcdh ]
      string ins;            //name of instructions
      ss >> ins;             // MOV

      // check labels
      if (ins[ins.length() - 1] == ':')
      {
         if (labels.count(ins))
         {
            cerr << "Error occur when reading file" << endl;
            cerr << "Duplicated Label ! at\n"
                 << line << endl;
            return 0;
         }
         ins = ins.substr(0, ins.length() - 1);
         // cout << "label found index:"<<insCounter<< endl;
         labels[ins] = insCounter; // store index of label
      }
      // else if it is not label, they are instructions
      else
      {
         commands.push_back(line);
         insCounter++;
      }
   }

   // in developement stage, it shows current memory, variables, labels, etc.
   if (DebugMode)
   {
      // print instructions
      for (int i = 0; i < commands.size(); i++)
      {
         cout << "[" << i << "] " << commands[i] << endl;
      }
      map<string, int>::iterator it;

      cout << endl
           << "Labels:\n";
      for (it = labels.begin(); it != labels.end(); ++it)
         cout << it->first << " => " << it->second << '\n';
      cout << endl;

      map<string, pair<string, int>>::iterator it2;
      cout << "Variables:\n";
      for (it2 = variables.begin(); it2 != variables.end(); ++it2)
         cout << it2->first << " => " << it2->second.first << " [" << it2->second.second << "]" << '\n';
      cout << endl;

      cout << "Program starts execution" << endl;
   }

   // PC is like program counter (PC)
   // it process
   while (PC < commands.size())
   {
      if (DebugMode)
      {
         cout << endl
              << "line: " << PC << endl;
      }

      string currcmd = commands[PC];
      istringstream iss(currcmd);
      string ins;
      iss >> ins;
      strip(ins);

      lowerCase(ins);

      // check if there are one or two operand
      if (currcmd.find(",") != string::npos)
      {
         string first;
         string second;
         getline(iss, first, ',');
         getline(iss, second, ',');
         strip(first);  // first operand
         strip(second); // second operand

         if (DebugMode)
         {
            cout << "Ins: " << ins << " Dest: " << first << " Src: " << second << endl;
         }

         if (ins == "mov")
         {
            mov(first, second);
         }
         else if (ins == "cmp")
         {
            cmp(first, second);
         }
         else if (ins == "add")
         {
            add(first, second);
         }
         else if (ins == "sub")
         {
            sub(first, second);
         }
         else if (ins == "xor")
         {
            _xor(first, second);
         }
         else if (ins == "or")
         {
            _or(first, second);
         }
         else if (ins == "and")
         {
            _and(first, second);
         }
         else if (ins == "rcl")
         {
            rcl(first, second);
         }
         else if (ins == "rcr")
         {
            rcr(first, second);
         }
         else if (ins == "shl")
         {
            shl(first, second);
         }
         else if (ins == "shr")
         {
            shr(first, second);
         }
         else
         {
            cout << "Error" << endl;
            exit(1);
         }
      }
      else
      {
         string first;
         getline(iss, first, ',');
         strip(first);
         if (DebugMode)
         {
            cout << "Ins: " << ins << " Dest: " << first << endl;
         }
         if (ins == "push")
         {
            push(first);
         }
         else if (ins == "pop")
         {
            pop(first);
         }

         else if (ins == "jz")
         {
            jz(first);
         }
         else if (ins == "jnz")
         {
            jnz(first);
         }
         else if (ins == "je")
         {
            je(first);
         }
         else if (ins == "jne")
         {
            jne(first);
         }
         else if (ins == "ja")
         {
            ja(first);
         }
         else if (ins == "jae")
         {
            jae(first);
         }
         else if (ins == "jb")
         {
            jb(first);
         }
         else if (ins == "jbe")
         {
            jbe(first);
         }
         else if (ins == "jnae")
         {
            jnae(first);
         }
         else if (ins == "jnb")
         {
            jnb(first);
         }
         else if (ins == "jnbe")
         {
            jnbe(first);
         }
         else if (ins == "jnc")
         {
            jnc(first);
         }
         else if (ins == "jc")
         {
            jc(first);
         }
         else if (ins == "nop")
         {
            if (DebugMode)
            {
               cout << "NOP - continue" << endl;
            }
         }
         else if (ins == "not")
         {
            _not(first);
         }
         else if (ins == "int")
         {
            _int(first);
         }
         else if (ins == "mul")
         {
            mul(first);
         }
         else if (ins == "div")
         {
            div(first);
         }
         else if (ins == "inc")
         {
            add(first, "1");
         }
         else if (ins == "dec")
         {
            sub(first, "1");
         }
         else
         {
            cout << "Error" << endl;
            exit(1);
         }
      }

      if (DebugMode)
      {
         print_16bitregs();
         printmemo();
         printflags();
         printStack();
      }

      PC++;
   }

   cout << endl;

   if (DebugMode)
   {
      cout << endl
           << "registers:" << endl;
      print_16bitregs();

      cout << endl
           << "variable memo:" << endl;
      printmemo();
      cout << endl
           << "flags:" << endl;
      printflags();
      cout << endl
           << "stack:" << endl;
      printStack();
   }
}

// prints memory that variables exist
void printmemo()
{
   for (int i = 6 * (insCounter + 2); i < variablestartpoint; i++)
   {
      cout << "memo[" << i << "] : " << 0 + memory[i] << endl;
   }
}

void printflags()
{
   cout << "ZF: " << zf << endl;
   cout << "SF: " << sf << endl;
   cout << "CF: " << cf << endl;
   cout << "AF: " << af << endl;
   cout << "OF: " << of << endl;
}

void printStack()
{
   for (int i = (1 << 16) - 1; i > sp + 1; i--)
   {
      cout << "stack[" << i << "] : " << 0 + memory[i] << endl;
   }
}

template <class datatype>
void print_bits(datatype x)
{
   int i;

   for (i = 8 * sizeof(x) - 1; i >= 0; i--)
   {
      (x & (1 << i)) ? putchar('1') : putchar('0');
   }
   printf("\n");
}

template <class datatype>
void print_hex(datatype x)
{
   if (sizeof(x) == 1)
      printf("%02x\n", x);
   else
      printf("%04x\n", x);
}

void print_16bitregs()
{
   printf("AX:%04x %d\n", *pax, *pax);
   printf("BX:%04x %d\n", bx, bx);
   printf("CX:%04x %d\n", cx, cx);
   printf("DX:%04x %d\n", *pdx, *pdx);
   printf("DI:%04x %d\n", di, di);
   printf("SI:%04x %d\n", si, si);
   printf("SP:%04x %d\n", sp, sp);
   printf("BP:%04x %d\n", bp, bp);
}

// strip deletes whitespaces and \r from left side and right side
// https://stackoverflow.com/questions/83439/remove-spaces-from-stdstring-in-c
// https://en.cppreference.com/w/cpp/algorithm/remove
string strip(string &str)
{
   string newstr;
   int start = 0, end = str.length() - 1;
   for (int i = 0; i < str.length(); i++)
   {
      if (str[i] == ' ')
      {
         start++;
      }
      else
      {
         break;
      }
   }
   for (int i = str.length() - 1; i >= 0; i--)
   {
      if (str[i] == ' ' || str[i] == '\r' || str[i] == '\n')
      {
         end--;
      }
      else
      {
         break;
      }
   }
   if (end >= start)
   {
      str = str.substr(start, end - start + 1);
      newstr = str;
      return newstr;
   }

   else
   {
      str = "";
      newstr = str;
      return newstr;
   }
}

// convert parameter-string to lower cased
void lowerCase(string &str)
{
   for (int i = 0; i < str.length(); i++)
   {
      str[i] = tolower(str[i]);
   }
}

// string to decimal integer, string can be hex,dec, or pure numbers
int tointeger(string str)
{

   strip(str);
   // // IMPLEMENT      !!! TODO
   // if (str[0] == '0')
   // { // like hex
   //    //  01h 01 01d 01fh 01fdh
   //    if (str.length() ==)

   // }
   // else
   if (str[str.length() - 1] == 'h')
   { // hex
      return stoi(str.substr(0, str.length() - 1), nullptr, 16);
   }
   else if (str[str.length() - 1] == 'd')
   { //digit
      return stoi(str.substr(0, str.length() - 1), nullptr, 10);
   }
   else if (str[str.length() - 1] == 'b')
   { //binary , olmayacakmış

      return stoi(str.substr(0, str.length() - 1), nullptr, 2);
   }
   else if ((str[str.length() - 1] == 34 && str[0] == 34) || (str[str.length() - 1] == 39 && str[0] == 39))
   {                                                //34 "  ,39 '
      string val = str.substr(1, str.length() - 2); // if val= "as"
      int ans = 0;                                  // = a*256 +s
      for (int i = 0; i < val.length(); i++)
      {
         ans *= 256;
         ans += val[i];
      }
      return ans;
   }
   else
   {
      return stoi(str);
   }
}

// to get easily reference of registers from string
map<string, unsigned short *> converter16bit = {
    {"ax", pax},
    {"bx", pbx},
    {"cx", pcx},
    {"dx", pdx},
    {"di", pdi},
    {"si", psi},
    {"sp", psp},
    {"bp", pbp},
};

// to get easily reference of registers from string
map<string, unsigned char *> converter8bit = {
    {"al", pal},
    {"ah", pah},
    {"bl", pbl},
    {"bh", pbh},
    {"cl", pcl},
    {"ch", pch},
    {"dl", pdl},
    {"dh", pdh}};

// returns bit number of operand type
int bitnumberof(string operand)
{
   strip(operand); // strip whitespaces from left and right side
   string temp = operand;
   lowerCase(temp); // w [ 1235h ]
   string bwsiztemp = temp.substr(1, temp.length() - 1);
   strip(bwsiztemp); // [ 1325h ]
   if (converter16bit.count(temp))
   {
      return 16;
   }
   else if (converter8bit.count(temp))
   {
      return 8;
   }
   else if (temp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return 8;
   }
   else if (temp[0] == 'b' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return 8;
   }
   else if (temp[0] == 'w' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return 16;
   }
   else if (temp.find("offset") == 0)
   {
      return 16;
   }
   else if (variables.count(temp))
   {
      return variables[temp].first == "db" ? 8 : 16;
   }
   else
   {
      return tointeger(operand) > (1 << 8) ? 16 : 8; // i am not sure about immediate values
   }
}

// returns "reg" "memory" "offset" "offset" "dw" "db" "value"
string typeofoperand(string operand)
{
   strip(operand);
   string temp = operand;
   lowerCase(temp); // w [ 1235h ]
   string bwsiztemp = temp.substr(1, temp.length() - 1);
   strip(bwsiztemp); // [ 1325h ]
   if (converter16bit.count(temp) || converter8bit.count(temp))
   {
      return "reg";
   }
   else if (temp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return "memory";
   }
   else if (temp[0] == 'b' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return "memory";
   }
   else if (temp[0] == 'w' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      return "memory";
   }
   else if (temp.find("offset") == 0)
   {
      return "offset";
   }
   else if (variables.count(temp))
   {
      return variables[temp].first; // "dw"   or "db"
   }
   else
   {
      return "value";
   }
}

//returns integer value of operand, usually used for right operand
int getValue(string operand)
{
   strip(operand);                                       // operand is pure version of input
   string temp = operand;                                // temp is lower cased version
   lowerCase(temp);                                      // w [ 1235h ]
   string bwsiztemp = temp.substr(1, temp.length() - 1); // bwsiz is cutted first char to look b,w
   strip(bwsiztemp);                                     // [ 1325h ]
   if (converter16bit.count(temp))
   {
      return *(converter16bit[temp]);
   }
   else if (converter8bit.count(temp))
   {
      return *(converter8bit[temp]);
   }
   else if (temp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = temp.substr(1, temp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      return memory[inval];
   }
   else if (temp[0] == 'b' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = bwsiztemp.substr(1, bwsiztemp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      return memory[inval];
   }
   else if (temp[0] == 'w' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = bwsiztemp.substr(1, bwsiztemp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      // cout << "debug: " << memory[inval + 1] * (1 << 8) << "   " << 0 + memory[inval] << endl;
      return memory[inval + 1] * (1 << 8) + memory[inval];
   }
   else if (temp.find("offset") == 0)
   {
      string tt = operand.substr(6, temp.length() - 6); // use operand due to variable can be UPPERCASE
      strip(tt);
      return variables[tt].second;
   }
   else if (variables.count(temp))
   {
      if (variables[temp].first == "db")
      {
         return memory[variables[temp].second];
      }
      else if (variables[temp].first == "dw")
      {
         return memory[variables[temp].second + 1] * (1 << 8) + memory[variables[temp].second];
      }
      else
      {
         error("at getValue ");
      }
   }
   else
   {
      if (tointeger(operand) > (1 << 16) - 1)
      {
         error("too big number");
      }
      return tointeger(operand);
   }
   return true;
}

// return reference of memory to use easily for assignment etc.
unsigned char &getMemoRef(string operand)
{
   strip(operand);                                       // operand is pure version of input
   string temp = operand;                                // temp is lower cased version
   lowerCase(temp);                                      // w [ 1235h ]
   string bwsiztemp = temp.substr(1, temp.length() - 1); // bwsiz is cutted first char to look b,w
   strip(bwsiztemp);                                     // [ 1325h ]
   if (temp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = temp.substr(1, temp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      return memory[inval];
   }
   else if (temp[0] == 'b' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = bwsiztemp.substr(1, bwsiztemp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      return memory[inval];
   }
   else if (temp[0] == 'w' && bwsiztemp[0] == '[' && temp[temp.length() - 1] == ']')
   {
      string innerstr = bwsiztemp.substr(1, bwsiztemp.length() - 2);
      strip(innerstr);
      int inval = getValue(innerstr);
      // cout << "debug: " << memory[inval + 1] * (1 << 8) << "   " << 0 + memory[inval] << endl;
      return memory[inval];
   }
   else
   {
      error("ERROR wrong memory referance");
   }
   return memory[0];
}

// MOV ins
bool mov(string dest, string src) //https://stackoverflow.com/questions/4088387/how-to-store-pointers-in-map/4088449
{
   if (DebugMode)
   {
      cout << "[in mov ] value - bit - type" << endl
           << getValue(dest) << " " << bitnumberof(dest) << "  " << typeofoperand(dest) << endl
           << getValue(src) << "   " << bitnumberof(src) << "  " << typeofoperand(src) << endl;
   }

   string typedest = typeofoperand(dest);
   string typesrc = typeofoperand(src);

   int bitdest = bitnumberof(dest);
   int bitsrc = bitnumberof(src);

   if (typedest == "reg")
   {
      strip(dest);

      if (typesrc == "value" || typesrc == "offset")
      { // bit independent, so no need compare bit sizes

         if (bitdest == 16)
         {
            *(converter16bit[dest]) = getValue(src);
         }

         else if (bitdest == 8)
         {
            if (getValue(src) < 1 << 8)
               *(converter8bit[dest]) = getValue(src);
            else
            {
               error("error big number to 1 byte");
            }
         }
         else
         {
            error("error");
         }
      }
      else
      {

         if (bitdest == 16 && bitsrc == 16)
         {
            *(converter16bit[dest]) = getValue(src);
         }

         else if (bitdest == 8 && bitsrc == 8)
         {
            *(converter8bit[dest]) = getValue(src);
         }
         else
         {
            error("error");
         }
      }
   }
   else if (typedest == "memory")
   {

      if (typesrc == "value" || typesrc == "offset")
      { // bit önemsiz olanlar

         if (bitdest == 16) // dw
         {
            if (getValue(src) < 1 << 16)
            {
               getMemoRef(dest) = getValue(src);
               *(&getMemoRef(dest) + 1) = getValue(src) >> 8;
            }
            else
            {
               error("error big number to 1 byte");
            }
         }

         else if (bitdest == 8) // db
         {
            if (getValue(src) < 1 << 8)
               getMemoRef(dest) = getValue(src);
            else
            {
               error("error big number to 1 byte");
            }
         }
         else
         {
            error("wrong bit dest");
         }
      }
      else
      {

         if (bitdest == 16 && bitsrc == 16)
         {
            getMemoRef(dest) = getValue(src);
            // cout<<"asdas: "<<0+&getMemoRef(dest)<<endl;
            *(&getMemoRef(dest) + 1) = getValue(src) >> 8;
         }

         else if (bitdest == 8 && bitsrc == 8)
         {
            getMemoRef(dest) = getValue(src);
         }
         else
         {
            error("bit problems at memory ref");
         }
      }
   }
   else if (typedest == "db" || typedest == "dw")
   {
      if (typedest == "db")
      {
         getMemoRef("b[offset" + dest + "]") = getValue(src);
      }
      else if (typedest == "dw")
      {
         getMemoRef("w[offset" + dest + "]") = getValue(src);
         *(&getMemoRef("w[offset" + dest + "]") + 1) = getValue(src) >> 8;
      }
      else
      {
         error("bir şeyler oldu");
      }
   }
   else
   {

      error("ERROR wrong type of dest" + typedest);
   }
   return true;
}

// PUSH
bool push(string operand)
{

   string type = typeofoperand(operand);
   int value = getValue(operand);
   if (type == "value" || type == "offset")
   {
      getMemoRef("w[" + to_string((int)sp) + "]") = value;
      getMemoRef("w[" + to_string((int)sp + 1) + "]") = value >> 8;
   }
   else
   {
      if (bitnumberof(operand) == 16)
      {
         getMemoRef("w[" + to_string((int)sp) + "]") = value;
         getMemoRef("w[" + to_string((int)sp + 1) + "]") = value >> 8;
      }
      else
      {
         error("not valid bit size !=16");
         return false;
      }
   }

   sp -= 2;
   return true;
}

// POP
bool pop(string operand)
{

   string value = to_string(getValue("w[" + to_string((int)sp + 2) + "]"));
   if (DebugMode)
   {
      cout << "pop val:" << value << endl;
   }

   if (bitnumberof(operand) == 16)
   {
      mov(operand, value);
   }
   else
   {
      error("not valid bit size !=16");
      return false;
   }
   sp += 2;
   return true;
}

// AND
bool _and(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      mov(a, to_string(getValue(a) & getValue(b)));
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         mov(a, to_string(getValue(a) & getValue(b)));
      }
   }
   return true;
}

// OR
bool _or(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      mov(a, to_string(getValue(a) | getValue(b)));
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         mov(a, to_string(getValue(a) | getValue(b)));
      }
   }
   return true;
}

// XOR
bool _xor(string a, string b)
{
   if (getValue(a) ^ getValue(b) == 0)
   {
      zf = 1;
   }
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      mov(a, to_string(getValue(a) ^ getValue(b)));
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         mov(a, to_string(getValue(a) ^ getValue(b)));
      }
   }

   return true;
}

// NOT
bool _not(string a)
{

   mov(a, to_string(~getValue(a)));
   return true;
}

// SHL
bool shl(string a, string b)
{
   int temp = getValue(a) << getValue(b);
   mov(a, to_string(temp % (1 << bitnumberof(a))));
   cf = (temp / ((1 << bitnumberof(a)))) % 2;
   return true;
}

// SHR
bool shr(string a, string b)
{
   int temp = getValue(a) >> getValue(b);
   mov(a, to_string(temp % (1 << bitnumberof(a))));
   cf = (temp >> (bitnumberof(a) - 1)) % 2;
   return true;
}

// CMP
bool cmp(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      if (getValue(a) == getValue(b))
      {
         zf = true;
         cf = false;
      }
      else if (getValue(a) < getValue(b))
      {
         zf = false;
         cf = true;
      }
      else if (getValue(a) > getValue(b))
      {
         zf = false;
         cf = false;
      }
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         if (getValue(a) == getValue(b))
         {
            zf = true;
            cf = false;
         }
         else if (getValue(a) < getValue(b))
         {
            zf = false;
            cf = true;
         }
         else if (getValue(a) > getValue(b))
         {
            zf = false;
            cf = false;
         }
      }
   }
   return true;
}

//If some specified condition is satisfied in conditional jump, the control flow is transferred to a target instruction. There are numerous conditional jump instructions depending upon the condition and data.

//Following are the conditional jump instructions used on signed data used for arithmetic operations − (Signed)

//JE     //signed and unsigned      //Jump equal
void je(string a)
{
   if (zf == true)
   { //Check ZF Flag
      PC = labels[strip(a)] - 1;
   }
}

//JZ      //signed and unsigned      //Jump zero
void jz(string a)
{
   if (zf == true)
   { //Check ZF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNE       //signed and unsigned        //Jump not equal
void jne(string a)
{
   if (zf == false)
   { //Check ZF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNZ       //signed and unsigned        //Jump not zero
void jnz(string a)
{
   if (zf == false)
   { //Check ZF Flag
      PC = labels[strip(a)] - 1;
   }
}

//Following are the conditional jump instructions used on unsigned data used for logical operations − (Unsigned)
//JNBE      //unsigned data used        //Jump Not Below/Equal
void jnbe(string a)

{
   if (cf == false && zf == false)
   { //Check CF AND ZF Flag
      PC = labels[strip(a)] - 1;
   }
}

//JA        //unsigned data used        //Jump Above
void ja(string a)
{
   if (cf == false && zf == false)
   { //Check CF AND ZF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JAE                                   //Jump Above/Equal
void jae(string a)
{
   if (cf == false)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNB      //unsigned data used         //Jump Not Below
void jnb(string a)
{
   if (cf == false)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

//JB        //unsigned data used        //Jump Below
void jb(string a)
{
   if (cf == true)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNAE      //unsigned data used        //Jump Not Above/Equal
void jnae(string a)
{
   if (cf == true)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

//JBE        //unsigned data used        //Jump Below/Equa
void jbe(string a)
{
   if (cf == true || zf == true)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

// http://faydoc.tripod.com/cpu/ja.htm
//The following conditional jump instructions have special uses and check the value of flags

//JC      //special      //Jump if carry
void jc(string a)
{
   if (cf == true)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNC       //special       //Jump if no carry
void jnc(string a)
{
   if (cf == false)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

// RCL
bool rcl(string a, string b)
{
   int bit = bitnumberof(a);
   int val = getValue(b);
   int va = getValue(a);
   mov(a, to_string(((va << (val)) % (1 << bit)) | (va << (val - 1)) / (1 << bit) | (cf * (1 << (val - 1)))));
   cf = (va / ((1 << (bitnumberof(a) - getValue(b))))) % 2;
   return true;
}

// RCR
bool rcr(string a, string b)
{
   int bit = bitnumberof(a);
   int val = getValue(b);
   int va = getValue(a);
   mov(a, to_string(((va) % (1 << (val - 1))) * (1 << (bit - val + 1)) + cf * (1 << (bit - val)) + (va >> val))); // kontrol et !
   cf = (va >> (bitnumberof(b) - 1)) % 2;
   return true;
}

// INT
bool _int(string operand)
{
   strip(operand);
   if (operand == "21h")
   {
      if (getValue("ah") == 1)
      {
         char temp;
         cin >> temp;
         if (DebugMode)
         {
            cout << "input(ascii dec no):" << to_string(temp) << endl;
         }

         *(converter8bit["al"]) = temp;
      }
      else if (getValue("ah") == 2)
      {
         if (DebugMode)
         {
            cout << "output:";
         }
         cout << (char)getValue("dl") << flush;
         if (DebugMode)
         {
            cout << endl;
         }
      }
      else
      {
         error("what does int 21h do?");
      }
   }
   else if (operand == "20h")
   {
      if (DebugMode)
      {
         cout << "int20h stop" << endl;
      }
      exit(1);
   }
   else
   {
      error("wrong int 21h parameter");
      return false;
   }
   return true;
}

// MUL
bool mul(string operand)
{
   if (typeofoperand(operand) == "reg" || typeofoperand(operand) == "memo")
   {
      if (bitnumberof(operand) == 8)
      {
         int val = getValue("al") * getValue(operand);
         if (val >= (1 << 8))
         {
            of = true;
            cf = true;
         }
         mov("ax", to_string(val));
      }
      else if (bitnumberof(operand) == 16)
      {
         int val = getValue("ax") * getValue(operand);
         if (DebugMode)
         {
            cout << "mul val:" << val << endl;
         }
         int valdx = 0, valax = 0;
         if (val >= (1 << 16))
         {
            of = true;
            cf = true;
         }
         valdx = val / (1 << 16);
         valax = val % (1 << 16);
         mov("ax", to_string(valax));
         mov("dx", to_string(valdx));
      }
   }
   else
   {
      error("wrong type operand");
   }
   return true;
}

// DIV
bool div(string operand)
{
   if (typeofoperand(operand) == "reg" || typeofoperand(operand) == "memo")
   {
      if (getValue(operand) == 0)
      {
         error("divide by 0 :D");
      }
      if (bitnumberof(operand) == 8)
      {
         int valal = getValue("ax") / getValue(operand);
         int valah = getValue("ax") % getValue(operand);
         mov("al", to_string(valal));
         mov("ah", to_string(valah));
      }
      else if (bitnumberof(operand) == 16)
      {
         int base = getValue("dx") * (1 << 16) + getValue("ax");
         int valax = base / getValue(operand);
         int valdx = base % getValue(operand);
         if (valax > ((1 << 16) - 1))
         {
            error("Div error, you cannot store the quotient in ax because  there aren't enough bits");
         }
         mov("ax", to_string(valax));
         mov("dx", to_string(valdx));
      }
   }
   else
   {
      error("wrong type operand");
   }
   return true;
}

//Register to register
//Memory to register
//Register to memory
//Register to constant data
//Memory to constant data
bool add(string a, string b)
{
   string typeA = typeofoperand(a);
   string typeB = typeofoperand(b);
   int bitA = bitnumberof(a);
   int bitB = bitnumberof(b);
   if (typeA == "reg" && typeB == "reg")
   {
      strip(a);
      strip(b);

      if (bitA == bitB)
      {
         int temp = getValue(a) + getValue(b);

         // maybe temp can be high value
         if (temp > (1 << bitA))
         {
            cf = 1;
         }
         else
         {
            cf = 0;
         }

         mov(a, to_string(temp % (1 << bitA)));
      }
      else
      {
         //error bit number is not appropriate
         error("bit number is not appropriate");
      }
   }
   else if (typeA == "memory" && typeB == "reg")
   {
      if (bitA == 16 && bitB == 16)
      {
         int temp = getValue(a) + getValue(b);
         mov(a, to_string(temp));
      }
      else if (bitA == 8 && bitB == 8)
      {
         int temp = getValue(a) + getValue(b);
         mov(a, to_string(temp));
      }
      else
      {
         error("bit problems");
      }
   }
   else if (typeA == "reg" && typeB == "memory")
   {
      if (bitA == 16 && bitB == 16)
      {
         int temp = getValue(a) + getValue(b);
         mov(a, to_string(temp));
      }
      else if (bitA == 8 && bitB == 8)
      {
         int temp = getValue(a) + getValue(b);
         mov(a, to_string(temp));
      }
      else
      {
         error("bit problems");
      }
   }
   else if (typeA == "reg" && typeB == "value")
   {
      strip(a);
      int temp = getValue(a) + getValue(b);
      // maybe temp can be high value !!!
      mov(a, to_string(temp));
   }
   else if (typeA == "memory" && typeB == "value")
   {
      if (bitA == 16)
      {
         if (getValue(b) < 1 << 16)
         {
            //getValue(a) can be change
            int temp = getValue(a) + getValue(b);
            mov(a, to_string(temp));
         }
         else
         {
            error("");
         }
      }
      else if (bitA == 8)
      {
         if (getValue(b) < 1 << 8)
         {
            int temp = getValue(a) + getValue(b);
            mov(a, to_string(temp));
         }
         else
         {
            error("");
         }
      }
   }
   else
   {
      error("");
   }
   return true;
}

// SUB
bool sub(string a, string b)
{
   string typeA = typeofoperand(a);
   string typeB = typeofoperand(b);
   int bitA = bitnumberof(a);
   int bitB = bitnumberof(b);
   if (typeA == "reg" && typeB == "reg")
   {
      strip(a);
      strip(b);
      if (bitA == bitB)
      {
         int temp = getValue(a) - getValue(b);
         if (temp < 0)
         {
            //negative value operations
            mov(a, to_string((1 << bitA) + temp));

            sf = 1;
            cf = 1;
         }
         else
         {
            mov(a, to_string(temp));
         }
      }
      else
      {
         error("");
      }
   }
   else if (typeA == "memory" && typeB == "reg")
   {
      if (bitA == 16 && bitB == 16)
      {
         int temp = getValue(a) - getValue(b);
         if (temp < 0)
         {
            mov(a, to_string((1 << bitA) + temp));
            sf = 1;
            cf = 1;
         }
         else
         {
            mov(a, to_string(temp));
         }
      }
      else if (bitA == 8 && bitB == 8)
      {
         int temp = getValue(a) - getValue(b);
         if (temp < 0)
         {
            mov(a, to_string((1 << bitA) + temp));

            sf = 1;
            cf = 1;
         }
         else
         {
            mov(a, to_string(temp));
         }
      }
      else
      {
         error("bit problems");
      }
   }
   else if (typeA == "reg" && typeB == "memory")
   {
      if (bitA == 16 && bitB == 16)
      {
         int temp = getValue(a) - getValue(b);
         if (temp < 0)
         {
            mov(a, to_string((1 << bitA) + temp));

            sf = 1;
            cf = 1;
         }
         else
         {
            mov(a, to_string(temp));
         }
      }
      else if (bitA == 8 && bitB == 8)
      {
         int temp = getValue(a) - getValue(b);
         if (temp < 0)
         {
            mov(a, to_string((1 << bitA) + temp));

            sf = 1;
            cf = 1;
         }
         else
         {
            mov(a, to_string(temp));
         }
      }
      else
      {
         error("bit problems");
      }
   }
   else if (typeA == "reg" && typeB == "value")
   {
      strip(a);
      int temp = getValue(a) - getValue(b);
      if (temp < 0)
      {
         mov(a, to_string((1 << bitA) + temp));

         sf = 1;
         cf = 1;
      }
      else
      {
         mov(a, to_string(temp));
      }
   }
   else if (typeA == "memory" && typeB == "value")
   {
      if (bitA == 16)
      {
         if (getValue(b) < 1 << 16)
         {
            //getValue(a) can be change
            int temp = getValue(a) - getValue(b);
            if (temp < 0)
            {
               mov(a, to_string((1 << bitA) + temp));

               sf = 1;
               cf = 1;
            }
            else
            {
               mov(a, to_string(temp));
            }
         }
         else
         {
            error("");
         }
      }
      else if (bitA == 8)
      {
         if (getValue(b) < 1 << 8)
         {
            int temp = getValue(a) - getValue(b);
            if (temp < 0)
            {
               mov(a, to_string((1 << bitA) + temp));
               sf = 1;
               cf = 1;
            }
            else
            {
               mov(a, to_string(temp));
            }
         }
         else
         {
            error("");
         }
      }
   }
   else if (typeA == "dw" || typeA == "db")
   {
      if ((getValue(a) - getValue(b) < 0))
      {
         cf = 1;
         sf = 1;
      }
      else if ((getValue(a) - getValue(b)) == 0)
      {
         zf = 1;
      }
      if (typeB == "value" || typeB == "offset")
      {
         mov(a, to_string(getValue(a) - getValue(b)));
      }
      else
      {
         if (bitA == bitB)
         {
            mov(a, to_string(getValue(a) - getValue(b)));
         }
         else
         {
            error("");
         }
      }
   }
   else
   {
      error("");
   }
   return true;
}

// NOTES
// dont confuse 1<<8 and 2<<8
// when write val > (1<<16) you forget equality case