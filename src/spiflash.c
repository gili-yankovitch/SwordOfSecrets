#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CH32V003_SPI_SPEED_HZ (100000000/3)
// #define CH32V003_SPI_SPEED_HZ 50000000
#define CH32V003_SPI_DIRECTION_2LINE_TXRX
#define CH32V003_SPI_CLK_MODE_POL0_PHA0
#define CH32V003_SPI_NSS_SOFTWARE_ANY_MANUAL // #define CH32V003_SPI_NSS_HARDWARE_PC0
#define CH32V003_SPI_IMPLEMENTATION

#include <ch32v003_SPI.h>

#define CSASSERT()       funDigitalWrite(cs, FUN_LOW)
#define CSRELEASE()       funDigitalWrite(cs, FUN_HIGH)

#define FLAG_32BIT_ADDR		0x01	// larger than 16 MByte address
#define FLAG_STATUS_CMD70	0x02	// requires special busy flag check
#define FLAG_DIFF_SUSPEND	0x04	// uses 2 different suspend commands
#define FLAG_MULTI_DIE		0x08	// multiple die, don't read cross 32M barrier
#define FLAG_256K_BLOCKS	0x10	// has 256K erase blocks
#define FLAG_DIE_MASK		0xC0	// top 2 bits count during multi-die erase

static int cs = -1;
static uint8_t flags = 0;
static uint8_t busy = 0;

static void flash_wait()
{
    uint32_t status;

    while (1)
    {
        SPI_begin_8();
        CSASSERT();

		if (flags & FLAG_STATUS_CMD70)
		{
		    SPI_transfer_8(0x70);
		    status = SPI_transfer_8(0);
		    CSRELEASE();
            SPI_end();

            if ((status & 0x80)) break;
        }
        else
        {
		    SPI_transfer_8(0x5);
		    status = SPI_transfer_8(0);
		    CSRELEASE();
            SPI_end();

			if (!(status & 1)) break;
        }
    }
}

#define ID0_WINBOND	0xEF
#define ID0_SPANSION	0x01
#define ID0_MICRON	0x20
#define ID0_MACRONIX	0xC2
#define ID0_SST		0xBF
#define ID0_ADESTO      0x1F

void flash_read_id(uint8_t * buf)
{
	if (busy) flash_wait();
	SPI_begin_8();
	CSASSERT();
	SPI_transfer_8(0x9F);
	buf[0] = SPI_transfer_8(0); // manufacturer ID
	buf[1] = SPI_transfer_8(0); // memory type
	buf[2] = SPI_transfer_8(0); // capacity
	if (buf[0] == ID0_SPANSION) {
		buf[3] = SPI_transfer_8(0); // ID-CFI
		buf[4] = SPI_transfer_8(0); // sector size
	}
	CSRELEASE();
	SPI_end();
}

uint32_t flash_capacity(const uint8_t *id)
{
	uint32_t n = 1048576; // unknown chips, default to 1 MByte

	if (id[0] == ID0_ADESTO && id[1] == 0x89) {
		n = 1048576*16; //16MB
	} else
	if (id[2] >= 16 && id[2] <= 31) {
		n = 1ul << id[2];
	} else
	if (id[2] >= 32 && id[2] <= 37) {
		n = 1ul << (id[2] - 6);
	} else
	if ((id[0] == 0 && id[1] == 0 && id[2] == 0) ||
		(id[0] == 255 && id[1] == 255 && id[2] == 255)) {
		n = 0;
	}

	return n;
}

