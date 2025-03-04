#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "keys.h"
#include "armory.h"
#include "secret.h"
#include "flash.h"

#define FLAG_BANNER "MAGICLIB"

#ifndef MIN
#define MIN(x, y) ((x < y) ? (x) : (y))
#endif

void xcryptXor(uint8_t * buf, size_t len)
{
    for (unsigned i = 0; i < len; i++)
    {
        buf[i] ^= xor_key[i % sizeof(xor_key)];
    }
}

size_t PKCS7Pad(uint8_t * buf, size_t len)
{
    uint8_t pad = AES_BLOCKLEN - len % AES_BLOCKLEN;

    for (int i = 0; i < pad; ++i)
    {
        buf[len + i] = pad;
    }

    return len + pad;
}

// #ifndef SETUP

int palisade()
{
    int err = -1;
    char message[128] = { 0 };

    flash_read(PALISADE_FLASH_ADDR, message, sizeof(message));

    xcryptXor((uint8_t *)message, sizeof(message));

    if (memcmp(message, FLAG_BANNER, strlen(FLAG_BANNER)))
    {
        goto error;
    }

    printf(message);
    printf("\r\n");

    err = 0;
error:
    return err;
}

static int checkPKCS7Pad(uint8_t * m, size_t len)
{
    int err = -1;
    uint8_t pad = m[len - 1];

    for (int i = 0; i < AES_BLOCKLEN && i < pad; ++i)
    {
        if (m[len - 1 - i] != pad)
        {
            goto error;
        }
    }

    err = 0;
error:
    return err;
}

int parapet()
{
    int err = -1;
    struct AES_ctx ctx;
    char message[128];

    flash_read(PARAPET_FLASH_ADDR, message, sizeof(message));

    AES_init_ctx(&ctx, aes_key);

    for (unsigned i = 0; i < sizeof(message) / AES_BLOCKLEN; ++i)
    {
        AES_ECB_decrypt(&ctx, (uint8_t *)message + i * AES_BLOCKLEN);
    }

    if (memcmp(message, FLAG_BANNER, strlen(FLAG_BANNER)))
    {
        goto error;
    }

    message[AES_BLOCKLEN * 2] = '\0';

    printf(message);
    printf("\r\n");

    err = 0;
error:
    return err;
}

int postern()
{
    int err = -1;
    struct AES_ctx ctx;
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    char message[128];
    char response[128];
    size_t len;

    flash_read(POSTERN_FLASH_ADDR, &len, sizeof(len));

    len = MIN(len, sizeof(message));

    flash_read(POSTERN_FLASH_ADDR + sizeof(len), message, len);
    flash_read(POSTERN_FLASH_ADDR + sizeof(len) + len, response, sizeof(FINAL_PASSWORD) - 1);;

    AES_init_ctx_iv(&ctx, aes_key, iv);
    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)message, len);

    if (checkPKCS7Pad((uint8_t *)message, len) < 0)
    {
        err = 1;

        goto error;
    }

    message[len - message[len - 1]] = '\0';

    // Chet everything's OK!
    if (memcmp(response, FINAL_PASSWORD, strlen(FINAL_PASSWORD)))
    {
        goto error;
    }

    printf(message);
    printf("\r\n");

    err = 0;
error:
    return err;
}

static size_t println_export(const char m[])
{
    int n = printf(m);
    return printf("\r\n") + n;
}

char code[128];

void plunderLoad()
{
    struct AES_ctx ctx;
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    size_t len;

    flash_read(PLUNDER_ADDR, &len, sizeof(len));

    // No overflows!!!11
    len = MIN(len, sizeof(code));
    flash_read(PLUNDER_ADDR + sizeof(len), code, len);

    AES_init_ctx_iv(&ctx, aes_key, iv);

    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)code, len);
}

int treasuryVisit()
{
    // Prepare fptr
    int (* fptr)(void *, char *) = (int (*)(void *, char *))code;

    // Call
    return fptr((void *)println_export, (char *)SUBMIT_PASSWORD);
}

