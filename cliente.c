#include "msg.h"

int main(void) {
	pid_t pid_c2 = fork(), pid = getpid();

	if (pid_c2 == 0) {

	} else {
		int msgid = msgget(MSG_KEY, PERMISSIONS);

		if (msgid == -1) {
			perror("msgget");
			return 1;
		}

		printf("msgid: %d\n", msgid);

		struct s_msg msg;

		while (1) {
			scanf(" %[^\n]s", msg.texto);

			msg.tipo = 1;
			msg.pid = pid;

			if (strcmp(msg.texto, "connect") == 0) {
				strcat(msg.texto, " ");

				// convert pid to string
				char pid_str[10];
				sprintf(pid_str, "%d", pid);

				strcat(msg.texto, pid_str);
			}

			if (msgsnd(msgid, (struct msgbuf *)&msg, sizeof(msg), 0) == -1) {
				perror("msgsnd");
				return 1;
			}

			printf("Sent: %s\n", msg.texto);
		}
	}

	return 0;
}
