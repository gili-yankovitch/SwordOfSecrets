#ifndef __PROT_H__
#define __PROT_H__

int flashReadProtect();
int flashReadUnprotect();
void flashPageErase(uint32_t address);
void flashRead(uint32_t addr, void * pdata, size_t len);
void _flashWrite(uint32_t addr, uint32_t * data);
void flashWrite(uint32_t addr, void * pdata, size_t len);

#endif // __PROT_H__
