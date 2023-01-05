#include "msg.h"

int fila_esta_cheia(struct s_shm *shm);
void handle_connect(struct s_shm *shm, struct s_msg *msg);
void handle_shutdown(struct s_shm *shm, struct s_msg *msg);
void handle_procura(int msg_id, struct s_shm *shm, struct s_msg *msg);

int main(int argc, char *argv[]) {
	int mq1_id, mq2_id, shm_id;
	init(argc, argv, &mq1_id, &mq2_id, &shm_id);

	printf("Servidor inicializado (pid: %d)\n", getpid());
	printf(" - ID Fila de Mensagens 1: %d\n", mq1_id);
	printf(" - ID Fila de Mensagens 2: %d\n", mq2_id);
	printf(" - ID Memória Partilhada : %d\n", shm_id);

	int ret;
	msgStruct f_msg;
	struct s_msg msg;
	struct s_shm *shm = (struct s_shm *)shmat(shm_id, NULL, 0);

	// Inicializar a memória partilhada com slots "vazios" (-1)
	for (int i = 0; i < SHM_SIZE; i++) {
		shm->pids[i] = PID_VAZIO;
	}

	while (1) {
		ret = msgrcv(mq1_id, (struct msgbuf *)&msg, sizeof(msg) - sizeof(long), SRV_MSG_TYPE, IPC_NOWAIT);

		// Se uma mensagem foi encontrada
		if (ret != -1) {
			printf("Mensagem de %ld recebida: %s\n", msg.pid, msg.texto);

			if (str_starts_with(msg.texto, "connect")) {
				handle_connect(shm, &msg);
			} else if (str_starts_with(msg.texto, "procura")) {
				handle_procura(mq1_id, shm, &msg);
			} else if (str_starts_with(msg.texto, "shutdown")) {
				handle_shutdown(shm, &msg);
			} else if (str_starts_with(msg.texto, "resposta")) {
				puts(msg.texto);
			} else {
				printf("Erro: mensagem desconhecida\n");
			}
		}

		ret = msgrcv(mq2_id, (struct msgbuf *)&f_msg, sizeof(f_msg) - sizeof(long), SRV_MSG_TYPE, IPC_NOWAIT);

		// Se uma mensagem foi encontrada
		if (ret != -1) {
			printf("Ficheiro %s recebido\n", f_msg.nome);
		}
	}

	shmdt(shm);

	return 0;
}

void handle_connect(struct s_shm *shm, struct s_msg *msg) {
	if (fila_esta_cheia(shm)) {
		printf("Erro: fila cheia\n");
		return;
	}

	long pid = get_ultimo_pid_msg(msg->texto);
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
		if (shm->pids[i] == PID_VAZIO) {
			shm->pids[i] = pid;
			break;
		}
	}

	printf("Pid %ld conectado\n", pid);
}

void handle_shutdown(struct s_shm *shm, struct s_msg *msg) {
	long pid = get_ultimo_pid_msg(msg->texto);
	if (pid == -1)
		return;

	int removido = 0;

	// Remover pid da memoria compartilhada
	for (int i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == pid) {
			shm->pids[i] = PID_VAZIO;
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
	long pid = get_ultimo_pid_msg(msg->texto);
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

		if (c_pid != PID_VAZIO && c_pid != pid) {
			struct s_msg n_msg;

			msg->tipo = c_pid;

			n_msg = *msg;
			strcpy(n_msg.texto, msg->texto);

			ret = msgsnd(msg_id, (struct msgbuf *)&n_msg, sizeof(n_msg) - sizeof(long), 0);
			exit_on_error(ret, "Erro ao tentar enviar mensagem");
		}
	}
}

int fila_esta_cheia(struct s_shm *shm) {
	for (int i = 0; i < SHM_SIZE; i++) {
		if (shm->pids[i] == PID_VAZIO) {
			return 0;
		}
	}

	return 1;
}
