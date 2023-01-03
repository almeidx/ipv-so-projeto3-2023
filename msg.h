#ifndef MSG_H
#define MSG_H

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define MSG_KEY 5
#define PERMISSIONS 0666

struct s_msg {
	long tipo;
	int pid;
	char texto[100];
};

#endif // MSG_H
