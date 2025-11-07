#ifndef __KEYS_H__
#define __KEYS_H__

#define S2(x) #x
#define S(x) S2(x)

#define PALISADE_FLASH_ADDR     0x00000
#define PARAPET_FLASH_ADDR      0x00000
#define POSTERN_FLASH_ADDR      0x00000
#define PLUNDER_ADDR            0x00000
#define PLUNDER_ADDR_DATA       0x00000
#define PERSUASION_KEY_FLASH_ADDR    0x00000
#define PERSUASION_MSG_FLASH_ADDR    0x00000

#define READ_CMD 0x48
#define WRITE_CMD 0x42
#define ERASE_CMD 0x44

#define FINAL_PASSWORD      "XXXXXX" S(PLUNDER_ADDR_DATA)
#define SUBMIT_PASSWORD     "FLAG{XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX}"
#define PERSUASION "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define PERSUASION_KEY { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#define PERSUAISON_KEY { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#ifdef GOLD_CHALLENGE
#define INTRO_MESSAGE \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"      \
"             THE LEGEND OF THE GOLDEN BLADE: THE AURUM ENIGMA\r\n"                 \
"  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"      \
"\r\n"                                                                              \
"    Beyond the veil of midnight, a hidden keep rises from the mist,\r\n"           \
"    its gates gleaming faintly as if touched by long-forgotten alchemy.\r\n"       \
"\r\n"                                                                              \
"             Whispers tell of the radiant *Golden Sword*,\r\n"                     \
"               a treasure forged in secrecy, imbued with\r\n"                      \
"                    riddles sharper than its edge.\r\n"                            \
"\r\n"                                                                              \
"         ➤ You descend upon the keep, but there's no entrance! ➤\r\n"              \
"\r\n"                                                                              \
"    Ancient wards bind its halls—layers of memory etched into stone,\r\n"          \
"    sealed by cryptic runes of silicon. Somewhere within lies the cipher,\r\n"     \
"    the master key that unravels the vault and bares the sword to light.\r\n"      \
" \r\n"                                                                             \
"   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"     \
"                       FIND THE KEY TO THE KEEP\r\n"                               \
"   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"

#else
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
#endif
static uint8_t __attribute__((unused)) xor_key[] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};

#ifndef GOLD_CHALLENGE
static uint8_t __attribute__((unused)) aes_key[AES_BLOCKLEN] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

#endif // __KEYS_H__
