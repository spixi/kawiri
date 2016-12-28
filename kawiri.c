
/*
* (C) 2016 Marius Spix <marius.spix@web.de>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <zlib.h>

#define APP_NAME "kawiri"
#define MAX_MESS_LENGTH 1024

uint8_t* array; //Bitmap used CRC32 values (needs 512 MiB)
int server_fd;
void (*die)(void);

int main(int, char**);
void die_stderr(void);
void die_syslog(void);
void handle_exit(void);
void start_deamon(void);

void die_stderr() {
	perror(APP_NAME);
	exit(EXIT_FAILURE);
}

void die_syslog() {
	syslog(LOG_ERR, strerror(errno));
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	atexit(handle_exit);

	if(argc != 2) {
		fprintf(stderr, "Usage: APP_NAME [port_number]\n");
		exit(EXIT_FAILURE);
	}

	long port = strtol(argv[1],NULL,10);
	if(errno || port < 1024 || port > 65535) {
		fprintf(stderr, "The port number must be between 1024 and 65535.\n");
		exit(EXIT_FAILURE);
	}

	start_deamon();

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
		die();

	struct sockaddr_in server;

	server.sin_family      = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port        = htons(port);

	if(bind(server_fd, (struct sockaddr *) &server, sizeof(server)) < 0)
		die();

	if(listen(server_fd, 64) < 0)
		die();

	array = calloc(1,0x20000000);

	if(array == NULL)
		die();

	while(1) {
		int client_fd = accept(server_fd, (struct sockaddr *)NULL, NULL);

		unsigned char buf[MAX_MESS_LENGTH];
		bzero( buf, MAX_MESS_LENGTH);
		int len = read(client_fd, buf, MAX_MESS_LENGTH);

		uint32_t checksum = crc32(0, buf, len);

		uint32_t offset = checksum >> 3;
		uint8_t bit = 1 << ( checksum & 7 );

		if(array[ offset ] & bit) {
			write(client_fd, "1", 1);
		}
		else {
			array[ offset ] |= bit;
			write(client_fd, "0", 1);
		}

		close(client_fd);
	}

	return 0;
}

void start_deamon() {
	   pid_t pid;

	   die = die_stderr;

	   if ((pid = fork()) < 0)
		   die();

	   if (pid > 0)
		   exit(EXIT_SUCCESS);

	   if (setsid() < 0)
		   die();

	   signal(SIGCHLD, SIG_IGN);
	   signal(SIGHUP, SIG_IGN);

	   if ((pid = fork()) < 0)
		   die();

	   if (pid > 0)
		   exit(EXIT_SUCCESS);

	   umask(0);

	   if (chdir("/"))
		   die();

       fclose(stdin);
       fclose(stdout);
       fclose(stderr);

       die = die_syslog;
       openlog(APP_NAME,LOG_PID,LOG_DAEMON);
}

void handle_exit() {
	shutdown(server_fd, SHUT_RD);
	close(server_fd);
	closelog();
	free(array);
}

