/* encoding: UTF-8 */

/* standard libs */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>    /* custom signal handler */
#include <errno.h>     /* error handling */
#include <sys/wait.h>  /* child process management */

/* networking */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT     61234
#define BUF_SIZE    32


static char *buf;

static int server_sock;
static int client_sock;

static struct sockaddr_in *server_addr;
static struct sockaddr_in *client_addr;

static 	pid_t pid;



void handle_signal(int signal)
{
	fprintf(stdout, "\n");
	fflush(stdout);

	free(buf);
	free(server_addr);
	free(client_addr);

	close(server_sock);
	close(client_sock);

	exit(EXIT_SUCCESS);
}


void handle_child(int signal)
{
	waitpid(pid, NULL, 0);
}


void die(const char *func, const char *err_msg)
{
	fprintf(stderr, "Error @ %s:\n"
	                "%s\n", func, err_msg);

	perror("Errno:\n");
	fflush(stderr);


	free(buf);
	free(server_addr);
	free(client_addr);

	close(server_sock);
	close(client_sock);

	exit(EXIT_FAILURE);
}


void launch(void)
{
	int ret;

	char *argv[] = {"firefox", "www.google.de", NULL};

	pid = fork();

	if (pid == 0) { /* child process */
		ret = execvp("firefox", argv);

		if (ret < 0)
			die("launch", "Couldn't execute command");

	} else { /* parent process */
		waitpid(pid, NULL, WNOHANG);
	}
}



int main(void)
{
	int ret;
	size_t sockaddr_size;

	/* register custom signal handler for clean exit using CTRL-C */
	if (signal(SIGINT, handle_signal) == SIG_ERR)
		die("main", "Couldn't register custom signal handler");

	/* register custom signal handler for collection zombie children */
	if (signal(SIGCHLD, handle_child) == SIG_ERR)
		die("main", "Couldn't register custom signal handler");


	/* prepare buffer */
	buf = (char *) malloc(BUF_SIZE);

	if (buf == NULL)
		die("main", "Couldn't alloc mem for buffer");

	bzero(buf, BUF_SIZE);


	/* prepare server address */
	sockaddr_size = sizeof(struct sockaddr_in);
	server_addr   = (struct sockaddr_in *) malloc(sockaddr_size);

	if (server_addr == NULL)
		die("main", "Couldn't alloc mem for server address");

	bzero(server_addr, sockaddr_size);

	server_addr->sin_family = AF_INET;
	server_addr->sin_port   = htons(PORT);
	server_addr->sin_addr.s_addr = INADDR_ANY;


	/* prepare client address */
	client_addr = (struct sockaddr_in *) malloc(sockaddr_size);

	if (client_addr == NULL)
		die("main", "Couldn't alloc mem for client address");

	bzero(client_addr, sockaddr_size);


	/* open socket */
	server_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (server_sock < 0)
		die("main", "Couldn't open socket");


	/* bind socket to server addr */
	ret = bind(server_sock,
	           (struct sockaddr *) server_addr,
	           sockaddr_size
	      );

	if (ret < 0)
		die("main", "Couldn't bind socket to server addr");


	/* start listening for incoming connections */
	listen(server_sock, 1);


	/* serve forever */
	while (1) {

		/* get incoming connection */
		client_sock = accept(server_sock,
		                     (struct sockaddr *) client_addr,
		                     (socklen_t *) &sockaddr_size
		                    );

		launch();

		close(client_sock);
	}

	/* clean up */
	free(server_addr);
	free(client_addr);

	close(server_sock);
	close(client_sock);

	return EXIT_SUCCESS;
}
