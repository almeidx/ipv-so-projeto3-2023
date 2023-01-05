#include "servidor.h"
#include "msg.h"

int main(int argc, char *argv[]) {
	int mq1_id, mq2_id, shm_id;
	init(argc, argv, &mq1_id, &mq2_id, &shm_id);

	printf("Servidor inicializado (pid: %d)\n", getpid());
	printf(" - ID Fila de Mensagens 1: %d\n", mq1_id);
	printf(" - ID Fila de Mensagens 2: %d\n", mq2_id);
	printf(" - ID Memória Partilhada : %d\n", shm_id);

	int ret;
	struct s_msg msg;
	msgStruct f_msg;
	struct s_shm *shm = (struct s_shm *)shmat(shm_id, NULL, 0);

	for (int i = 0; i < SHM_SIZE; i++) {
		shm->pids[i] = EMPTY_PID;
	}

	while (1) {
		ret = msgrcv(mq1_id, (struct msgbuf *)&msg, sizeof(msg) - sizeof(long), SRV_MSG_TYPE, IPC_NOWAIT);

		if (ret != -1) {
			printf("Mensagem de %ld recebida: %s\n", msg.pid, msg.texto);

			if (str_starts_with(msg.texto, "connect")) {
				handle_connect(shm, &msg);
			} else if (str_starts_with(msg.texto, "procura")) {
				handle_procura(mq1_id, shm, &msg);
			} else if (str_starts_with(msg.texto, "quero")) {
				handle_quero(mq1_id, shm, &msg);
			} else if (str_starts_with(msg.texto, "shutdown")) {
				handle_shutdown(shm, &msg);
			} else if (str_starts_with(msg.texto, "resposta")) {
				printf("%s\n", msg.texto);
			} else {
				printf("Erro: mensagem desconhecida\n");
			}
		}

		ret = msgrcv(mq2_id, (struct msgbuf *)&f_msg, sizeof(f_msg) - sizeof(long), SRV_MSG_TYPE, IPC_NOWAIT);

		if (ret != -1) {
			printf("Ficheiro %s recebido\n", f_msg.nome);
		}
	}

	shmdt(shm);

	return 0;
}

void handle_connect(struct s_shm *shm, struct s_msg *msg) {
	if (queue_full(shm)) {
		printf("Erro: fila cheia\n");
		return;
	}

	long pid = get_last_pid_from_msg(msg->texto);
	if (pid == -1)
		return;

	int ja_conectado = 0;
	int i;

	// Verificar se pid já está conectado
	for (i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == pid) {
			ja_conectado = 1;
			break;
		}
	}

	if (ja_conectado) {
		printf("Erro: pid %ld já conectado\n", pid);
		return;
	}

	// Adicionar pid na memoria compartilhada
	for (i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == EMPTY_PID) {
			shm->pids[i] = pid;
			break;
		}
	}

	printf("Pid %ld conectado\n", pid);
}

void handle_shutdown(struct s_shm *shm, struct s_msg *msg) {
	long pid = get_last_pid_from_msg(msg->texto);
	if (pid == -1)
		return;

	int removido = 0;
	int i;

	// Remover pid da memoria compartilhada
	for (i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == pid) {
			shm->pids[i] = EMPTY_PID;
			removido = 1;
			break;
		}
	}

	if (!removido) {
		printf("Erro: pid %ld não encontrado\n", pid);
		return;
	}

	kill(pid, SIGUSR1);

	printf("Pid %ld desconectado\n", pid);
}

void handle_procura(int msg_id, struct s_shm *shm, struct s_msg *msg) {
	long pid = get_last_pid_from_msg(msg->texto);
	if (pid == -1)
		return;

	int i;
	int conectado = 0;
	int qnt_outros_clientes = 0;

	// Verificar se pid está conectado
	for (i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == pid) {
			conectado = 1;
		} else if (shm->pids[i] != 0) {
			qnt_outros_clientes++;
		}
	}

	if (qnt_outros_clientes == 0) {
		printf("Erro: não há outros clientes conectados\n");
		return;
	}

	if (!conectado) {
		printf("Erro: pid %ld não conectado\n", pid);
		return;
	}

	int ret;

	// Enviar mensagem para os outros clientes
	for (i = 0; i < SHM_SIZE; i++) {
		long c_pid = shm->pids[i];

		if (c_pid != EMPTY_PID && c_pid != pid) {
			struct s_msg n_msg;

			msg->tipo = c_pid;

			n_msg = *msg;
			strcpy(n_msg.texto, msg->texto);

			ret = msgsnd(msg_id, (struct msgbuf *)&n_msg, sizeof(n_msg) - sizeof(long), 0);
			exit_on_error(ret, "Erro ao tentar enviar mensagem");

			printf("msg enviada para %ld: %s\n", n_msg.tipo, n_msg.texto);
		}
	}
}

void handle_quero(int msg_id, struct s_shm *shm, struct s_msg *msg) {}

int queue_full(struct s_shm *shm) {
	for (int i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == EMPTY_PID) {
			return 0;
		}
	}

	return 1;
}
