#ifndef _SERVIDOR_H
#define _SERVIDOR_H

#include "msg.h"

int queue_full(struct s_shm *shm);

void handle_connect(struct s_shm *shm, struct s_msg *msg);
void handle_shutdown(struct s_shm *shm, struct s_msg *msg);
void handle_quero(int msg_id, struct s_shm *shm, struct s_msg *msg);
void handle_procura(int msg_id, struct s_shm *shm, struct s_msg *msg);

#endif // _SERVIDOR_H
