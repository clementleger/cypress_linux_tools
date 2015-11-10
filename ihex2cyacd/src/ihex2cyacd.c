#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>

#include <ihex2cyacd_cmdline.h>

#define MAX_IHEX_FILE_LENGTH	1024
#define MAX_IHEX_BIN_SIZE	(1 << 16)

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
	uint8_t data[MAX_IHEX_FILE_LENGTH], length, type;
	uint16_t addr;
	FILE *input_hex, *output_cyacd;
	struct cyacd_header_info *infos;
	size_t line_length = MAX_IHEX_FILE_LENGTH;
	int ret, i, line_empty;
	uint32_t cur_row_num;
	uint8_t crc;
	uint8_t *all_data;
	uint32_t last_addr = 0;

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
		printf("Failed to allocate data to read file\n");
		return 1;
	}

	all_data = calloc(1, MAX_IHEX_BIN_SIZE);
	if(!all_data) {
		printf("Failed to allocate all data\n");
		return 1;
	}

	infos = &header_infos[args_info.cpu_arg];

	while (getline(&line_ptr, &line_length, input_hex) > 0) {
		ret = parse_ihex_line(line_ptr, &length, &addr, &type, data);
		if (ret) {
			printf("Failed to parse ihex file\n");
			return 1;
		}
		line_length = MAX_IHEX_FILE_LENGTH;

		/* Skip non relevant line and bootloader reserved space one */
		if (type != 00 || addr < args_info.bootloader_size_arg)
			continue;

		for (i = 0; i < length; i++)
			all_data[addr + i] = data[i];

		if (addr > last_addr)
			last_addr = addr + length;
	}
	/* Write the output file */

	/* Add cyacd header */
	fprintf(output_cyacd, "%08X%02X00\r\n", infos->silicon_id, infos->silicon_rev);

	bootloader_text_rows = args_info.bootloader_size_arg / infos->flash_row_size;
	/* The result is not an integer, add one row */
	if (args_info.bootloader_size_arg % infos->flash_row_size)
		bootloader_text_rows++;

	cur_row_num = bootloader_text_rows;

	for (addr = cur_row_num * infos->flash_row_size; addr < last_addr; addr += infos->flash_row_size) {

		line_empty = 1;
		/* Is the line empty ? */
		for (i = 0; i < infos->flash_row_size; i++) {
			if (all_data[addr + i] != 0) {
				line_empty = 0;
				break;
			}
		}

		/* If so, skip it */
		if (line_empty) {
			cur_row_num++;
			continue;
		}

		fprintf(output_cyacd, ":00%04X%04X", cur_row_num, infos->flash_row_size);
		crc = cur_row_num & 0xFF;
		crc += (cur_row_num >> 8) & 0xFF;
		crc += infos->flash_row_size & 0xFF;
		crc += (infos->flash_row_size >> 8) & 0xFF;

		for (i = 0; i < infos->flash_row_size; i++) {
			crc += all_data[addr + i];
			fprintf(output_cyacd, "%02X", all_data[addr + i]);
		}

		crc = ~crc + 1;
		fprintf(output_cyacd, "%02X\r\n", crc);
		cur_row_num++;
	}

	fclose(input_hex);
	fclose(output_cyacd);
	

	return 0;
}
