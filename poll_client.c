/* encoding: UTF-8 */

/* standard libs */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <strings.h>

/* libnfc */
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

/* networking */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define POLL_AMOUNT    20
#define POLL_DURATION   2
#define POLL_CONST    150

#define SLEEP   1

#define BUF_SIZE 32
//#define HOST "127.0.0.1"
#define HOST "192.168.2.102"
#define PORT 61234


static nfc_context *context;
static nfc_device *device;

static int socket_fd;
static char *buf;
static struct sockaddr_in *server_addr;


/* Prints error message and dies */
static void die(char *func, char *msg)
{
	if (func == NULL || msg == NULL) {
		fprintf(stderr, "Error @ die():\n"
		                "Given func or msg pointer was NULL\n");
		fflush(stderr);

		nfc_close(device);
		nfc_exit(context);

		close(socket_fd);
		free(buf);
		free(server_addr);

		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Error @ %s():\n%s\n\n", func, msg);
	perror("Errno contained:\n");
	fflush(stderr);

	nfc_close(device);
	nfc_exit(context);

	close(socket_fd);
	free(buf);
	free(server_addr);

	exit(EXIT_FAILURE);
}


/* Custom signal handler for a clean exit, in case SIGINT is received */
static void stop_polling(int signal)
{
	if (device != NULL)
		nfc_abort_command(device);
	else {
		nfc_close(device);
		nfc_exit(context);

		close(socket_fd);
		free(buf);
		free(server_addr);

		exit(EXIT_SUCCESS);
	}
}


static void handle_tag(void)
{
	int ret;
	size_t sockaddr_size;

	/* prepare buffer */
	buf = (char *) malloc(BUF_SIZE);

	if (buf == NULL)
		die("handle_tag", "Couldn't alloc mem for buffer");

	bzero(buf, BUF_SIZE);


	/* init server address */
	sockaddr_size = sizeof(struct sockaddr_in);
	server_addr   = (struct sockaddr_in *) malloc(sockaddr_size);

	if (server_addr == NULL)
		die("handle_tag", "Couldn't alloc mem for server addr");

	bzero(server_addr, sockaddr_size);

	server_addr->sin_family      = AF_INET;
	server_addr->sin_port        = htons(PORT);
	server_addr->sin_addr.s_addr = inet_addr(HOST);


	/* open socket */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0)
		die("handle_tag", "Couldn't open socket");


	/* connect to the socket */
	ret = connect(socket_fd,
	              (struct sockaddr *) server_addr,
	              sockaddr_size
	      );
	if (ret < 0)
		die("handle_tag", "Couldn't connect to socket");

	free(buf);
	free(server_addr);

	close(socket_fd);
}


int main(void)
{
	int res;

	uint8_t poll_amount;
	uint8_t poll_duration;


	/* custom signal handler for clean exit */
	if (signal(SIGINT, stop_polling) == SIG_ERR)
		die("main", "Couldn't register custom signal handler");

	/*
	 * list of tag types that we are going to poll for. Right now we
	 * are only planning on using ISO-14443A (Mifare Classic) tags
	 */
	nfc_modulation tag_types[1];
	size_t tag_types_size;

	nfc_target target;

	context = NULL;
	device  = NULL;


	poll_amount   = POLL_AMOUNT;
	poll_duration = POLL_DURATION;

	tag_types[0].nmt = NMT_ISO14443A;  /* modulation type */
	tag_types[0].nbr = NBR_106;        /* baud rate */

	tag_types_size   = 1;


	nfc_init(&context);
	if (context == NULL)
		die("main", "Couldn't init libnfc");

	device = nfc_open(context, NULL);
	if (device == NULL)
		die("main", "Couldn't open device");

	res = nfc_initiator_init(device);
	if (res < 0)
		die("main", "Couldn't init our device as a reader");


	fprintf(stdout, "Reader: %s opened\n", nfc_device_get_name(device));
	fprintf(stdout, "Will be polling for %d ms\n"
	                "(%d pollings of %d ms for %d modulations)\n",
	      poll_amount * poll_duration * (int) tag_types_size * POLL_CONST,
	      poll_amount, poll_duration * POLL_CONST, (int) tag_types_size);

	fflush(stdout);


	while (1) {
		fprintf(stdout, "polling ...\n");
		fflush(stdout);

		res = nfc_initiator_poll_target(device, tag_types,
		         tag_types_size, poll_amount, poll_duration, &target);

		if (res < 0) {
			break;

		} else if (res == 0) {
			fprintf(stdout, "No target found\n\n");
			fflush(stdout);

		} else { /* tag was found */
			printf("target found!\n\n");
			handle_tag();
			sleep(SLEEP);
		}

		sleep(SLEEP);
	}


	nfc_close(device);
	nfc_exit(context);

	close(socket_fd);
	free(buf);
	free(server_addr);

	return EXIT_SUCCESS;
}
