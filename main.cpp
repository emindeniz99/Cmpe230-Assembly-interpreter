#include <stdio.h>
#include <bits/stdc++.h>
#include <iostream> // std::cout
#include <fstream>  // std::ifstream

using namespace std;

// prototypes
template <class datatype>
void print_bits(datatype x);
template <class datatype>
void print_hex(datatype x);
template <class regtype>
void mov_reg_reg(regtype *preg1, regtype *preg2);
void print_16bitregs();
void printflags();
void printmemo();

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
unsigned short bp = 0;             // base pointer ? usually 0

unsigned short PC = 0;

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

map<string, int> labels;

vector<string> commands;

map<string, pair<string, int>> variables; // myvar,type,memory ref, myvar,db/dw,65456 index

int variablestartpoint = 0; // it should be updated wrt sayac, until int20h

// functions
string strip(string &str);
void lowerCase(string &str);

bool mov(string a, string b);
bool add(string a, string b);
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

int tointeger(string str); // 1205h -> 4555 , 'd'->24 str to int ascii....

// ERROR message
void error(string errormsg)
{
   cout << "ERROR: " << errormsg << endl;
   //  exit(1);   // comment it to avoid from stopping program
}

int sayac = -1; // !
int main(int argc, char *argv[])
{
   // *pax = 1;
   // *pbx = 7;
   // *pah = 3;

   // 1. Read instruction lines

   if (argv[1] == NULL)
   {
      cerr << "Please, give filename as an argument" << endl;
      return 0;
   }
   cout << "Loaded filename: " << argv[1] << endl;
   ifstream file("./" + (string)argv[1]);

   if (file.peek() == EOF) // checks if file exists or not
   {
      cerr << "Your file isn't found. Please, give exists filename as an argument" << endl;
      return 0;
   }
   string line;

   bool vardefstart = false;

   while (getline(file, line, '\n'))
   {
      strip(line); // get rid of return carriage
      // ref: https://stackoverflow.com/questions/14295420/c-cout-overwriting-itself-while-in-for-loop

      if (line == "") // skip empty lines
         continue;

      if (line.find("code segment") != string::npos)
      {
         // cout << "code segment" << endl;
         sayac = 0;
         continue;
      }

      // wait until code segment is seen
      if (sayac < 0)
         continue;

      // break if code ends
      if (line.find("code ends") != string::npos)
      {
         // cout << "bitti" << line.find("code ends") << endl;
         break;
      }

      if (line.find("int 20h") != string::npos)
      {
         // cout << "bitti" << line.find("code ends") << endl;
         variablestartpoint = 6 * (sayac + 2); // +2 due to own and next empty index of array
         vardefstart = true;
         continue;
      }

      // Variable parser
      if (vardefstart == true)
      {
         // cout << "var:" << line << endl;
         stringstream ls(line);     //linestream
         string varname, type, val; // name and dw or db
         // char value;
         ls >> varname >> type;
         getline(ls, val);
         strip(val);
         int asd = tointeger(val);

         if (variables.count(varname) > 0)
         {
            cout << "duplicated var name" << endl;
            exit(1);
         }

         // cout << "VariableAdress: (int)" << asd << endl;
         if (type == "db")
         {
            memory[variablestartpoint] = asd;

            variables[varname] = make_pair("db", variablestartpoint);

            // cout << "memo[" << variablestartpoint << "] = " << (0 + memory[variablestartpoint]) << endl;
            variablestartpoint++;
         }
         else if (type == "dw")
         { // https://piazza.com/class/k6aep8s1v8v50g?cid=65 top low memo
            memory[variablestartpoint] = asd;
            memory[variablestartpoint + 1] = asd >> 8; // get upper via shift 8 bit right

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

      stringstream ss(line); // MOV AX,[ 0abcdh ]
      string ins;
      ss >> ins; // MOV

      string delimiter = ":";
      if (ins[ins.length() - 1] == ':')
      {
         if (labels.count(ins))
         {
            cerr << "Error occur when reading file" << endl;
            cerr << "Duplicated Label ! at\n"
                 << line << endl;
            return 0;
         }
         ins=ins.substr(0,ins.length()-1);
         // cout << "label buldum "<< endl;
         labels[ins] = sayac;
      }
      else
      {
         commands.push_back(line);
      }

      sayac++;

      // vector<string> result;
      // while (ss.good()) // AX,[ 0abcdh ]
      // {
      //    string substr;
      //    getline(ss, substr, ' ');
      //    cout<<substr;
      //    result.push_back(substr);
      // }

      // istringstream iss(line);
      // string result;
      // if (getline(iss, result, '='))
      // {
      //    if (result == "foo")
      //    {
      //       string token;
      //       while (getline(iss, token, ','))
      //       {
      //          cout << token << endl;
      //       }
      //    }
      //    else
      //    {
      //       // ...
      //    }
      // }
   }

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
   while (PC != commands.size())
   {
      cout << endl
           << "line: " << PC << endl;
      string currcmd = commands[PC];
      istringstream iss(currcmd);
      string ins;
      iss >> ins;
      strip(ins);

      lowerCase(ins);

      if (currcmd.find(",") != string::npos)
      {
         string first;
         string second;
         getline(iss, first, ',');
         getline(iss, second, ',');
         // iss >> second;
         strip(first);
         strip(second);
         cout << "Ins: " << ins << " Dest: " << first << " Src: " << second << endl;

         if (ins == "mov")
         {
            mov(first, second); // ax  ,bx    ax,01h
            // break;
         }
        else if (ins == "cmp")
         {
            cmp(first,second);
         }
         // else if(ins=="add"){
         // add(first,second);
         // }
         // else if(ins=="sub"){
         //    sub(first,second);
         // }
         // else if(ins=="inc"){
         //    inc(first,second);
         // }
         // else if(ins=="dec"){
         //    dec(first,second);
         // }
         // else if(ins=="mul"){
         //    mul(first,second);
         // }
         // else if(ins=="div"){
         //    div(first,second);
         // }
         // else if (ins == "xor")
         // {
         //    _xor(first, second);
         // }
         // else if (ins == "or")
         // {
         //    _or(first, second);
         // }
         // else if (ins == "and")
         // {
         //    _and(first, second);
         // }
         // else if (ins == "not")
         // {
         //    _not(first);
         // }
         // else if(ins=="rcl"){
         //    rcl(first,second);
         // }
         // else if(ins=="rcr"){
         //    rcr(first,second);
         // }
         // else if (ins == "shl")
         // {
         //    shl(first, second);
         // }
         // else if (ins == "shr")
         // {
         //    shr(first, second);
         // }
         // else if(ins=="push"){
         //    push(first,second);
         // }
         // else if(ins=="pop"){
         //    pop(first,second);
         // }
         // else if(ins=="nop"){
         //    nop(first,second);
         // }
         // else if(ins=="push"){
         //    push(first,second);
         // }
         // else if(ins=="pop"){
         //    pop(first,second);
         // }
         // else if(ins=="int20h"){
         //    int20h(first,second);
         // }
      }
      else
      {
         string first;
         getline(iss, first, ',');
         strip(first);
         cout << "1: " << ins << " Par1: " << first << endl;

          
         if (ins == "jz")
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
      }

      // vector<string> result;
      // while (ss.good()) // AX,[ 0abcdh ]
      // {
      //    string substr;
      //    getline(ss, substr, ' ');
      //    cout<<substr;
      //    result.push_back(substr);
      // }
      print_16bitregs();
      printmemo();
   printflags();

      PC++;
   }

   // 2. Place  dw and db data  and compute addresses

   // 3.  PC = line 0
   // while (PC !=  int20line) {
   //     getline(PC)
   //     parse instruction in the line  to extract opcode and operands
   //     if (strcmp(opcode,"ADD) {
   //        .... determine operands and call appropriate add function
   //     }
   //     else if (strcmp(opcode,"MOV") ) {
   //         .. determine operands call  appropriate mov function
   // mov_reg_reg(pax, pbx);
   // *pax = 50000;
   //     }
   //
   // }
   cout << endl
        << "registers:" << endl;
   print_16bitregs();

   cout << endl
        << "variable memo:" << endl;
   printmemo();
   cout<<endl<< "flags:"<<endl;
   printflags();
   // print_hex(*pah);
   // print_hex(*pal);
}

void printmemo()
{
   for (int i = 6 * (sayac + 2); i < variablestartpoint; i++)
   {
      cout << "memo[" << i << "] : " << 0 + memory[i] << endl;
   }
}


void printflags(){

cout<<"ZF: "<<zf<<endl;
cout<<"SF: "<<sf<<endl;
cout<<"CF: "<<cf<<endl;
cout<<"AF: "<<af<<endl;
cout<<"OF: "<<of<<endl;
}

template <class regtype>
bool mov_reg_reg(regtype *preg1, regtype *preg2)
{
   *preg1 = *preg2;
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

// https://stackoverflow.com/questions/83439/remove-spaces-from-stdstring-in-c
// https://en.cppreference.com/w/cpp/algorithm/remove
// strip deletes spaces according to left side and right side
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

void lowerCase(string &str)
{
   for (int i = 0; i < str.length(); i++)
   {
      str[i] = tolower(str[i]);
   }
}
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

// si di is not working, change them to pointer
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

map<string, unsigned char *> converter8bit = {
    {"al", pal},
    {"ah", pah},
    {"bl", pbl},
    {"bh", pbh},
    {"cl", pcl},
    {"ch", pch},
    {"dl", pdl},
    {"dh", pdh}};

int bitnumberof(string operand)
{
   strip(operand);
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
   //   if(  )   TODO add imme,offst,...
   {
      return tointeger(operand) > (2 << 8) ? 16 : 8; // i am not sure about immediate values
   }

   // reg
   // offset
   // value  b w
   // memory
   // immediate
   // variable
}

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
   // else if(temp.find("offset")!=string::npos){
   //    return   "offset";
   // }
   else if (temp.find("offset") == 0)
   {
      return "offset";
   }
   else if (variables.count(temp))
   {
      return variables[temp].first;
   }
   //   if(  )   TODO add imme,offst,...
   else
   {
      return "value";
   }

   // reg
   // offset
   // value  b w
   // memory
   // immediate
}

//returns integer value of operand, usally used for right operand
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
   else if (temp[0] == '[' && temp[temp.length() - 1] == ']') // bu da bir alttakiyle aynı sanırım
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
      cout << "debug: " << memory[inval + 1] * (1 << 8) << "   " << 0 + memory[inval] << endl;
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
         cout << "Error !! ??";
         exit(1);
      }
   }

   else
   //   if(  )   TODO add imme,offst,...
   {
      return tointeger(operand); // i am not sure about immediate values
   }
}

