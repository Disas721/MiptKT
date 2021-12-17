#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/msg.h>
#include<errno.h>
#include<pthread.h>
#define S 8


struct mbuf { long mtype; };

struct ip { int i; };

int qid;
struct ip pdata[S];

void* threader(void *data) {
	int i = ((struct ip*)data)->i;
	struct mbuf buffer;

	if (msgrcv(qid, &buffer, 0, i, 0) < 0) {
		perror("failed to read message");
		exit(errno);
	}
	buffer.mtype = i + 1;
	printf("thread %d running, next %ld\n", i, buffer.mtype);
	fflush(stdout);
	if (msgsnd(qid, &buffer, 0, 0) == -1) {
		perror("failed to send message");
		exit(errno);
	}
	pthread_exit(NULL);
}

int main() {
    pthread_t thid[S];
	struct mbuf buffer;
	
	key_t key = ftok(".", 0);
	qid = msgget(key, IPC_CREAT | 0666);
	if (qid == -1) {
		perror("failed to get message queue");
		exit(errno);
	}
	for (int i = 0; i < S; i++) {
		pdata[i].i = i + 1;
		if (pthread_create(&thid[i], NULL, threader, (void*)&pdata[i]) != 0) {
			perror("failed to create thread");
			msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
			exit(errno);
		}
	}
	buffer.mtype = 1;
	
	if (msgsnd(qid, &buffer, 0, 0) == -1) {
		perror("failed to send message from master");
		msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
		exit(errno);
	}
	
	if (msgrcv(qid, &buffer, 0, S + 1, 0) < 0) {
		perror("failed to read message");
		msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
		exit(errno);
	}
	printf("threads finished, received %d\n", S + 1);

	for (int i = 0; i < S; i++) {
		if (pthread_join(thid[i], NULL) != 0) {
			perror("failed to join thread");
			msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
			exit(errno);
		}
	}
	msgctl(qid, IPC_RMID, (struct msqid_ds *) NULL);
	return 0;
}
