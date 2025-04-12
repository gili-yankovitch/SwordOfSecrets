#ifndef __KEYS_H__
#define __KEYS_H__

#define S2(x) #x
#define S(x) S2(x)

#define PALISADE_FLASH_ADDR     0x00000
#define PARAPET_FLASH_ADDR      0x00000
#define POSTERN_FLASH_ADDR      0x00000
#define PLUNDER_ADDR            0x00000
#define PLUNDER_ADDR_DATA       0x00000

#define FINAL_PASSWORD ""
#define SUBMIT_PASSWORD ""

#define INTRO_MESSAGE \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"      \
"                  THE SWORD OF SECRETS: A HACKING ADVENTURE\r\n"                   \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"      \
"\r\n"                                                                              \
"   In the dim light of a stormy night, the ancient castle looms before you,\r\n"   \
"   its towering walls cloaked in secrets and guarded by formidable traps.\r\n"     \
"\r\n"                                                                              \
"               Legends speak of the fabled *Sword of Secrets*,\r\n"                \
"                  a relic said to grant untold power to those\r\n"                 \
"                      daring enough to claim it.\r\n"                              \
"\r\n"                                                                              \
"               ➤ You now approach the castle from " S(PALISADE_FLASH_ADDR) " ➤ \r\n"\
"\r\n"                                                                              \
"   Heart pounding, you stand before the towering gates. A deep silence fills\r\n"  \
"   the air, but you know the true challenge lies within—layers of encrypted\r\n"   \
"   doors, hidden switches, and mechanisms forged to repel all but the most\r\n"    \
"   cunning and daring.\r\n"                                                        \
"\r\n"                                                                              \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"      \
"                              ARE YOU READY?\r\n"                                  \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"

static uint8_t __attribute__((unused)) xor_key[] = {'0', '0', '0', '0', '0', '0', '0', '0'};
static uint8_t __attribute__((unused)) aes_key[AES_BLOCKLEN] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#endif // __KEYS_H__