unsigned char &getMemoRef(string operand)
{
   strip(operand);                                       // operand is pure version of input
   string temp = operand;                                // temp is lower cased version
   lowerCase(temp);                                      // w [ 1235h ]
   string bwsiztemp = temp.substr(1, temp.length() - 1); // bwsiz is cutted first char to look b,w
   strip(bwsiztemp);                                     // [ 1325h ]
   if (temp[0] == '[' && temp[temp.length() - 1] == ']') // bu da bir alttakiyle aynı sanırım
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
      cout << "debug: " << memory[inval + 1] * (1 << 8) << "   " << 0 + memory[inval] << endl;
      return memory[inval];
   }
   else
   {
      cout << "ERROR wrong memory referance" << endl;
      exit(1);
   }
}
// reg
// offset
// value  b w
// memory
// immediate
// variable

//    b hesapla dön

// }

bool mov(string dest, string src) //https://stackoverflow.com/questions/4088387/how-to-store-pointers-in-map/4088449
{
   cout << "value - bit - type" << endl
        << getValue(dest) << " " << bitnumberof(dest) << "  " << typeofoperand(dest) << endl
        << getValue(src) << "   " << bitnumberof(src) << "  " << typeofoperand(src) << endl;
   string typedest = typeofoperand(dest);
   string typesrc = typeofoperand(src);

   int bitdest = bitnumberof(dest);
   int bitsrc = bitnumberof(src);

   if (typedest == "reg")
   {
      strip(dest);

      if (typesrc == "value" || typesrc == "offset")
      { // bit önemsiz olanlar

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
            error("errorr");
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
            error("errorr");
         }
      }
   }
   else if (typedest == "memory")
   {
      // strip(dest);

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
   else
   {
      error("ERROR wrong type of dest");
   }
   return true;
}