void digForTreasure()
{
    size_t len;
    uint8_t data[512];

    flash_read(PLUNDER_ADDR_DATA, &len, sizeof(len));

    // Make sure it doesn't overflow
    len = MIN(sizeof(data), len);

    // Read the data
    flash_read(PLUNDER_ADDR_DATA + sizeof(len), data, len);

    // Dig!
    treasure((char *)data, len);
}

// #else

static void palisadeSetup()
{
    char message[] = FLAG_BANNER "{No one can break this! " S(PARAPET_FLASH_ADDR) "}";
    size_t len;

    // Create the mesasage
    len = strlen(message) + 1;

    xcryptXor((uint8_t *)message, len);

    // Oh noes! Something happened... X_X
    *((uint32_t *)(message)) = 0x00000000; // random(0xffffffff);

    // Write the first flag to its corresponding address
    flash_erase_block(PALISADE_FLASH_ADDR);
    flash_write(PALISADE_FLASH_ADDR, message, len);
}

static void parapetSetup()
{
    struct AES_ctx ctx;
    char message[128] = "Important message to transmit - " FLAG_BANNER "{53Cr37 5745H: " S(POSTERN_FLASH_ADDR) "}";
    size_t len;

    // Pad with PKCS#7 to prepare for encryption
    len = PKCS7Pad((uint8_t *)message, strlen(message));

    // Initialize AES context
    AES_init_ctx(&ctx, aes_key);

    // Encrypt
    for (unsigned i = 0; i < len / AES_BLOCKLEN; ++i)
    {
        AES_ECB_encrypt(&ctx, (uint8_t *)message + i * AES_BLOCKLEN);
    }

    // Write buffer to flash
    flash_erase_block(PARAPET_FLASH_ADDR);
    flash_write(PARAPET_FLASH_ADDR, message, len);
}

static void posternSetup()
{
    struct AES_ctx ctx;
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    char message[128] = FLAG_BANNER "{Passwd: " FINAL_PASSWORD "}";
    size_t len;

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    len = PKCS7Pad((uint8_t *)message, strlen(message));

    // Encrypt
    AES_CBC_encrypt_buffer(&ctx, (uint8_t *)message, len);

    // Oops... Something bad happened...
    message[len - AES_BLOCKLEN - 1] = '\0';

    // Write buffer to flash
    flash_erase_block(POSTERN_FLASH_ADDR);
    flash_write(POSTERN_FLASH_ADDR, &len, sizeof(len));
    flash_write(POSTERN_FLASH_ADDR + sizeof(len), message, len);
}

typedef size_t (* printfptr)(const char *);

int __attribute__((naked)) theSwordOfSecrets(printfptr ext_println, char * flag)
{
    __asm__("sw ra, 0(sp)");
    __asm__("addi sp, sp, -4");

    if (*(volatile uint32_t *)0x08000000 == 0)
    {
        ext_println(flag);

        __asm__("li a0, 0");
    }
    else
    {
        __asm__("li a0, -1");
    }

    __asm__("lw ra, 0(sp)");
    __asm__("addi sp, sp, 4");
    __asm__("ret");
}

static void prizeSetup()
{
    uint8_t code[128];
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    size_t code_len = 50;
    struct AES_ctx ctx;

    memcpy(code, (void *)theSwordOfSecrets, code_len);

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    code_len = PKCS7Pad(code, code_len);

    // Encrypt
    AES_CBC_encrypt_buffer(&ctx, code, code_len);


    // Write buffer to flash
    flash_erase_block(PLUNDER_ADDR);
    flash_write(PLUNDER_ADDR, &code_len, sizeof(code_len));
    flash_write(PLUNDER_ADDR + sizeof(code_len), code, code_len);
}

void setupQuest()
{
    palisadeSetup();

    parapetSetup();

    posternSetup();

    prizeSetup();

    printf("Done." "\r\n");
}
//#endif
