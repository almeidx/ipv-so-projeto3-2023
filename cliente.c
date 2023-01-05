#include "msg.h"
#include <dirent.h>

int filtro_fich(const struct dirent *entry);
void tratar_sigusr1();
void pid_to_string(long pid, char *str);
char *get_nome_fich_args(struct s_msg *msg);
int contar_n_args(char *str);

int main(int argc, char *argv[]) {
	int mq1_id, mq2_id;
	init(argc, argv, &mq1_id, &mq2_id, NULL);

	long pid_c2 = (long)fork(), pid = (long)getpid();

	if (pid_c2 == 0) { // C2
		// Tratar o sinal SIGUSR1
		struct sigaction sa;
		sa.sa_handler = tratar_sigusr1;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGUSR1, &sa, NULL);

		// Ler até 20 ficheiros na pasta atual
		struct dirent **namelist;
		int n = scandir(".", &namelist, filtro_fich, alphasort);
		exit_on_error(n, "Erro ao tentar ler a pasta atual");

		int n_ficheiros = 0;
		struct s_file ficheiros[20];

		while (n--) {
			if (n_ficheiros < 20) {
				strcpy(ficheiros[n_ficheiros].nome, namelist[n]->d_name);

				FILE *f = fopen(namelist[n]->d_name, "r");
				exit_on_error(f, "Erro ao tentar abrir o ficheiro");

				fread(ficheiros[n_ficheiros].conteudo, 1024, 1, f);
				fclose(f);

				n_ficheiros++;
			}

			free(namelist[n]);
		}

		free(namelist);

		printf("[C2] Número de ficheiros lidos: %d\n", n_ficheiros);

		int ret;
		struct s_msg msg;
		msgStruct msg_f;
		long pid_c2_real = (long)getpid();

		while (1) {
			ret = msgrcv(mq1_id, (struct msgbuf *)&msg, sizeof(msg) - sizeof(long), pid_c2_real, 0);
			exit_on_error(ret, "Erro ao tentar receber mensagem");

			if (str_starts_with(msg.texto, "procura")) {
				long pid_req = get_ultimo_pid_msg(msg.texto);
				if (pid_req == -1)
					continue;

				// Exemplo de uso: procura abcde 1111
				char *filename = get_nome_fich_args(&msg);
				if (filename == NULL)
					continue;

				for (int i = 0; i < n_ficheiros; i++) {
					if (strcmp(ficheiros[i].nome, filename) == 0) {
						char pid_str[10];
						pid_to_string(pid_c2_real, pid_str);

						strcpy(msg.texto, "resposta ");
						strcat(msg.texto, ficheiros[i].nome);
						strcat(msg.texto, " ");
						strcat(msg.texto, pid_str);

						// msg.tipo = pid_req;
						msg.tipo = SRV_MSG_TYPE;
						msg.pid = pid_c2_real;

						ret = msgsnd(mq1_id, (struct msgbuf *)&msg, sizeof(msg) - sizeof(long), 0);

						break;
					}
				}
			} else if (str_starts_with(msg.texto, "quero")) {
				long pid_req = get_ultimo_pid_msg(msg.texto);
				if (pid_req == -1)
					continue;

				// quero abcde 1214 1111
				char *filename = get_nome_fich_args(&msg);
				if (filename == NULL)
					continue;

				for (int i = 0; i < n_ficheiros; i++) {
					if (strcmp(ficheiros[i].nome, filename) == 0) {
						// char pid_str[10];
						// pid_to_string(pid_req, pid_str);

						// msg_f.type = pid_req;
						msg_f.type = SRV_MSG_TYPE;
						strcpy(msg_f.nome, ficheiros[i].nome);
						strcpy(msg_f.conteudo, ficheiros[i].conteudo);

						ret = msgsnd(mq2_id, (struct msgbuf *)&msg_f, sizeof(msg_f) - sizeof(long), 0);
						exit_on_error(ret, "Erro ao tentar enviar mensagem");

						break;
					}
				}
			}
		}
	} else { // C1
		printf("Cliente inicializado (C1: %ld, C2: %ld)\n", pid, pid_c2);

		int ret;
		struct s_msg msg;

		msg.pid = pid;

		printf("-------------------------------------------------------\n");
		printf("Introduza comandos (connect, procura, quero, shutdown):\n");

		while (1) {
			scanf(" %[^\n]s", msg.texto);

			if (strcmp(msg.texto, "connect") == 0) {
				if (contar_n_args(msg.texto) != 1) {
					printf("Erro: comando inválido (uso: connect)\n");
					continue;
				}

				sprintf(msg.texto, "connect %ld", pid_c2);
				msg.tipo = SRV_MSG_TYPE;
			} else if (str_starts_with(msg.texto, "procura")) {
				if (contar_n_args(msg.texto) != 2) {
					printf("Erro: comando inválido (uso: procura PID)\n");
					continue;
				}

				// Converter o PID para string
				char pid_str[10];
				pid_to_string(pid_c2, pid_str);

				strcat(msg.texto, " ");
				strcat(msg.texto, pid_str);

				msg.tipo = SRV_MSG_TYPE;
			} else if (str_starts_with(msg.texto, "quero")) {
				if (contar_n_args(msg.texto) != 3) {
					printf("Erro: comando inválido (uso: quero NOME PID)\n");
					continue;
				}

				long req_pid = get_ultimo_pid_msg(msg.texto);
				if (req_pid == -1)
					continue;

				char pid_str[10];
				pid_to_string(req_pid, pid_str);

				strcat(msg.texto, " ");
				strcat(msg.texto, pid_str);

				msg.tipo = req_pid;
			} else if (strcmp(msg.texto, "shutdown") == 0) {
				if (contar_n_args(msg.texto) != 1) {
					printf("Erro: comando inválido (uso: shutdown)\n");
					continue;
				}

				sprintf(msg.texto, "shutdown %ld", pid_c2);
				msg.tipo = SRV_MSG_TYPE;
			} else {
				// ignorar mensagem
				continue;
			}

			ret = msgsnd(mq1_id, (struct msgbuf *)&msg, sizeof(msg) - sizeof(long), 0);
			exit_on_error(ret, "Erro ao tentar enviar mensagem");
		}
	}

	return 0;
}

int filtro_fich(const struct dirent *entry) {
	// Ignorar '.' e '..' (pasta atual e pasta anterior, respetivamente)
	return strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0;
}

void tratar_sigusr1() {
	printf("Recebi o sinal SIGUSR1. A terminar os processos...\n");

	kill(getppid(), SIGTERM);

	exit(EXIT_SUCCESS);
}

void pid_to_string(long pid, char *str) {
	sprintf(str, "%ld", pid);
}

/* O nome do ficheiro será o segundo parâmetro. Exemplo: procura abc.txt */
char *get_nome_fich_args(struct s_msg *msg) {
	char *arg = strtok(msg->texto, " ");
	if (arg == NULL) {
		printf("Erro: não foi possível obter o nome do ficheiro\n");
		return NULL;
	}

	arg = strtok(NULL, " ");

	if (arg == NULL) {
		printf("Erro: não foi possível obter o nome do ficheiro\n");
		return NULL;
	}

	return arg;
}

int contar_n_args(char *str) {
	int n = 0;
	size_t len = strlen(str);

	for (int i = 0; i < len; i++) {
		if (str[i] == ' ')
			n++;
	}

	return n;
}
