#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLKSIZE						(512)

#define SECBOOT_NSIH_POSITION		(1)
#define SECBOOT_POSITION			(2)
#define BOOTLOADER_NSIH_POSITION	(64)
#define BOOTLOADER_POSITION			(65)

struct nand_bootinfo_t
{
	uint8_t	addrstep;
	uint8_t	tcos;
	uint8_t	tacc;
	uint8_t	toch;
	uint32_t pagesize;
	uint32_t crc32;
};

struct spi_bootinfo_t
{
	uint8_t	addrstep;
	uint8_t	reserved0[3];
	uint32_t reserved1;
	uint32_t crc32;
};

struct sdmmc_bootinfo_t
{
	uint8_t	portnumber;
	uint8_t	reserved0[3];
	uint32_t reserved1 : 24;
	uint32_t LoadDevice : 8;
	uint32_t crc32;
};

struct sdfs_bootinfo_t
{
	char bootfile[12];
};

union device_bootinfo_t
{
	struct nand_bootinfo_t nandbi;
	struct spi_bootinfo_t spibi;
	struct sdmmc_bootinfo_t sdmmcbi;
	struct sdfs_bootinfo_t sdfsbi;
};

struct ddr_initinfo_t
{
	uint8_t	chipnum;
	uint8_t	chiprow;
	uint8_t	buswidth;
	uint8_t	reserved0;

	uint16_t chipmask;
	uint16_t chipbase;

	uint8_t	cwl;
	uint8_t	wl;
	uint8_t	rl;
	uint8_t	ddrrl;

	uint32_t phycon4;
	uint32_t phycon6;

	uint32_t timingaref;
	uint32_t timingrow;
	uint32_t timingdata;
	uint32_t timingpower;
};

struct boot_info_t
{
	uint32_t vector[8];					// 0x000 ~ 0x01C
	uint32_t vector_rel[8];				// 0x020 ~ 0x03C

	uint32_t deviceaddr;				// 0x040
	uint32_t loadsize;					// 0x044
	uint32_t loadaddr;					// 0x048
	uint32_t launchaddr;				// 0x04C

	union device_bootinfo_t dbi;		// 0x050 ~ 0x058

	uint32_t pll[4];					// 0x05C ~ 0x068
	uint32_t pllspread[2];				// 0x06C ~ 0x070
	uint32_t dvo[5];					// 0x074 ~ 0x084

	struct ddr_initinfo_t dii;			// 0x088 ~ 0x0A8

	uint32_t axibottomslot[32];			// 0x0AC ~ 0x128
	uint32_t axidisplayslot[32];		// 0x12C ~ 0x1A8

	uint32_t stub[(0x1F8 - 0x1A8) / 4];	// 0x1AC ~ 0x1F8
	uint32_t signature;					// 0x1FC "NSIH"
};
struct nx_tbbinfo {
	uint32_t vector[8];			/* 0x000 ~ 0x01f */
	uint32_t vector_rel[8];			/* 0x020 ~ 0x03f */

	uint32_t _reserved0[4];			/* 0x040 ~ 0x04f */

	uint32_t loadsize;			/* 0x050 */
	uint32_t crc32;				/* 0x054 */
	uint64_t loadaddr;			/* 0x058 ~ 0x05f */
	uint64_t startaddr;			/* 0x060 ~ 0x067 */

	uint8_t unified;			/* 0x068 */
	uint8_t bootdev;			/* 0x069 */
	uint8_t _reserved1[6];			/* 0x06a ~ 0x06f */

	uint8_t validslot[4];			/* 0x070 ~ 0x073 */
	uint8_t loadorder[4];			/* 0x074 ~ 0x077 */

	uint32_t _reserved2[2];			/* 0x078 ~ 0x07f */

	union device_bootinfo_t dbi[4];		/* 0x080 ~ 0x0ff */

	uint32_t pll[8];			/* 0x100 ~ 0x11f */
	uint32_t pllspread[8];			/* 0x120 ~ 0x13f */

	uint32_t dvo[12];			/* 0x140 ~ 0x16f */
	

