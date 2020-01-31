//shared libraries
#include <stdio.h>
#include <stdlib.h>

//sleep function
#include <unistd.h>

//daemon libraries
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

//thread libraries
#include <pthread.h>

//socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 1955

static void daemon_skeleton(void) {
	//pid_t is pid datatype for handling pid's
	pid_t pid;

	//fork the parent process
	pid = fork();

	//error checks:
	//if fork gives negative value, it was unsuccessful
	if (pid < 0)
		exit(EXIT_FAILURE);

	//if fork gives positive value, it was successful
	//if successful, kill the parent
	if (pid > 0)
		exit(EXIT_SUCCESS);

	//the child becomes session leader
	//returns -1 if error occurred
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	//signal handling
	//has to be filled out, either ignore og handle
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	//fork a second time to ensure no zombies
	pid = fork();

	//error checks:
	//if fork gives negative value, it was unsuccessful
	if (pid < 0)
		exit(EXIT_FAILURE);

	//if fork gives positive value, it was successful
	//if successful, kill the parent
	if (pid > 0)
		exit(EXIT_SUCCESS);

	//set file permissions
	//umask(0) = file is world-writeable
	umask(0);

	//change working directory
	chdir("/");

	//close all open file descriptors
	//SC_OPEN_MAX is max open files for a process
	int x = 1;
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
		close(x);
	}

	//open log file
	openlog("first daemon", LOG_PID, LOG_DAEMON);
}

void *socket_thread(void *socket_func) {
	int server_fd, new_socket, msgrecv;
	struct sockaddr_in adress;
	int opt;
	int addrlen = sizeof(adress);
	//{0} sets whole string to 0's
	char buffer[20] = { 0 };
	//greeting
	char *greeting = "Welcome to daemon server \n QUIT to close connection \n QUIT! to shutdown daemon \n";

	//create socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		syslog(LOG_NOTICE, "daemon erver start: file descriptor failed");
		exit(EXIT_FAILURE);
	} else {
		syslog(LOG_NOTICE, "daemon server start: file descriptor success");
	}

	//attaching socket to port "rules"
	//reuseaddr and reuseport is allow, opt is the boolean value
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
			sizeof(opt))) {
		syslog(LOG_NOTICE, "daemon server start: set socket options failed");
		exit(EXIT_FAILURE);
	} else {
		syslog(LOG_NOTICE, "daemon server start: set socket options success");
	}
	adress.sin_family = AF_INET;
	adress.sin_addr.s_addr = INADDR_ANY;
	adress.sin_port = htons(PORT);

	//attaching socket to port "bind"
	if (bind(server_fd, (struct sockaddr *) &adress, sizeof(adress)) < 0) {
		syslog(LOG_NOTICE, "daemon server start: bind failed");
		exit(EXIT_FAILURE);
	} else {
		syslog(LOG_NOTICE, "server start: bind success");
	}

	while (1) {
		if (listen(server_fd, 3) < 0) {
			syslog(LOG_NOTICE, "daemon server: server listen");
			exit(EXIT_FAILURE);
		}

		if ((new_socket = accept(server_fd, (struct sockaddr *) &adress,
				(socklen_t*) &addrlen)) < 0) {
			syslog(LOG_NOTICE, "Daemon server: client not accepted");
			exit(EXIT_FAILURE);
		} else {
			syslog(LOG_NOTICE, "Daemon server: client accepted");
			send(new_socket, greeting, strlen(greeting), 0);
		}

		while (1) {
			if ((msgrecv = recv(new_socket, buffer, 20, 0)) == -1) {
				syslog(LOG_NOTICE, "daemon server: message fail");
			} else {
				syslog(LOG_NOTICE, "daemon server: message recieved");
				send(new_socket, buffer, strlen(buffer), 0);
			}

			if (buffer[0] == 'Q') {
				if (buffer[1] == 'U') {
					if (buffer[2] == 'I') {
						if (buffer[3] == 'T') {
							break;
						}
					}
					{
						buffer[0] = '\0';
					}
				}
				{
					buffer[0] = '\0';
				}
			} else {
				buffer[0] = '\0';
			}
		}
		close(new_socket);

		if (buffer[4] == '!') {
			break;
		} else {
			buffer[0] = '\0';
		}
	}

	return NULL;
}

int main(void) {
	daemon_skeleton();

	syslog(LOG_NOTICE, "daemon started");

	pthread_t thread_id;

	pthread_create(&thread_id, NULL, socket_thread, NULL);

	pthread_join(thread_id, NULL);

	syslog(LOG_NOTICE, "Daemon server: shutting down");

	return EXIT_SUCCESS;
}
