#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <poll.h>
#include <errno.h>

#include <cybtldr_api.h>
#include <cybtldr_api2.h>

#include <cyhostboot_cmdline.h>


#ifdef DEBUG
#define dbg_printf(fmt, args...)    printf(fmt, ## args)
#else
#define dbg_printf(fmt, args...)    /* Don't do anything in release builds */
#endif

#define TRANSFER_SIZE	64

static struct cyhostboot_args_info args_info;

/**
 * No context for callback itnerface... use a shared var.
 */
static int g_serial_fd = -1;

static unsigned long long timespec_milliseconds(struct timespec *a) 
{
	return a->tv_sec*1000 + a->tv_nsec/1000000;
}

static speed_t get_serial_speed(int baudrate)
{
	switch (baudrate) {
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		default:
			printf("Invalid baudrate %d\n", baudrate);
			exit(EXIT_FAILURE);
	};
}

static int serial_open()
{
	struct termios tty;
	speed_t baudrate = get_serial_speed(args_info.baudrate_arg);

	dbg_printf("Opening serial %s\n", args_info.serial_arg);

	g_serial_fd = open(args_info.serial_arg, O_RDWR | O_NOCTTY);
	if (g_serial_fd < 0) {
		printf("Failed to open serial: %s\n", strerror(errno));
		return 1;
	}

	if(tcgetattr(g_serial_fd, &tty) < 0) {
		printf( "Failed to get serial port attribute\n");
		return 1;
	}

	cfmakeraw(&tty);
	/* Set Baud Rate */
	cfsetospeed (&tty, baudrate);
	cfsetispeed (&tty, baudrate);

	tcflush(g_serial_fd, TCIFLUSH);
	tcflush(g_serial_fd, TCOFLUSH);

	if (tcsetattr(g_serial_fd, TCSANOW, &tty) < 0) {
		printf( "Failed to set serial port attribute\n");
		return 1;
	}

	return CYRET_SUCCESS;
}

static int serial_close()
{
	dbg_printf("Closing serial\n");
	close(g_serial_fd);

	return CYRET_SUCCESS;
}

static int serial_read(unsigned char *bytes, int size)
{
	struct timespec tp;
	unsigned long long start_milli = 0, end_milli = 0;
	ssize_t read_bytes;
	struct pollfd fds[1];
	int poll_ret, i;
	unsigned int cur_byte = 0;

	while(1) {
		fds[0].revents = 0;
		fds[0].events = POLLIN | POLLPRI;
		fds[0].fd = g_serial_fd;

		clock_gettime(CLOCK_MONOTONIC, &tp);
		end_milli = timespec_milliseconds(&tp);
		/* If we have read at least one value, start timer before ending read */
		if (start_milli && (end_milli - start_milli) > 100)
			break;

		/* Check if there is data to read from serial */
		poll_ret = poll(fds, 1, 0);
		if (poll_ret == 0) {
			continue;
		} else if (poll_ret < 0) {
			printf("Poll error: %s\n", strerror(errno));
			return 1;
		}

		/* Ok, we read some data, start the stop timer */
		clock_gettime(CLOCK_MONOTONIC, &tp);
		start_milli = timespec_milliseconds(&tp);

		read_bytes = read(g_serial_fd, &bytes[cur_byte++], 1);
		if (read_bytes != 1) {
			return 1;
		}
	}
	dbg_printf("Read %d bytes\n", cur_byte);
	for(i = 0; i < cur_byte; i++)
		dbg_printf(" 0x%02x ", bytes[i]);
	dbg_printf("\n");

	return CYRET_SUCCESS;
}

static int serial_write(unsigned char *bytes, int size)
{
	int i;
	ssize_t write_bytes;

	dbg_printf("Serial: writing %d bytes to bootloader\n", size);
	for(i = 0; i< size; i++)
		dbg_printf(" 0x%02x ", bytes[i]);
	dbg_printf("\n");
	write_bytes = write(g_serial_fd, bytes, size);
	if (write_bytes != size) {
		printf("Error when writing bytes\n");
		return 1;
	}

	return CYRET_SUCCESS;
}


static CyBtldr_CommunicationsData serial_coms = {
	.OpenConnection = serial_open,
	.CloseConnection = serial_close,
	.ReadData = serial_read,
	.WriteData = serial_write,
	.MaxTransferSize = 64,
};


static void serial_progress_update(unsigned char arrayId, unsigned short rowNum)
{
	printf("Progress: array_id %d, row_num %d\n", arrayId, rowNum);
}

int main(int argc, char **argv)
{
	int ret, action = PROGRAM;
	const char *action_str = "programing";

	if (cyhostboot_cmdline_parser(argc, argv, &args_info) != 0) {
		return EXIT_FAILURE;
	}

	if (args_info.erase_given) {
		action = ERASE;
		action_str = "erasing";
	} else if (args_info.verify_given) {
		action = VERIFY;
		action_str = "verifying";
	}

	if (action == PROGRAM)
		printf("Programing file %s\n", args_info.file_arg);

	printf("Start %s on serial %s, baudrate %d\n", action_str, args_info.serial_arg, args_info.baudrate_arg);
	ret = CyBtldr_RunAction(action, args_info.file_arg, NULL, 1, &serial_coms, serial_progress_update);
	if (ret != CYRET_SUCCESS) {
		printf("%s failed: %d\n", action_str, ret);
		return 1;
	}
	printf("%s OK !\n", action_str);

	return 0;
}