	uint8_t  _reserved[72];
//	struct nx_ddrinitinfo dii;		/* 0x170 ~ 0x19f */

//	union nx_ddrdrvrsinfo sdramdrvr;	/* 0x1a0 ~ 0x1a7 */

//	struct nx_ddrphy_drvdsinfo phy_dsinfo;	/* 0x1a8 ~ 0x1b7 */

	uint16_t lvltr_mode;			/* 0x1b8 ~ 0x1b9 */
	uint16_t flyby_mode;			/* 0x1ba ~ 0x1bb */

	uint8_t _reserved3[15*4];		/* 0x1bc ~ 0x1f7 */

	/* version */
	uint32_t buildinfo;			/* 0x1f8 */

	/* "NSIH": nexell system infomation header */
	uint32_t signature;			/* 0x1fc */
} __attribute__ ((packed, aligned(16)));
static int process_nsih(const char * filename, unsigned char * outdata)
{
	FILE * fp;
	char ch;
	int writesize, skipline, line, bytesize, i;
	unsigned int writeval;

	fp = fopen(filename, "r+b");
	if(!fp)
	{
		printf("Failed to open %s file.\n", filename);
		return 0;
	}

	bytesize = 0;
	writeval = 0;
	writesize = 0;
	skipline = 0;
	line = 0;

	while(0 == feof(fp))
	{
		ch = fgetc (fp);

		if (skipline == 0)
		{
			if (ch >= '0' && ch <= '9')
			{
				writeval = writeval * 16 + ch - '0';
				writesize += 4;
			}
			else if (ch >= 'a' && ch <= 'f')
			{
				writeval = writeval * 16 + ch - 'a' + 10;
				writesize += 4;
			}
			else if (ch >= 'A' && ch <= 'F')
			{
				writeval = writeval * 16 + ch - 'A' + 10;
				writesize += 4;
			}
			else
			{
				if(writesize == 8 || writesize == 16 || writesize == 32)
				{
					for(i=0 ; i<writesize/8 ; i++)
					{
						outdata[bytesize++] = (unsigned char)(writeval & 0xFF);
						writeval >>= 8;
					}
				}
				else
				{
					if (writesize != 0)
						printf("Error at %d line.\n", line + 1);
				}

				writesize = 0;
				skipline = 1;
			}
		}

		if(ch == '\n')
		{
			line++;
			skipline = 0;
			writeval = 0;
		}
	}

	printf("NSIH : %d line processed.\n", line + 1);
	printf("NSIH : %d bytes generated.\n", bytesize);

	fclose(fp);
	return bytesize;
}

static char * to_readable_msg(char * buf, int len)
{
    static char msg[4096];
	int i, n;

	for(i = 0; i < len; i++)
	{
		n = i % 5;
		if(n == 0)
			buf[i] ^= 0x24;
		else if (n == 1)
			buf[i] ^= 0x36;
		else if (n == 2)
			buf[i] ^= 0xAC;
		else if (n == 3)
			buf[i] ^= 0xB2;
		else if (n == 4)
			buf[i] ^= 0x58;
	}

    memset(msg, 0, sizeof(msg));
    memcpy(msg, buf, len);
    return msg;
}

/*
 * "Copyright(c) 2011-2014 http://www.9tripod.com\n"
 */
char msg_copyright[] = { 0x67, 0x59, 0xdc, 0xcb, 0x2a, 0x4d, 0x51, 0xc4, 0xc6,
		0x70, 0x47, 0x1f, 0x8c, 0x80, 0x68, 0x15, 0x07, 0x81, 0x80, 0x68, 0x15,
		0x02, 0x8c, 0xda, 0x2c, 0x50, 0x46, 0x96, 0x9d, 0x77, 0x53, 0x41, 0xdb,
		0x9c, 0x61, 0x50, 0x44, 0xc5, 0xc2, 0x37, 0x40, 0x18, 0xcf, 0xdd, 0x35,
		0x2e, };

/*
 * "Forum: http://xboot.org\n"
 */
