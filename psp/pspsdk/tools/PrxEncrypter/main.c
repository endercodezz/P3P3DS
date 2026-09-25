#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#include "types.h"
#include "endian_.h"
#include "kirk_engine.h"
#include "psp_headers.h"
#include "pspemu_pboot_headers.h"

typedef struct Header_List {
	unsigned char *pspHeader;
	unsigned char *kirkHeader;
} Header_List;

static Header_List psp_header_list[] = {
	{	psp_pspHeader_small2, psp_kirkHeader_small2	},
	{	psp_pspHeader_small , psp_kirkHeader_small	},
	{	psp_pspHeader_big   , psp_kirkHeader_big		},
};

static Header_List pspemu_pboot_header_list[] = {
	{ pspemu_pboot_pspHeader_smallest, pspemu_pboot_kirkHeader_smallest },
	{ pspemu_pboot_pspHeader_small,    pspemu_pboot_kirkHeader_small },
	{ pspemu_pboot_pspHeader_middle,   pspemu_pboot_kirkHeader_middle },
	{ pspemu_pboot_pspHeader_big,      pspemu_pboot_kirkHeader_big },
	{ pspemu_pboot_pspHeader_biggest,  pspemu_pboot_kirkHeader_biggest },
};

static u8 in_buffer[1024*1024*10];
static u8 out_buffer[1024*1024*10];

static u8 kirk_raw[1024*1024*10];
static u8 kirk_enc[1024*1024*10];
static u8 elf[1024*1024*10];

static u8 pspemu_pboot_flag = 0;

#define KIRK_BASE_SIZE (pspemu_pboot_flag ? 0x920 : 0x110)
#define PSP_HEADER_SIZE (pspemu_pboot_flag ? 0x960 : 0x150)
#define HEADER_LIST (pspemu_pboot_flag ? pspemu_pboot_header_list : psp_header_list)

typedef struct header_keys
{
    u8 AES[16];
    u8 CMAC[16];
}header_keys;

