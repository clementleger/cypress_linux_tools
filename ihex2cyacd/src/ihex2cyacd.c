#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>

#include <ihex2cyacd_cmdline.h>

#define MAX_IHEX_FILE_LENGTH	512

#ifdef DEBUG
#define dbg_printf(fmt, args...)    printf(fmt, ## args)
#else
#define dbg_printf(fmt, args...)    /* Don't do anything in release builds */
#endif

struct cyacd_header_info {
	uint32_t silicon_id;
	uint8_t silicon_rev;
	uint16_t flash_row_size;
};

static struct cyacd_header_info header_infos[] = 
{
	[cpu_arg_CY8C41] = {0x04161193, 0x11, 128},
	[cpu_arg_CY8C42] = {0x04C81193, 0x11, 128},
};

static struct ihex2cyacd_args_info args_info;

#define get_line_chars(__str, __out_str, __n) 		\
	__out_str[__n] = 0;				\
	strncpy(__out_str, __str, __n);			\
	__str += __n;

static int parse_ihex_line(const char *line, uint8_t *length, uint16_t *addr, uint8_t *type, uint8_t data[MAX_IHEX_FILE_LENGTH])
{
	char tmp_buf[MAX_IHEX_FILE_LENGTH];
	int i;
	uint8_t crc, sum = 0;
	
	if(line[0] != ':') {
		printf("Missing field start\n");
		return 1;
	}
	line++;
	
	get_line_chars(line, tmp_buf, 2);
	*length = strtol(tmp_buf, NULL, 16);
	sum += *length;

	get_line_chars(line, tmp_buf, 4);
	*addr = strtol(tmp_buf, NULL, 16);
	sum += ((uint8_t) ((*addr) & 0xFF));
	sum += ((uint8_t) (((*addr) >> 8) & 0xFF));

	get_line_chars(line, tmp_buf, 2);
	*type = strtol(tmp_buf, NULL, 16);
	sum += *type;

	for( i = 0; i < *length; i++) {
		get_line_chars(line, tmp_buf, 2);
		data[i] = strtol(tmp_buf, NULL, 16);
		sum += data[i];
	}

	get_line_chars(line, tmp_buf, 2);
	crc = strtol(tmp_buf, NULL, 16);
	sum += crc;

	if (sum != 0) {
		printf("CRC failed\n");
		return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	uint32_t bootloader_text_rows;
	char *line_ptr;
	uint8_t data[MAX_IHEX_FILE_LENGTH],length, type;
	uint16_t addr;
	FILE *input_hex, *output_cyacd;
	struct cyacd_header_info *infos;
	size_t line_length = MAX_IHEX_FILE_LENGTH;
	int ret;

	if (ihex2cyacd_cmdline_parser(argc, argv, &args_info) != 0) {
		return EXIT_FAILURE;
	}

	input_hex = fopen(args_info.input_arg, "r");
	if (!input_hex) {
		printf("Failed to open input file %s\n", args_info.input_arg);
		return 1;
	}
	output_cyacd = fopen(args_info.output_arg, "w+");
	if (!output_cyacd) {
		printf("Failed to open output file %s\n", args_info.output_arg);
		return 1;
	}
	line_ptr = malloc(MAX_IHEX_FILE_LENGTH);
	if(!line_ptr) {
		printf("Failed to allocate data\n");
		return 1;
	}
	infos = &header_infos[args_info.cpu_arg];

	bootloader_text_rows = args_info.bootloader_size_arg / infos->flash_row_size;

	fprintf(output_cyacd, "%08x%02x00\r\n", infos->silicon_id, infos->silicon_rev);

	while( getline(&line_ptr, &line_length, input_hex) > 0) {
		ret = parse_ihex_line(line_ptr, &length, &addr, &type, data);
		if (ret) {
			printf("Failed to parse ihex file\n");
			return 1;
		}
		line_length = MAX_IHEX_FILE_LENGTH;
		/* TODO: concat data */
	}
	

	return 0;
}
