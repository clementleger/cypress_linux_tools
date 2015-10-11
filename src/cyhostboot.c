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


#ifdef DEBUG
#  define dbg_printf printf
#else
#  define dbg_printf
#endif

static char* g_tty_device = "/dev/ttyACM2";
static int g_serial_fd = -1;

unsigned long long timespec_milliseconds(struct timespec *a) 
{
	return a->tv_sec*1000 + a->tv_nsec/1000000;
}

static int serial_open()
{
	struct termios tty;

	dbg_printf("Opening serial %s\n", g_tty_device);

	g_serial_fd = open(g_tty_device, O_RDWR | O_NOCTTY);
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
	cfsetospeed (&tty, (speed_t)B115200);
	cfsetispeed (&tty, (speed_t)B115200);

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
		if (start_milli && (end_milli - start_milli) > 100)
			break;

		poll_ret = poll(fds, 1, 0);
		if (poll_ret == 0) {
			continue;
		} else if (poll_ret < 0) {
			printf("Poll error: %s\n", strerror(errno));
			return 1;
		}
		
		clock_gettime(CLOCK_MONOTONIC, &tp);
		start_milli = timespec_milliseconds(&tp);

		read_bytes = read(g_serial_fd, &bytes[cur_byte++], 1);
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


CyBtldr_CommunicationsData serial_coms = {
	.OpenConnection = serial_open,
	.CloseConnection = serial_close,
	.ReadData = serial_read,
	.WriteData = serial_write,
	.MaxTransferSize = 64,
};


void serial_progress_update(unsigned char arrayId, unsigned short rowNum)
{
	printf("Flashing array_id %d, row_num %d\n", arrayId, rowNum);
}

int main(int argc, char **argv)
{
	int ret;
	g_tty_device = argv[2];
	
	printf("Starting programming\n");
	ret = CyBtldr_RunAction(PROGRAM, argv[1], NULL, 1, &serial_coms, serial_progress_update);
	if (ret != CYRET_SUCCESS) {
		printf("Programming failed: %d\n", ret);
		return 1;
	}
	printf("programming OK !\n");
	return 0;
}