static int load_elf(char *elff)
{
	FILE *fp = fopen(elff, "rb");

	if(fp == NULL) {
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	size_t result = fread(elf, 1, size, fp);
	if (result < 0) {
		fprintf(stderr, "Error, could not read the ELF\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return size;
}

static int dumpFile(char *name, void *in, int size)
{
	FILE *fp = fopen(name, "wb");

	if(fp == NULL) {
		return -1;
	}

	fwrite(in, 1, size, fp);
	fclose(fp);

	return 0;
}

static int get_kirk_size(u8 *key_hdr)
{
	int krawSize = *(u32*)(key_hdr+0x70);

	if(krawSize % 0x10) {
		krawSize += 0x10 - (krawSize % 0x10); // 16 bytes aligned
	}

	krawSize += KIRK_BASE_SIZE;

	return krawSize;
}

static Header_List *get_header_list(int size)
{
	Header_List *header_list = pspemu_pboot_flag 
		? pspemu_pboot_header_list
		: psp_header_list;

	size_t header_list_size = pspemu_pboot_flag
		? sizeof(pspemu_pboot_header_list)/sizeof(pspemu_pboot_header_list[0])
		: sizeof(psp_header_list)/sizeof(psp_header_list[0]);
	
	for(int i=0; i<header_list_size; i++) {
		int h_size = get_kirk_size(header_list[i].kirkHeader);
		h_size -= PSP_HEADER_SIZE;

		if( h_size >= size ) {
			return &header_list[i];
		}
	}

	return NULL;
}

static int is_compressed(u8 *psp_header)
{
	if (*(u16*)(psp_header+6) == 1) {
		return 1;
	}

	return 0;
}

static int get_elf_size(u8 *psp_header)
{
	return *(u32*)(psp_header+0x28);
}

static int gzip_compress(u8 *dst, const u8 *src, int size)
{
	int ret;
	z_stream strm;
	u8 *elf_compress;
	const int compress_max_size = 10 * 1024 * 1024;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	elf_compress = malloc(compress_max_size);

	if(elf_compress == NULL) {
		return -1;
	}

	ret = deflateInit2(&strm, 9, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY);

	if(ret != Z_OK) {
		printf("%s: compress error\n", __func__);
		free(elf_compress);
		
		return -2;
	}

	strm.avail_in = size;
	strm.next_in = (void*)src;
	strm.avail_out = compress_max_size;
	strm.next_out = elf_compress;

	ret = deflate(&strm, Z_FINISH);

	if(ret == Z_STREAM_ERROR) {
		deflateEnd(&strm);
		printf("%s: compress error\n", __func__);
		free(elf_compress);

		return -3;
	}

	memcpy(dst, elf_compress, strm.total_out);
	deflateEnd(&strm);
	free(elf_compress);

	return 0;
}

int main(int argc, char **argv)
{
	header_keys keys;
	u8 rawkheaderBk[0x90];
	char *in_fname = NULL;
	char *out_fname = NULL;

	if (argc < 2)
	{
		printf("USAGE: %s [--pspemu-pboot] input_prx [output_prx]\n", argv[0]);
		return 0;
	}

	if (strcmp(argv[1], "--pspemu-pboot") == 0) {
		pspemu_pboot_flag = 1;

		if (argc < 3) {
			printf("USAGE: %s [--pspemu-pboot] input_prx [output_prx]\n", argv[0]);
			return 0;
		}

		in_fname = argv[2];
		out_fname = argc > 3 ? argv[3] : "./DATA.PSP";
	} else {
		in_fname = argv[1];
		out_fname = argc > 2 ? argv[2] : "./DATA.PSP";
	}

	memset(in_buffer, 0, sizeof(in_buffer));
	memset(out_buffer, 0, sizeof(out_buffer));
	memset(kirk_raw, 0, sizeof(kirk_raw));
	memset(kirk_enc, 0, sizeof(kirk_enc));
	memset(elf, 0, sizeof(elf));

	kirk_init();

	int elfSize = load_elf(in_fname);

	if(elfSize < 0) {
		printf("Cannot open %s\n", in_fname);
		return 0;
	}

	Header_List *target_header = get_header_list( elfSize );

	if( target_header == NULL ) {
		printf("PRX SIGNER: Elf is to big\n");

		return 0;
	}

	u8 *kirkHeader	= target_header->kirkHeader;
	u8 *pspHeader	= target_header->pspHeader;
	int krawSize = get_kirk_size(kirkHeader);

	if (is_compressed(pspHeader)) {
		elfSize = get_elf_size(pspHeader);
		gzip_compress(elf, elf, elfSize);
	}

	memcpy(kirk_raw, kirkHeader, KIRK_BASE_SIZE);
	memcpy(rawkheaderBk, kirk_raw, sizeof(rawkheaderBk));

	kirk_decrypt_keys((u8*)&keys, kirk_raw);
	memcpy(kirk_raw, &keys, sizeof(header_keys));
	memcpy(kirk_raw+KIRK_BASE_SIZE, elf, elfSize);

	if(kirk_CMD0(kirk_enc, kirk_raw, sizeof(kirk_enc), 0) != 0)
	{
		printf("PRX SIGNER: Could not encrypt elf\n");
		return 0;
	}

	memcpy(kirk_enc, rawkheaderBk, sizeof(rawkheaderBk));

	if(kirk_forge(kirk_enc, sizeof(kirk_enc)) != 0)
	{
		printf("PRX SIGNER: Could not forge cmac block\n");

		return 0;
	}

	memcpy(out_buffer, pspHeader, PSP_HEADER_SIZE);
	memcpy(out_buffer+PSP_HEADER_SIZE, kirk_enc+KIRK_BASE_SIZE, krawSize-KIRK_BASE_SIZE);

	return dumpFile(out_fname, out_buffer, (krawSize-KIRK_BASE_SIZE)+PSP_HEADER_SIZE);
}
