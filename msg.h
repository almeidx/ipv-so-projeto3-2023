#ifndef MSG_H
#define MSG_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#define SHM_KEY 42
#define PERMISSIONS 0666

#define SRV_MSG_TYPE 1
#define MAX_MSG_SIZE 100

#define PID_VAZIO -1

#define exit_on_error(s, m)                                                                                            \
	if (s < 0) {                                                                                                       \
		perror(m);                                                                                                     \
		exit(1);                                                                                                       \
	}

struct s_msg {
	long tipo;
	long pid;
	char texto[MAX_MSG_SIZE];
};

typedef struct {
	long type;
	char nome[256];
	char conteudo[1024];
} msgStruct;

struct s_file {
	char nome[256];
	char conteudo[1024];
};

#define SHM_SIZE 10

struct s_shm {
	long pids[SHM_SIZE];
};

int str_starts_with(const char *str, const char *prefix) {
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

long get_ultimo_pid_msg(char *texto) {
	char *pid = strrchr(texto, ' ');
	if (pid == NULL) {
		printf("Erro: pid não encontrado\n");
		return -1;
	}

	return atol(pid);
}

/* O último parâmetro será NULL no cliente */
void init(int argc, char *argv[], int *mq1_id, int *mq2_id, int *shm_id) {
	if (argc != 3) {
		printf("Erro: número de argumentos invalido\nUsage: %s <mq1_key> <mq2_key>", argv[0]);
		exit(EXIT_FAILURE);
	}

	int mq1_key = atoi(argv[1]);
	int mq2_key = atoi(argv[2]);

	int flags = PERMISSIONS;
	if (shm_id != NULL)
		flags = flags | IPC_CREAT;

	*mq1_id = msgget(mq1_key, flags);
	exit_on_error(*mq1_id, "Erro ao criar a fila de mensagens 1");

	*mq2_id = msgget(mq2_key, flags);
	exit_on_error(*mq2_id, "Erro ao criar a fila de mensagens 2");

	if (shm_id != NULL) {
		*shm_id = shmget(SHM_KEY, sizeof(struct s_shm), flags);
		exit_on_error(*shm_id, "Erro ao criar a memória compartilhada");
	}
}

#endif // MSG_H
