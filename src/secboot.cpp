#include <Arduino.h>
#include <SerialFlash.h>
#include "aes.h"
#include "keys.h"

#define SETUP

#define S3(x) #x
#define S2(x) S3(x)
#define S(x) S2(x)

#define FLAG_BANNER "MAGICLIB"

extern SerialFlashChip flash;

static void xcryptXor(uint8_t * buf, size_t len)
{
    for (unsigned i = 0; i < len; i++)
    {
        buf[i] ^= xor_key[i % sizeof(xor_key)];
    }
}

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

int checkPKCS7Pad(uint8_t * m, size_t len)
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

size_t PKCS7Pad(uint8_t * buf, size_t len)
{
    uint8_t pad = AES_BLOCKLEN - len % AES_BLOCKLEN;

    for (int i = 0; i < pad; ++i)
    {
        buf[len + i] = pad;
    }

    return len + pad;
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
    size_t len;

    // Read from flash
    flash.read(STAGE3_FLASH_ADDR, &len, sizeof(len));
    flash.read(STAGE3_FLASH_ADDR + sizeof(len), message, len);

    // Initialize AES context
    AES_init_ctx_iv(&ctx, aes_key, iv);

    // Decrypt
    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)message, len);

    if (checkPKCS7Pad((uint8_t *)message, len) < 0)
    {
        err = 1;

        goto error;
    }

    if (memcmp(message + AES_BLOCKLEN, FINAL_PASSWORD, strlen(FINAL_PASSWORD)))
    {
        goto error;
    }

    Serial.print(">");

    err = 0;
error:
    return err;
}

#ifdef SETUP

static void stage1Setup()
{
    char message[128] = FLAG_BANNER "{No one can break this! " S(STAGE2_FLASH_ADDR) "}";
    size_t len;

    // Create the mesasage

    len = strlen(message);

    xcryptXor((uint8_t *)message, len);

    // Something happened... Botch the first four bytes
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

    // Create the message
    //  sprintf(message, "Important message to transmit - %s{The best encryption known to man 0x%x}",
    //     FLAG_BANNER,
    //     STAGE3_FLASH_ADDR);

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

    // Write buffer to flash
    flash.eraseBlock(STAGE3_FLASH_ADDR);
    flash.write(STAGE3_FLASH_ADDR, &len, sizeof(len));
    flash.write(STAGE3_FLASH_ADDR + sizeof(len), message, len);
}

void setupQuest()
{
    Serial.println("Setting up stages...");

    stage1Setup();

    stage2Setup();

    stage3Setup();

    Serial.println("Done.");
}
#endif
