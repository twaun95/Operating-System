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
#include "que1.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include "msg.h"
#define MAXPROC 5

void signal_handler(int signo);
void signal_handler2(int signo);
void signal_handler3(int signo);
void order();
void FIFO();
void SJF();
void RR();
int tick = 0;
void fire();
int msgsend();
int msgrecieve();

struct msgbuf msg;
pcb_t PCB[MAXPROC];
// only for child!
typedef struct child{
	int exec_time;
	int IO;
	int ppid;
}user;

user uProc;

QUEUE *qrun;
QUEUE *qwait;
QUEUE *qready;
int ptr;
int wptr;

struct sigaction old_sa;
struct sigaction new_sa;

int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	qrun = CreateQueue();
	qwait = CreateQueue();
	qready = CreateQueue();
	srand((unsigned)time(NULL));
	pid_t pid;

	for(int k=0; k<MAXPROC; k++)
	{
		//uProc.exec_time = (rand() % 15) + 1;
		uProc.exec_time = 5;
		uProc.ppid = getpid();
		uProc.IO = 3;
		pid=fork();
		if(pid<0){
			perror("fork fail");
		}
		else if(pid == 0){//child
		//	printf("child\n");
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGUSR1, &new_sa, &old_sa);
			while(1);
			exit(0);
		}
		else{//parent
		//	printf("parent\n");
			PCB[k].pid = pid;
			PCB[k].remaining_tq = 3;
			PCB[k].exec_time = uProc.exec_time;
		}
	}
	for(int k=0; k<MAXPROC; k++)
	{
		printf("PID : %d\n", PCB[k].pid);
		printf("TQ : %d\n", PCB[k].remaining_tq);
		printf("Exe : %d\n", PCB[k].exec_time);
		//printf("uProc.cpid : %d\n", cpid);
		Enqueue(qready, k);
	}
	printf("===========\n");
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	fire(1);
	
	while(1);
//	{
//		new_sa.sa_handler = &signal_handler3;
//		sigaction(SIGUSR2, &new_sa, &old_sa);
//	}
}

void RR()
{
	if(emptyQueue(qready) != 1){
	ptr = qready->front->data;
	if(PCB[ptr].remaining_tq > 0)
	{
		kill(PCB[ptr].pid, SIGUSR1);
		PCB[ptr].remaining_tq--;
		PCB[ptr].exec_time--;
		printf("Index : %d Pid : %d  EXE : %d\n", ptr, PCB[ptr].pid, PCB[ptr].exec_time);
		if(PCB[ptr].exec_time == 0)
		{
			PCB[ptr].exec_time = 5;
			Enqueue(qwait, Dequeue(qready));
			new_sa.sa_handler = &signal_handler3;
			sigaction(SIGUSR2, &new_sa, &old_sa);
			return;
		}
	}
	if(PCB[ptr].remaining_tq == 0)
	{
			PCB[ptr].remaining_tq = DEFAULT_TQ;
			Enqueue(qready, Dequeue(qready));
	}}
}

void do_wait()
{
	if(emptyQueue(qwait))
	{
//		printf("--------\n");
		return;
	}
	else
	{
		for(int k=0; k<MAXPROC;k++)
		{
		PCB[k].IO_time--;
		if(PCB[k].IO_time == 0)
		{
			Enqueue(qready, Dequeue(qwait));
		}
		else if(PCB[k].IO_time > 0)
		{
			Enqueue(qwait, Dequeue(qwait));
		}}
	}
}

void signal_handler(int signo)
{
	tick++;
	if(tick == 100) 
	{
		for(int k=0; k<MAXPROC; k++)
		{
			kill(PCB[k].pid, SIGINT);
		}
		DestroyQueue(qrun);
		DestroyQueue(qready);
		DestroyQueue(qwait);
		exit(0);
	}
	RR();
	do_wait();
}

void signal_handler2(int signo)
{
	uProc.exec_time--;
//	printf("Ppid : %d\n", uProc.ppid);
//	printf("uProc.exec_time : %d\n", uProc.exec_time);
	if(uProc.exec_time == 0)
	{
		printf("============================\n");
		msgsend();
		kill(uProc.ppid, SIGUSR2);
		uProc.exec_time = 5;
	}
}

void signal_handler3(int signo)
{

	printf("msgget!\n");
	msgrecieve();
	//for(int k=0; k<MAXPROC; k++)
	//{
		if(PCB[ptr].pid == msg.pid)
		{
			PCB[ptr].IO_time = msg.io_time;
			printf("mother IO : %d, index : %d\n", PCB[ptr].IO_time, ptr);
			//Enqueue(qwait, Dequeue(qready));
		}
	//}
}

int msgsend()
{
        int msgq;
        int ret;
        int key = 0x32143153;
        msgq = msgget( key, IPC_CREAT | 0666);
        memset(&msg, 0, sizeof(msg));
        msg.mtype = 0;
        msg.pid = getpid();
        msg.io_time = 10;
        ret = msgsnd(msgq, &msg, sizeof(msg), NULL);
        return 0;
}

int msgrecieve()
{
        int msgq;
        int ret;
        int key = 0x32143153;
        msgq = msgget( key, IPC_CREAT | 0666);
        memset(&msg, 0, sizeof(msg));
        ret = msgrcv(msgq, &msg, sizeof(msg), 0, NULL);
        return 0;
}

void fire(int interval_sec)
{
        struct itimerval new_itimer, old_itimer;
        new_itimer.it_interval.tv_sec = 0;
        new_itimer.it_interval.tv_usec = 100;
        new_itimer.it_value.tv_sec = 0;//interval_sec;
        new_itimer.it_value.tv_usec = 100;
        setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}

/*
void order()
{
        int temp;
        int j,k;

        for(j=0; j<MAXPROC; j++)
        {
                for(k=0; k<MAXPROC-1; k++)
                {
                        if(PCB[k].exec_time > PCB[k+1].exec_time)
                        {
                                temp = PCB[k].exec_time;
                                PCB[k].exec_time = PCB[k+1].exec_time;
                                PCB[k+1].exec_time = temp;
                        }
                }
        }
}

void SJF()
{
        order();
        FIFO();
}
*/

/*void FIFO()
{
        ptr = qready->front->data;
        if(PCB[ptr].exec_time > 0)
        {
                PCB[ptr].exec_time--;
                printf("EXE : %d\n", PCB[ptr].exec_time);
                if(PCB[ptr].exec_time == 0)
                {
                        Dequeue(qready);
                        kill(PCB[ptr].pid, SIGUSR1);
                }
        }
        else
        {
                Dequeue(qready);
                kill(PCB[ptr].pid, SIGUSR1);
        }
}*/
