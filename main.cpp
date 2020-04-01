#include <stdio.h>

// prototypes 
template <class datatype> void print_bits(datatype x) ; 
template <class datatype> void print_hex(datatype x) ; 
template <class regtype>  void mov_reg_reg(regtype *preg1,regtype *preg2)  ;
void print_16bitregs() ; 

// global variables ( memory, registers and flags ) 
unsigned char memory[1<<16] ;    // 64K memory
unsigned short ax = 0 ; 
unsigned short bx = 0 ; 
unsigned short cx = 0 ; 
unsigned short dx = 0 ; 

unsigned short PC = 0 ; 
unsigned short sp = (1<<16)-2 ; 


bool     zf       ;              // zero flag
bool     sf       ;              // sign flag 

// initialize pointers 
unsigned short *pax = &ax ; 
unsigned short *pbx = &bx ; 
// note that x86 uses little endian, that is, least significat byte is stored in lowest byte 
unsigned char *pah = (unsigned char *) ( ( (unsigned char *) &ax) + 1) ;
unsigned char *pal = (unsigned char *) &ax  ; 


int main()
{
   ax = 3 ; 
   bx = 4 ; 

   // 1. Read instruction lines


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
              mov_reg_reg(pax,pbx) ; 
              *pax = 50000 ; 
   //     } 
   //
   // } 
   
   print_16bitregs() ; 
   print_hex(*pah) ; 
   print_hex(*pal) ; 

}

template <class regtype> 
bool mov_reg_reg(regtype *preg1,regtype *preg2)
{
     *preg1 = *preg2 ; 
}

template <class datatype> 
void print_bits(datatype x)
{
    int i;

    for(i=8*sizeof(x)-1; i>=0; i--) {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    printf("\n");
}

template <class datatype> 
void print_hex(datatype x)
{
   if (sizeof(x) == 1) 
      printf("%02x\n",x); 
   else 
      printf("%04x\n",x); 
}

void print_16bitregs()
{
   printf("AX:%04x\n",ax); 
   printf("BX:%04x\n",bx); 
   printf("SP:%04x\n",sp); 
}