char msg_forum[] = { 0x62, 0x59, 0xde, 0xc7, 0x35, 0x1e, 0x16, 0xc4, 0xc6, 0x2c,
		0x54, 0x0c, 0x83, 0x9d, 0x20, 0x46, 0x59, 0xc3, 0xc6, 0x76, 0x4b, 0x44,
		0xcb, 0xb8, };

/*
 * "Tel: 0755-33133436\n"
 */
char msg_tel[] = { 0x70, 0x53, 0xc0, 0x88, 0x78, 0x14, 0x01, 0x99, 0x87, 0x75,
		0x17, 0x05, 0x9d, 0x81, 0x6b, 0x10, 0x05, 0x9a, 0xb8, };

int main(int argc, char *argv[])
{
	FILE * fp;
	struct boot_info_t * bi;
	struct nx_tbbinfo *nbi;
	unsigned char nsih[512];
	char * buffer;
	int length, reallen;
	int nbytes, filelen;

	printf("%s", to_readable_msg(msg_copyright, sizeof(msg_copyright)));
    printf("%s", to_readable_msg(msg_forum, sizeof(msg_forum)));
    printf("%s", to_readable_msg(msg_tel, sizeof(msg_tel)));
#if 0
	if(argc != 5)
	{
		printf("Usage: mk6818 <destination> <nsih> <2ndboot> <bootloader>\n");
		return -1;
	}
#endif
	if(process_nsih("nsih.txt", &nsih[0]) != 512)
		return -1;

	length = 32 * 1024 * 1024;
	buffer = malloc(length);
	memset(buffer, 0, length);

	/* 2ndboot nsih */
	memcpy(&buffer[(SECBOOT_NSIH_POSITION - 1) * BLKSIZE], &nsih[0], 512);

	/* 2ndboot */
	fp = fopen("bl1-test.bin", "r+b");
	if(fp == NULL)
	{
		printf("Open file 2ndboot error\n");
		free(buffer);
		return -1;
	}

	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	nbytes = fread(&buffer[(SECBOOT_POSITION - 1) * BLKSIZE], 1, filelen, fp);
	if(nbytes != filelen)
	{
		printf("Read file 2ndboot error\n");
		free(buffer);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	/* fix 2ndboot nsih */
	bi = (struct boot_info_t *)(&buffer[(SECBOOT_NSIH_POSITION - 1) * BLKSIZE]);
	/* ... */
	bi->dbi.sdmmcbi.LoadDevice = 3;
	bi->dii.chipmask = 0x7c0;
	/* bootloader nsih */
	memcpy(&buffer[(BOOTLOADER_NSIH_POSITION - 1) * BLKSIZE], &nsih[0], 512);

	/* bootloader */
	fp = fopen("u-boot.bin", "r+b");
	if(fp == NULL)
	{
		printf("Open file bootloader error\n");
		free(buffer);
		return -1;
	}

	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	reallen = (BOOTLOADER_POSITION - 1) * BLKSIZE + filelen;
	fseek(fp, 0L, SEEK_SET);

	nbytes = fread(&buffer[(BOOTLOADER_POSITION - 1) * BLKSIZE], 1, filelen, fp);
	if(nbytes != filelen)
	{
		printf("Read file bootloader error\n");
		free(buffer);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	/* fix bootloader nsih */
	nbi = (struct nx_tbbinfo *)(&buffer[(BOOTLOADER_NSIH_POSITION - 1) * BLKSIZE]);
	nbi->loadsize = ((filelen + 512 + 512) >> 9) << 9;
	nbi->loadaddr =  0x43c00000;
	nbi->startaddr = 0x43c00000;

	/* destination */
	fp = fopen("ubootpak.bin", "w+b");
	if(fp == NULL)
	{
		printf("Destination file open error\n");
		free(buffer);
		return -1;
	}

	nbytes = fwrite(buffer, 1, reallen, fp);
	if(nbytes != reallen)
	{
		printf("Destination file write error\n");
		free(buffer);
		fclose(fp);
		return -1;
	}

	free(buffer);
	fclose(fp);

	printf("Generate destination file: %s\n", "ubootpak.bin");

	return 0;
}