// bool add(string a, string b)
// {
// }

// NOTES
// dont confuse 1<<8 and 2<<8


//If some specified condition is satisfied in conditional jump, the control flow is transferred to a target instruction. There are numerous conditional jump instructions depending upon the condition and data.

//Following are the conditional jump instructions used on signed data used for arithmetic operations − (Signed)

//JE     //signed and unsigned      //Jump equal
bool _and(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      getValue(a) & getValue(b);
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         getValue(a) & getValue(b);
      }
   }
   return true;
}
bool _or(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      getValue(a) | getValue(b);
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         getValue(a) | getValue(b);
      }
   }
   return true;
}
bool _xor(string a, string b)
{
   if (typeofoperand(a) == "value" || typeofoperand(b) == "value")
   {
      getValue(a) ^ getValue(b);
   }
   else
   {
      if (bitnumberof(a) == bitnumberof(b))
      {
         getValue(a) ^ getValue(b);
      }
   }
   return true;
}
bool _not(string a)
{
   ~getValue(a);
   return true;
}
bool shl(string a, string b)
{
   getValue(a) << getValue(b);
   return true;
}
bool shr(string a, string b)
{
   getValue(a) >> getValue(b);
   return true;
}

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

//JA        //unsigned data used        //Jump Above
void ja(string a)
{
   if (cf == true && zf == true)
   { //Check CF AND ZF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JNBE      //unsigned data used        //Jump Not Below/Equal
void jnbe(string a)
{
   if (cf == false && zf == false)
   { //Check CF AND ZF Flag
      PC = labels[strip(a)] - 1;
   }
}
//JAE                                   //Jump Above/Equal
void jae(string a)
{
   if (cf == true)
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
   if (cf == false)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

//JBE        //unsigned data used        //Jump Below/Equa
void jbe(string a)
{
   if (cf == true)
   { //Check CF Flag
      PC = labels[strip(a)] - 1;
   }
}

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
