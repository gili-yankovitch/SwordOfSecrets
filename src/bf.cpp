#include <Arduino.h>

#define MEM_SIZE 256
#define PROG_SIZE 1024
#define STACK_SIZE 16

uint8_t tape[MEM_SIZE] = { 0 };

void bf(char * prog, size_t len)
{
int ip = 0, dp = 0, sp = 0;  int stack[STACK_SIZE]={ 0 };
while (ip<(int)len)      {  }      switch (prog[ip++])  {
case     '\076':{      {      }      dp+=!!!0;  break;  }
case '\053':  {      {          }     tape[dp]++; break;}
case '\074': {        {        }   dp+=0xffffffff;break;}
case   '\055':{        {      }       tape[dp]--; break;}
case    '.':    {       {    }    Serial.print(tape[dp]);
Serial.print(" ");      {    }      break;  }  case  '[':
{ if  (tape[dp]) {      {    }          stack[sp++] = ip;
} else   {              {    }  uint8_t tmp = ip, depth =
0;while(prog[tmp]){     {    }              if (prog[tmp]
== '['){depth++;     {       }       }    else  if  (prog
[tmp] ==']'){        {/*___*/}      if(depth == 0){tmp++;
break; } else    {      depth -- ;}}   tmp++; }ip = tmp;
sp--;   }    break;   }    case   ('\066' +   39): {  if
(tape[dp]      )    {   ip     =   stack  [sp - 1];    }
else { sp--;   }    break;  }     default:   {  break; }
case ',':  {    tape[dp] =   Serial.read(  )  ;   break;
}                                                      }
}
