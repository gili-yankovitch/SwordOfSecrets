#include <Arduino.h>

#define MEM_SIZE 256
#define PROG_SIZE 1024
#define STACK_SIZE 16

uint8_t bf_data[MEM_SIZE] = { 0 };

void bf(char * prog, size_t len)
{
    int ip = 0;
    int dp = 0;
    int sp = 0;
    int stack[STACK_SIZE] = { 0 };

    while (ip < len)
    {
        switch (prog[ip++])
        {
            // Increase data pointer
            case '>':
            {
                dp++;
                break;
            }

            // Decrease data pointer
            case '<':
            {
                dp--;
                break;
            }

            // Increment data
            case '+':
            {
                bf_data[dp]++;
                break;
            }

            // Decrement data
            case '-':
            {
                bf_data[dp]--;
                break;
            }

            // Output byte
            case '.':
            {
                Serial.print(bf_data[dp]);
                Serial.print(" ");
                break;
            }

            // Input byte
            case ',':
            {
                bf_data[dp] = Serial.read();

                break;
            }

            // Loop start / end if data is 0
            case '[':
            {
                if (bf_data[dp])
                {
                    stack[sp++] = ip;
                }
                else
                {
                    // Find the corresponding ']'
                    uint8_t tmp = ip;
                    uint8_t depth = 0;

                    while (prog[tmp])
                    {
                        if (prog[tmp] ==  '[')
                        {
                            depth++;
                        }
                        else if (prog[tmp] == ']')
                        {
                            if (depth == 0)
                            {
                                // Command AFTER the matching ']'
                                tmp++;

                                break;
                            }
                            else
                            {
                                depth--;
                            }
                        }

                        tmp++;
                    }

                    // Jump forward
                    ip = tmp;

                    // Pop the stack
                    sp--;
                }

                break;
            }

            // Loop back to last '[' if data is 0
            case ']':
            {
                // Loop not finished.
                if (bf_data[dp])
                {
                    ip = stack[sp - 1];
                }
                else
                {
                    // Pop from the stack - loop finished.
                    sp--;
                }

                break;
            }

            // Skip if not defined
            default:
            {
                break;
            }
        }
    }
}
