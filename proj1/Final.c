/* signal test */
/* sigaction */
#include "sched.h"
#include "queue.h"
#include "msg.h"

QUEUE *qrun;
QUEUE *qwait;
QUEUE *qready;
int ptr;
int wptr;
int msgq;
int key = 0x32143153;

struct sigaction old_sa;
struct sigaction new_sa;
struct msgbuf msg;
pcb_t PCB[MAXPROC];
user uProc;

int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	msgq = msgget( key, IPC_CREAT | 0666);
	memset(&msg, 0, sizeof(msg));
	qwait = CreateQueue();
	qready = CreateQueue();
	srand((unsigned)time(NULL));
	pid_t pid;

	for(int k=0; k<MAXPROC; k++)
	{
		uProc.exec_time = (rand() % 10) + 1;
		uProc.ppid = getpid();
		uProc.IO = (rand() % 10) + 1;
		pid=fork();
		if(pid<0){
			perror("fork fail");
		}
		else if(pid == 0){//child
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGUSR1, &new_sa, &old_sa);
			while(1);
			exit(0);
		}
		else{//parent
			PCB[k].pid = pid;
			PCB[k].remaining_tq = DEFAULT_TQ;
			PCB[k].exec_time = uProc.exec_time;
		}
		Enqueue(qready, k);
	}

	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	new_sa.sa_handler = &signal_handler3;
	sigaction(SIGUSR2, &new_sa, &old_sa);
	fire(1);
	
	while(1);
}

void RR()
{
	if(emptyQueue(qready))return;
	else{
		ptr = qready->front->data;
		if(PCB[ptr].remaining_tq > 0)
		{
			PCB[ptr].remaining_tq--;
			PCB[ptr].exec_time--;
			printf("Tick : %d, PID : %d\tPCB[%d].exec_time : %d\n", tick, PCB[ptr].pid, ptr, PCB[ptr].exec_time);
			kill(PCB[ptr].pid, SIGUSR1);
			if(PCB[ptr].exec_time == 0)
			{
				PCB[ptr].remaining_tq = DEFAULT_TQ;
				Enqueue(qwait, Dequeue(qready));
				return;
			}
		}
		if(PCB[ptr].remaining_tq == 0)
		{
			PCB[ptr].remaining_tq = DEFAULT_TQ;
			Enqueue(qready, Dequeue(qready));
			return;
		}
	}
	return;
}

void do_wait()
{
	if(emptyQueue(qwait))
	{
		return;
	}
	else
	{
		for(int k=0; k<qwait->count; k++)
		{
			wptr = qwait->front->data;
			if(PCB[wptr].IO_time > 0)
			{
				PCB[wptr].IO_time--;
				//printf("PCB[%d].IOtime : %d\n", wptr, PCB[wptr].IO_time);
				if(PCB[wptr].IO_time == 0)
				{
					Enqueue(qready, Dequeue(qwait));
				}
				else if(PCB[wptr].IO_time > 0)
				{
					Enqueue(qwait, Dequeue(qwait));
				}
			}
		}
	}
}

void signal_handler(int signo)
{
	tick++;
	if(tick == 10000) 
	{
		for(int k=0; k<MAXPROC; k++)
		{
			kill(PCB[k].pid, SIGINT);
		}
		DestroyQueue(qready);
		DestroyQueue(qwait);
		printf("Tick : %d, OS exit\n", tick);
		exit(0);
	}
	do_wait();
	RR();
}

void signal_handler2(int signo)
{
	uProc.exec_time--;
	if(uProc.exec_time == 0)
	{
		uProc.exec_time = (rand() % 10) + 1;
		msgsend();
		kill(uProc.ppid, SIGUSR2);
	}
}

void signal_handler3(int signo)
{

	msgrecieve();
	for(int k=0; k<MAXPROC; k++)
	{
		if(PCB[k].pid == msg.pid)
		{
			PCB[k].IO_time = msg.io_time;
			PCB[k].exec_time = msg.cpu_time;
			//printf("mother CPU : %d IO : %d, index : %d\n", PCB[k].exec_time, PCB[k].IO_time, k);
		}
	}
}

int msgsend()
{
        int ret = -1;
        //struct msgbuf msg;
        msg.mtype = 0;
        msg.pid = getpid();
        msg.io_time = uProc.IO;
	msg.cpu_time = uProc.exec_time;
	while(ret == -1){
        ret = msgsnd(msgq, &msg, sizeof(msg), 0);}
        return 0;
}


int msgrecieve()
{
        int ret = -1;
        //struct msgbuf msg;
	while(ret == -1)
	{
        	ret = msgrcv(msgq, &msg, sizeof(msg), 0, MSG_NOERROR);
	}
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
