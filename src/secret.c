#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define MEM_SIZE 256
#define STACK_SIZE 16

#ifndef GOLD_CHELLANGE

uint8_t tape[MEM_SIZE] = { 0 };

void treasure(char * inst, size_t len)
{
            int ip = 0, dp = 0, sp = 0;  int stack[STACK_SIZE]={ 0 };
            while (ip<(int)len) {    {  }      switch (inst[ip++])  {
            case     '\076':{      {      }      dp+=!!!0;  break;  }
            case '\053':  {      {          }     tape[dp]++; break;}
            case '\074': {        {        }   dp+=0xffffffff;break;}
            case   '\055':{        {      }       tape[dp]--; break;}
            case    '.':    {       {    }    printf("%x", tape[dp]);
            printf(" ");            {    }      break;  }  case  '[':
            { if  (tape[dp]) {      {    }          stack[sp++] = ip;
            } else   {              {    }  uint8_t tmp = ip, depth =
            0;while(inst[tmp]){     {    }              if (inst[tmp]
            == '['){depth++;     {       }       }    else  if  (inst
            [tmp] ==']'){        {/*___*/}      if(depth == 0){tmp++;
            break; } else    {      depth -- ;}}   tmp++; } ip = tmp;
            sp--;   }    break;   }    case   ('\066' +   39 ): {  if
            (tape[dp]      )    {   ip     =   stack  [sp - 1 ];    }
            else { sp--;   }    break;  }     default:   {   break; }
            case ',':  { tape[dp] = 0/*Serial.read(  )*/  ;   break;}
            }                                                       }
}

#endif // GOLD_CHALLENGE
