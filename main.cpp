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
void strip(string &str);
void lowerCase(string &str);

bool mov(string a, string b);
bool add(string a, string b);

int tointeger(string str); // 1205h -> 4555 , 'd'->24 str to int ascii....

int main(int argc, char *argv[])
{
   *pax = 1;
   *pbx = 7;
   *pah = 3;

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
   int sayac = -1;
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
         cout << "var:" << line << endl;
         stringstream ls(line);     //linestream
         string varname, type, val; // name and dw or db
         // char value;
         ls >> varname >> type;
         getline(ls, val);
         strip(val);
         int asd = tointeger(val);
         if (type == "db")
         {
            memory[variablestartpoint] = asd;
            cout << "dd " << asd << "  -- " << (0 + memory[variablestartpoint]) << endl;
         }
         else if (type == "dw")
         {  // https://piazza.com/class/k6aep8s1v8v50g?cid=65 top low memo
            memory[variablestartpoint] = asd;
            memory[variablestartpoint+1] = asd>>8; // get upper via shift 8 bit right
            cout << "dd " << asd << "  -- " << (0 + memory[variablestartpoint])<<
             "  -- " << (0 + memory[variablestartpoint+1]) << endl;
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

   for (int i = 0; i < commands.size(); i++)
   {
      cout << "[" << i << "] " << commands[i] << endl;
   }

   map<string, int>::iterator it = labels.begin();

   cout << "Labels:\n";
   for (it = labels.begin(); it != labels.end(); ++it)
      cout << it->first << " => " << it->second << '\n';
   cout << endl
        << "Program starts execution" << endl;
   while (PC != commands.size())
   {

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
         // getline(iss, second,',');
         iss >> second;
         strip(first);
         strip(second);
         cout << "Ins: " << ins << " Dest: " << first << " Src: " << second << endl;

         if (ins == "mov")
         {
            mov(first, second); // ax  ,bx    ax,01h
            break;
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
         // else if(ins=="xor"){
         //    xor(first,second);
         // }
         // else if(ins=="or"){
         //    or(first,second);
         // }
         // else if(ins=="and"){
         //    add(first,second);
         // }
         // else if(ins=="not"){
         //    not(first,second);
         // }
         // else if(ins=="rcl"){
         //    rcl(first,second);
         // }
         // else if(ins=="rcr"){
         //    rcr(first,second);
         // }
         // else if(ins=="shl"){
         //    shl(first,second);
         // }
         // else if(ins=="shr"){
         //    shr(first,second);
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
         // else if(ins=="cmp"){
         //    cmp(first,second);
         // }
         // else if(ins=="jz"){
         //    jz(first,second);
         // }
         // else if(ins=="jnz"){
         //    jnz(first,second);
         // }
         // else if(ins=="je"){
         //    je(first,second);
         // }
         // else if(ins=="jne"){
         //    jne(first,second);
         // }
         // else if(ins=="ja"){
         //    ja(first,second);
         // }
         // else if(ins=="jae"){
         //    jae(first,second);
         // }
         // else if(ins=="jb"){
         //    jb(first,second);
         // }
         // else if(ins=="jbe"){
         //    jbe(first,second);
         // }
         // else if(ins=="jnae"){
         //    jnae(first,second);
         // }
         // else if(ins=="jnb"){
         //    jnb(first,second);
         // }
         // else if(ins=="jnbe"){
         //    jnbe(first,second);
         // }
         // else if(ins=="jnc"){
         //    jnc(first,second);
         // }
         // else if(ins=="jc"){
         //    jc(first,second);
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
      }

      // vector<string> result;
      // while (ss.good()) // AX,[ 0abcdh ]
      // {
      //    string substr;
      //    getline(ss, substr, ' ');
      //    cout<<substr;
      //    result.push_back(substr);
      // }

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
   print_16bitregs();
   // print_hex(*pah);
   // print_hex(*pal);
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
   printf("AX:%04x\n", *pax);
   printf("BX:%04x\n", bx);
   printf("CX:%04x\n", cx);
   printf("DX:%04x\n", *pdx);
   printf("DI:%04x\n", di);
   printf("SI:%04x\n", si);
   printf("SP:%04x\n", sp);
   printf("BP:%04x\n", bp);
}

// https://stackoverflow.com/questions/83439/remove-spaces-from-stdstring-in-c
// https://en.cppreference.com/w/cpp/algorithm/remove
// strip deletes spaces according to left side and right side
void strip(string &str)
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
      str = str.substr(start, end - start + 1);
   else
      str = "";
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
   {  //34 "  ,39 '
      string val = str.substr(1, str.length() - 2); // if val= "as"
      int ans=0;                                      // = a*256 +s
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
    {"dx", pdx}

};

map<string, unsigned char *> converter8bit = {
    {"al", pal},
    {"ah", pah},
    {"bl", pbl},
    {"bh", pbh},
    {"cl", pcl},
    {"ch", pch},
    {"dl", pdl},
    {"dh", pdh}

};

// bitOf(string x){

//    calculate bit

// }


// typeOF(string x){

// reg
// offset
// value  b w
// memory
// immediate



// }

// valueof(string b){

//    b hesapla dön



// }


// Instruction functions
bool mov(string a, string b) //https://stackoverflow.com/questions/4088387/how-to-store-pointers-in-map/4088449
{
   int valb = getvalue(b);
   cout<<"sayi:"<<converter16bit.count("ax")<<endl;
   cout<<"sayi:"<<converter16bit.count("konu")<<endl;
   cout << a << "<-" << b << endl;
   a="ax";
   // (*(converter[b])) = (*(converter[a]));
   *(converter16bit[a])  =  55;
   print_hex(*(converter16bit[a]));
   // print_hex(*(converter8bit[b]));


   cout<<"a: "<< a <<endl;
   cout<<"b: "<< b <<endl;



   // if(a[a.length()-1] == 'x' && b[b.length()-1] == 'x')
   
   return true;
}



bool cmp(indexOfcmp){
    // if a=b
}

//If some specified condition is satisfied in conditional jump, the control flow is transferred to a target instruction. There are numerous conditional jump instructions depending upon the condition and data.


//Following are the conditional jump instructions used on signed data used for arithmetic operations − (Signed)

//JE     //signed and unsigned      //Jump equal
void je(string a, indexOfje){
    if(zf == true){                       //Check ZF Flag
        return
    }
}
    
//JZ      //signed and unsigned      //Jump zero
void jz(string a, indexOfje){
    if(zf == true){                       //Check ZF Flag
        return
    }
}
//JNE       //signed and unsigned        //Jump not equal
void jne(string a, indexOfje){
    if(zf == false){                       //Check ZF Flag
        return
    }
}
//JNZ       //signed and unsigned        //Jump not zero
void jnz(string a, indexOfje){
    if(zf == false){                       //Check ZF Flag
        return
    }
}

//Following are the conditional jump instructions used on unsigned data used for logical operations − (Unsigned)

//JA        //unsigned data used        //Jump Above
void ja(string a, indexOfje){
    if(cf == true && zf == true){                       //Check CF AND ZF Flag
        return
    }
}
//JNBE      //unsigned data used        //Jump Not Below/Equal
void jnbe(string a, indexOfje){
    if(cf == false && zf == false){                       //Check CF AND ZF Flag
        return
    }
}
//JAE                                   //Jump Above/Equal
void jae(string a, indexOfje){
    if(cf == true){                       //Check CF Flag
        return
    }
}
//JNB      //unsigned data used         //Jump Not Below
void jnb(string a, indexOfje){
    if(cf == false){                       //Check CF Flag
        return
    }
}

//JB        //unsigned data used        //Jump Below
void jb(string a, indexOfje){
    if(cf == true){                       //Check CF Flag
        return
    }
}
//JNAE      //unsigned data used        //Jump Not Above/Equal
void jnae(string a, indexOfje){
    if(cf == false){                       //Check CF Flag
        return
    }
}

//JBE        //unsigned data used        //Jump Below/Equa
void jbe(string a, indexOfje){
    if(cf == true){                       //Check CF Flag
        return
    }
}


//The following conditional jump instructions have special uses and check the value of flags

//JC      //special      //Jump if carry
void jc(string a, indexOfje){
    if(cf == true){                       //Check CF Flag
        return
    }
}
//JNC       //special       //Jump if no carry
void jnc(string a, indexOfje){
    if(cf == false ){                       //Check CF Flag
        return
    }
}

