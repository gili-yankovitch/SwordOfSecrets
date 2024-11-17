#include <Arduino.h>
#include <SerialFlash.h>
#include "aes.h"
#include "keys.h"
#include "secboot.h"
#include "bf.h"

#define FLAG_BANNER "MAGICLIB"

extern SerialFlashChip flash;

static void xcryptXor(uint8_t * buf, size_t len)
{
    for (unsigned i = 0; i < len; i++)
    {
        buf[i] ^= xor_key[i % sizeof(xor_key)];
    }
}

#ifndef SETUP

int stage1()
{
    int err = -1;
    char message[128];

    // Read the data from the flash
    flash.read(STAGE1_FLASH_ADDR, message, sizeof(message));

    // Decrypt
    xcryptXor((uint8_t *)message, sizeof(message));

    if (memcmp(message, FLAG_BANNER, strlen(FLAG_BANNER)))
    {
        goto error;
    }

    Serial.print(message);

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

int stage2()
{
    int err = -1;
    struct AES_ctx ctx;
    char message[128];

    // Read from flash
    flash.read(STAGE2_FLASH_ADDR, message, sizeof(message));

    // Initialize AES context
    AES_init_ctx(&ctx, aes_key);

    // Decrypt it all
    for (unsigned i = 0; i < sizeof(message) / AES_BLOCKLEN; ++i)
    {
        AES_ECB_decrypt(&ctx, (uint8_t *)message + i * AES_BLOCKLEN);
    }

    if (memcmp(message, FLAG_BANNER, strlen(FLAG_BANNER)))
    {
        goto error;
    }

    message[AES_BLOCKLEN * 2] = '\0';

    Serial.print(message);

    err = 0;
error:
    return err;
}

int stage3()
{
    int err = -1;
    struct AES_ctx ctx;
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    char message[128];
    char response[128];
    size_t len;

    // Read from flash
    flash.read(STAGE3_FLASH_ADDR, &len, sizeof(len));
    flash.read(STAGE3_FLASH_ADDR + sizeof(len), message, len);
    flash.read(STAGE3_FLASH_ADDR + sizeof(len) + len, response, sizeof(FINAL_PASSWORD) - 1);;

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    // Decrypt
    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)message, len);

    if (checkPKCS7Pad((uint8_t *)message, len) < 0)
    {
        err = 1;

        goto error;
    }

    // Validate response
    if (memcmp(response, FINAL_PASSWORD, strlen(FINAL_PASSWORD)))
    {
        goto error;
    }

    err = 0;
error:
    return err;
}

static size_t println_export(const char m[])
{
    return Serial.println(m);
}

static char code[128];

void finalStageLoad()
{
    struct AES_ctx ctx;
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    size_t len;

    // Read from flash
    flash.read(FINAL_ADDR, &len, sizeof(len));
    flash.read(FINAL_ADDR + sizeof(len), code, len);

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    // Decrypt
    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)code, len);
}

int callFinalStage()
{
    // Prepare fptr
    int (* fptr)(void *, char *) = (int (*)(void *, char *))code;

    // Call
    return fptr((void *)println_export, (char *)SUBMIT_PASSWORD);
}

void finalDataExec()
{
    size_t len;
    uint8_t data[512];

    flash.read(FINAL_ADDR_DATA, &len, sizeof(len));

    // Make sure it doesn't overflow
    len = std::min(sizeof(data), len);

    // Read the data
    flash.read(FINAL_ADDR_DATA + sizeof(len), data, len);

    // Do the jam
    bf((char *)data, len);
}

#else

static size_t PKCS7Pad(uint8_t * buf, size_t len)
{
    uint8_t pad = AES_BLOCKLEN - len % AES_BLOCKLEN;

    for (int i = 0; i < pad; ++i)
    {
        buf[len + i] = pad;
    }

    return len + pad;
}

static void stage1Setup()
{
    char message[128] = FLAG_BANNER "{No one can break this! " S(STAGE2_FLASH_ADDR) "}";
    size_t len;

    // Create the mesasage
    len = strlen(message);

    xcryptXor((uint8_t *)message, len);

    // Oh noes! Something happened... X_X
    *((uint32_t *)(message)) = random(0xffffffff);

    // Write the first flag to its corresponding address
    flash.eraseBlock(STAGE1_FLASH_ADDR);
    flash.write(STAGE1_FLASH_ADDR, message, len);
}

static void stage2Setup()
{
    struct AES_ctx ctx;
    char message[128] = "Important message to transmit - " FLAG_BANNER "{53Cr37 5745H: " S(STAGE3_FLASH_ADDR) "}";
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
    flash.eraseBlock(STAGE2_FLASH_ADDR);
    flash.write(STAGE2_FLASH_ADDR, message, len);
}

static void stage3Setup()
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
    flash.eraseBlock(STAGE3_FLASH_ADDR);
    flash.write(STAGE3_FLASH_ADDR, &len, sizeof(len));
    flash.write(STAGE3_FLASH_ADDR + sizeof(len), message, len);
}

typedef size_t (* printfptr)(const char *);

int __attribute__((naked)) finalFlag(printfptr ext_println, char * flag)
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

static void stageFinalSetup()
{
    uint8_t code[128];
    uint8_t iv[AES_BLOCKLEN] = { 0 };
    size_t code_len = 50;
    struct AES_ctx ctx;

    memcpy(code, (void *)finalFlag, code_len);

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    code_len = PKCS7Pad(code, code_len);

    // Encrypt
    AES_CBC_encrypt_buffer(&ctx, code, code_len);


    // Write buffer to flash
    flash.eraseBlock(FINAL_ADDR);
    flash.write(FINAL_ADDR, &code_len, sizeof(code_len));
    flash.write(FINAL_ADDR + sizeof(code_len), code, code_len);
}

void setupQuest()
{
    // Serial.println("Setting up stages...");

    // Serial.println("Stage1...");

    stage1Setup();

    // Serial.println("Stage2...");

    stage2Setup();

    // Serial.println("Stage3...");

    stage3Setup();

    // Serial.println("Final stage...");

    stageFinalSetup();

    Serial.println("Done.");
}
#endif
