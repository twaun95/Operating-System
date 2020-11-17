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
#define MAXPROC 10

int SJF[MAXPROC];
void signal_handler(int signo);
void signal_handler2(int signo);
void order();
int count = 0;
void fire();

pcb_t PCB[MAXPROC];

QUEUE *qrun;
QUEUE *qwait;
QUEUE *qready;
int ptr;

int main()
{
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	qrun = CreateQueue();
	qwait = CreateQueue();
	qready = CreateQueue();
	srand((unsigned)time(NULL));

	//order();
	pid_t pid;
	for(int k=0; k<10; k++)
	{
		pid=fork();
		if(pid<0){
			perror("fork fail");
		}
		else if(pid == 0){//child
			printf("child\n");
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGUSR1, &new_sa, &old_sa);
			while(1);
			exit(0);
		}
		else{//parent
			printf("parent\n");
			PCB[k].pid = pid;
			PCB[k].remaining_tq = DEFAULT_TQ;
			//SJF[k] = rand() % 15;
			//PCB[k].exec_time = SJF[k];
			PCB[k].exec_time = 10;
		}
	}
	for(int k=0; k<10; k++)
	{
		printf("PID : %d\n", PCB[k].pid);
		printf("TQ : %d\n", PCB[k].remaining_tq);
		printf("Exe : %d\n", PCB[k].exec_time);
		Enqueue(qready, k);
	}

	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	fire(1);
	while (1);
}
/*
void order()
{
	int temp;
	int i,j,k;

	for(i=0; i<MAXPROC; i++)
	{
		SJF[i] = rand() % 15;
	}
	for(j=0; j<MAXPROC; j++)
	{
		for(k=0; k<MAXPROC-1; k++)
		{
			if(SJF[k] > SJF[k+1])
			{
				temp = SJF[k];
				SJF[k] = SJF[k+1];
				SJF[k+1] = temp;
			}
		}
	}
}
*/


void fire(int interval_sec)
{
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = interval_sec;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}
/*
void FIFO()
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
}
*/
void RR()
{
	ptr = qready->front->data;
	if(PCB[ptr].remaining_tq > 0)
	{
		printf("TQ : %d PCB[0].TQ : %d\n", PCB[ptr].remaining_tq, PCB[0].remaining_tq);
		PCB[ptr].remaining_tq--;
		PCB[ptr].exec_time--;
		printf("TQ : %d PCB[0].TQ : %d\n", PCB[ptr].remaining_tq, PCB[0].remaining_tq);
		printf("EXE : %d\n", PCB[ptr].exec_time);
		if(PCB[ptr].exec_time == 0) 
		{
			Dequeue(qready);
			kill(PCB[ptr].pid, SIGUSR1);
		}
	}
	else if(PCB[ptr].remaining_tq == 0)
	{
		PCB[ptr].remaining_tq = DEFAULT_TQ;
		Enqueue(qready, Dequeue(qready));
	}
}

void signal_handler(int signo)
{
	if(emptyQueue(qready)) 
	{
		DestroyQueue(qrun);
		DestroyQueue(qready);
		DestroyQueue(qwait);
		exit(0);
	}
	FIFO();
}

void signal_handler2(int signo)
{
	ptr = qready->front->data;
        if(PCB[ptr].remaining_tq > 0)
        {
                printf("TQ : %d PCB[0].TQ : %d\n", PCB[ptr].remaining_tq, PCB[0].remaining_tq);
                PCB[ptr].remaining_tq--;
                PCB[ptr].exec_time--;
                printf("TQ : %d PCB[0].TQ : %d\n", PCB[ptr].remaining_tq, PCB[0].remaining_tq);
                printf("EXE : %d\n", PCB[ptr].exec_time);
                if(PCB[ptr].exec_time == 0)
                {
                        Enqueue(qwait, Dequeue(qready));
                        //kill(PCB[ptr].pid, SIGUSR1);
                }
        }
        else if(PCB[ptr].remaining_tq == 0)
        {
                PCB[ptr].remaining_tq = DEFAULT_TQ;
                Enqueue(qready, Dequeue(qready));
		if(PCB[ptr].exec_time == 0)
		{
			Enqueue(qwait, Dequeue(qready));
		}
        }

}
