/* signal test */
/* sigaction */
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define MAXPROC 10
#define DEFAULT_TQ 3

void signal_handler(int signo);
void signal_handler2(int signo);
void signal_handler3(int signo);
void order();
void FIFO();
void SJF();
void RR();
void do_wait();
int tick;
void fire(int interval_sec);
int msgsend();
int msgrecieve();

// only for parent!
typedef struct pcb_{
        int pid;
        int remaining_tq;
        int exec_time;
	int cpu_time;
        int IO_time;
}pcb_t;


// only for child!
typedef struct child{
        int exec_time;
        int IO;
        int ppid;
}user;