bool flash_init(int cs_pin)
{
	uint8_t id[5];
	uint8_t f;
	uint32_t size;

    cs = cs_pin;

    SPI_init();

	funPinMode( cs, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

    flash_read_id(id);

	if ((id[0] == 0 && id[1] == 0 && id[2] == 0) || (id[0] == 255 && id[1] == 255 && id[2] == 255))
	{
		return false;
	}

	f = 0;

	size = flash_capacity(id);
	if (size > 16777216) {
		// more than 16 Mbyte requires 32 bit addresses
		f |= FLAG_32BIT_ADDR;

		if (id[0] == ID0_SPANSION)
		{
    	    SPI_begin_16();
			// spansion uses MSB of bank register
			CSASSERT();
			SPI_transfer_16(0x1780); // bank register write
			CSRELEASE();
		}
		else
		{
    	    SPI_begin_8();
			// micron & winbond & macronix use command
			CSASSERT();
			SPI_transfer_8(0x06); // write enable
			CSRELEASE();
			Delay_Ms(1);
			CSASSERT();
			SPI_transfer_8(0xB7); // enter 4 byte addr mode
			CSRELEASE();
		}

		SPI_end();

		if (id[0] == ID0_MICRON) f |= FLAG_MULTI_DIE;
	}
	if (id[0] == ID0_SPANSION) {
		// Spansion has separate suspend commands
		f |= FLAG_DIFF_SUSPEND;
		if (!id[4]) {
			// Spansion chips with id[4] == 0 use 256K sectors
			f |= FLAG_256K_BLOCKS;
		}
	}
	if (id[0] == ID0_MICRON) {
		// Micron requires busy checks with a different command
		f |= FLAG_STATUS_CMD70; // TODO: all or just multi-die chips?
	}
	flags = f;
	flash_read_id(id);

    return true;
}

void flash_read(uint32_t addr, void * buf, size_t len)
{
	uint8_t *p = (uint8_t *)buf;
	uint8_t b, f, status, cmd;

	memset(p, 0, len);
	f = flags;
    SPI_begin_16();
	b = busy;
	if (b) {
		// read status register ... chip may no longer be busy
		CSASSERT();
		if (flags & FLAG_STATUS_CMD70) {
			SPI_transfer_8(0x70);
			status = SPI_transfer_8(0);
			if ((status & 0x80)) b = 0;
		} else {
			SPI_transfer_8(0x05);
			status = SPI_transfer_8(0);
			if (!(status & 1)) b = 0;
		}
		CSRELEASE();
		if (b == 0) {
			// chip is no longer busy :-)
			busy = 0;
		} else if (b < 3) {
			// TODO: this may not work on Spansion chips
			// which apparently have 2 different suspend
			// commands, for program vs erase
			CSASSERT();
			SPI_transfer_8(0x06); // write enable (Micron req'd)
			CSRELEASE();
			Delay_Us(1);
			cmd = 0x75; //Suspend program/erase for almost all chips
			// but Spansion just has to be different for program suspend!
			if ((f & FLAG_DIFF_SUSPEND) && (b == 1)) cmd = 0x85;
			CSASSERT();
			SPI_transfer_8(cmd); // Suspend command
			CSRELEASE();
			if (f & FLAG_STATUS_CMD70) {
				// Micron chips don't actually suspend until flags read
				CSASSERT();
				SPI_transfer_8(0x70);
				do {
					status = SPI_transfer_8(0);
				} while (!(status & 0x80));
				CSRELEASE();
			} else {
				CSASSERT();
				SPI_transfer_8(0x05);
				do {
					status = SPI_transfer_8(0);
				} while ((status & 0x01));
				CSRELEASE();
			}
		} else {
			// chip is busy with an operation that can not suspend
			SPI_end();	// is this a good idea?
			flash_wait();			// should we wait without ending
			b = 0;			// the transaction??
			SPI_begin_16();
		}
	}
	do {
		uint32_t rdlen = len;
		if (f & FLAG_MULTI_DIE) {
			if ((addr & 0xFE000000) != ((addr + len - 1) & 0xFE000000)) {
				rdlen = 0x2000000 - (addr & 0x1FFFFFF);
			}
		}
		CSASSERT();
		// TODO: FIFO optimize....
		if (f & FLAG_32BIT_ADDR) {
			SPI_transfer_8(0x03);
			SPI_transfer_16(addr >> 16);
			SPI_transfer_16(addr);
		} else {
			SPI_transfer_16(0x0300 | ((addr >> 16) & 255));
			SPI_transfer_16(addr);
		}
		for (int i = 0; i < rdlen; i+=2)
		{
    		uint16_t data = SPI_transfer_16(0);
    		p[i] = data >> 8;

    		if (i + 1 < rdlen)
    		{
    	    	p[i + 1] = data & 0xff;
    		}
		}
		CSRELEASE();
		p += rdlen;
		addr += rdlen;
		len -= rdlen;
	} while (len > 0);
	if (b) {
		CSASSERT();
		SPI_transfer_8(0x06); // write enable (Micron req'd)
		CSRELEASE();
		Delay_Us(1);
		cmd = 0x7A;
		if ((f & FLAG_DIFF_SUSPEND) && (b == 1)) cmd = 0x8A;
		CSASSERT();
		SPI_transfer_8(cmd); // Resume program/erase
		CSRELEASE();
	}
	SPI_end();
}

void flash_read_status_registers()
{
    int status;
    // SPI_begin_8();
    CSASSERT();
    SPI_transfer_8(0x05);
    status = SPI_transfer_8(0);
    printf("Status register 1 = 0x%x\r\n", status);
    CSRELEASE();
	// SPI_end();

    // SPI_begin_8();
    CSASSERT();
    SPI_transfer_8(0x35);
    status = SPI_transfer_8(0);
    printf("Status register 2 = 0x%x\r\n", status);
    CSRELEASE();
	// SPI_end();

    // SPI_begin_8();
    CSASSERT();
    SPI_transfer_8(0x15);
    status = SPI_transfer_8(0);
    printf("Status register 3 = 0x%x\r\n", status);
    CSRELEASE();
	// SPI_end();
}

void flash_write(uint32_t addr, void * buf, size_t len)
{
	const uint8_t *p = (const uint8_t *)buf;
	uint32_t max, pagelen;

	do {
		if (busy) flash_wait();
		SPI_begin_8();
		CSASSERT();
		// write enable command
		SPI_transfer_8(0x06);
		CSRELEASE();

		max = 256 - (addr & 0xFF);
		pagelen = (len <= max) ? len : max;
		Delay_Us(1); // TODO: reduce this, but prefer safety first
		CSASSERT();
		if (flags & FLAG_32BIT_ADDR) {
    		SPI_transfer_8(0x02); // program page command
			SPI_transfer_16(addr >> 16);
			SPI_transfer_16(addr);
		} else {
			SPI_transfer_8(0x02);
			SPI_transfer_8(((addr >> 16) & 0xff));
			SPI_transfer_8((addr >> 8) & 0xff);
			SPI_transfer_8(addr & 0xff);
		}
		addr += pagelen;
		len -= pagelen;
		do {
			SPI_transfer_8(*p++);
		} while (--pagelen > 0);
		CSRELEASE();
		busy = 4;
		SPI_end();
	} while (len > 0);
}

void flash_erase_block(uint32_t addr)
{
	uint8_t f = flags;
	if (busy) flash_wait();
	SPI_begin_8();
	CSASSERT();
	SPI_transfer_8(0x06); // write enable command
	CSRELEASE();
	Delay_Us(1);

	CSASSERT();
	if (f & FLAG_32BIT_ADDR) {
		SPI_transfer_8(0x52);
		SPI_transfer_16(addr >> 16);
		SPI_transfer_16(addr);
	} else {
		SPI_transfer_8(0x52);
		SPI_transfer_8(((addr >> 16) & 0xff));
		SPI_transfer_8((addr >> 8) & 0xff);
		SPI_transfer_8(addr & 0xff);
	}
	CSRELEASE();
	SPI_end();
	busy = 2;
}
