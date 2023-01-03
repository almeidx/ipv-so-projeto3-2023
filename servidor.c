#include "msg.h"

int main(void) {
	int msgid = msgget(MSG_KEY, PERMISSIONS | IPC_CREAT);

	if (msgid == -1) {
		perror("msgget");
		return 1;
	}

	printf("msgid: %d\n", msgid);

	struct s_msg msg;

	while (1) {
		if (msgrcv(msgid, &msg, sizeof(msg), 0, 0) == -1) {
			perror("msgrcv");
			return 1;
		}

		printf("Received: %s\n", msg.texto);
	}

	return 0;
}
