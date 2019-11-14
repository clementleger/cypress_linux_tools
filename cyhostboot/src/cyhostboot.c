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
	speed_t baudrate = get_serial_speed(args_info.baudrate_arg);

	g_serial_fd = open( args_info.serial_arg, O_RDWR );
	if( g_serial_fd < 0 )
	{
		printf("Unable to open device: %s\n", args_info.serial_arg );
		return 1;
	}
	// setting default baud rate and attributes
	struct termios port_settings;
	memset( &port_settings, 0, sizeof( port_settings ) );
	cfsetispeed( &port_settings, baudrate );
	cfsetospeed( &port_settings, baudrate );
	if( args_info.odd_given )
	{
		printf( "odd parity\n");
		port_settings.c_cflag |= PARENB; // enable parity
		port_settings.c_cflag |= PARODD; // enable odd parity => enable odd parity
	}
	else if( args_info.even_given )
	{
		printf( "even parity\n");
		port_settings.c_cflag |= PARENB; // enable parity
		port_settings.c_cflag &= ~PARODD; // disable odd parity => enable even parity
	}
	else
	{
		printf( "no parity\n" );
		port_settings.c_cflag &= ~PARENB; // disable parity
	}
	port_settings.c_cflag &= ~CSTOPB; // disable extra stop bit => one stop bit
	port_settings.c_cflag |= CS8; // 8 bits per byte
	port_settings.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control
	port_settings.c_cflag |= CREAD | CLOCAL; // turn on read and disable ctrl lines
	port_settings.c_lflag &= ~ICANON; // disable canonical mode
	port_settings.c_lflag &= ~(ECHO | ECHOE | ECHONL); // disable any kind of echo
	port_settings.c_lflag &= ~ISIG; // disable interruption of INTR, QUIT and SUSP
	port_settings.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off sw flow control
	port_settings.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	port_settings.c_oflag &= ~(OPOST); // Prevent special interpretation of output bytes (e.g. newline chars)
	port_settings.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	port_settings.c_cc[VTIME] = 0; // do not wait, return immediately
	port_settings.c_cc[VMIN] = 0; // return as soon as some data
	if( tcsetattr( g_serial_fd, TCSAFLUSH, &port_settings ) != 0 )
	{
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
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


static unsigned char hex_decode( unsigned char v )
{
	unsigned char rv = 0xff;

	if( v < '0' )
	{
		printf ( "error: illegal hex character %c\n", v );
	}
	else if ( v <= '9' )
	{
		rv = v - '0';
	}
	else if ( v < 'A' )
	{
		printf ("error: illegal hex character %c\n", v );
	}
	else if ( v <= 'F' )
	{
		rv = v - 'A' + 10;
	}
	else if ( v < 'a' )
	{
		printf( "error: illegal hex character %c\n",v );
	}
	else if ( v <= 'f' )
	{
		rv = v- 'a' + 10;
	}
	else
	{
		printf( "error: illegal hex character %c\n",v );
	}
	return rv;
}

static void serial_progress_update(unsigned char arrayId, unsigned short rowNum)
{
	printf("Progress: array_id %d, row_num %d\n", arrayId, rowNum);
}

unsigned char sec_key[6];

int main(int argc, char **argv)
{
	int ret, action = PROGRAM;
	const char *action_str = "programing";
	unsigned char *key = NULL;

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

	if (args_info.key_given )
	{
		for( int i=0; i<12; i+=2 )
		{
			unsigned char k = 0;
			unsigned char *c = (unsigned char *)&args_info.key_arg[i];

			k|=hex_decode( c[0] )<<4;
			k|=hex_decode( c[1] );

			sec_key[i>>1]=k;

			key = sec_key;
		}
	}

	printf("Start %s on serial %s, baudrate %d\n", action_str, args_info.serial_arg, args_info.baudrate_arg);
	ret = CyBtldr_RunAction(action, args_info.file_arg, key, 1, &serial_coms, serial_progress_update);
	if (ret != CYRET_SUCCESS) {
		printf("%s failed: %d\n", action_str, ret);
		return 1;
	}
	printf("%s OK !\n", action_str);

	return 0;
}
