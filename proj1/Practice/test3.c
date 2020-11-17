/* signal test */
/* sigaction */
#include "sched.h"
#include "queue.h"
#include "msg.h"

pcb_t PCB[MAXPROC]; // only for parent
int exec_time = 0; // only for child

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

	pid_t pid;
	for(int k=0; k<MAXPROC; k++)
	{
		exec_time = (rand() % 15) + 1;
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
			PCB[k].exec_time = exec_time;
			Enqueue(qready, k);
		}
	}
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	fire(1);
	while (1);
}

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

void fire(int interval_sec)
{
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = 100;
	new_itimer.it_value.tv_sec = 0;//interval_sec;
	new_itimer.it_value.tv_usec = 100;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}

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
	/*else
	{
		Dequeue(qready);
		kill(PCB[ptr].pid, SIGUSR1);
	}*/
}
void RR()
{
	ptr = qready->front->data;
	if(PCB[ptr].remaining_tq > 0)
	{
		kill(PCB[ptr].pid, SIGUSR1);
		PCB[ptr].remaining_tq--;
		PCB[ptr].exec_time--;
		printf("Tick : %d TQ : %d PCB[%d].exec_time : %d\n", tick, PCB[ptr].remaining_tq, ptr, PCB[ptr].exec_time);
		if(PCB[ptr].exec_time == 0)
		{
			Dequeue(qready);
			return;
		}
	}
	if(PCB[ptr].remaining_tq == 0)
	{
			PCB[ptr].remaining_tq = DEFAULT_TQ;
			Enqueue(qready, Dequeue(qready));
	}
}

void signal_handler(int signo)
{
	tick++;
	if(emptyQueue(qready)) 
	{
		DestroyQueue(qrun);
		DestroyQueue(qready);
		DestroyQueue(qwait);
		exit(0);
	}
	RR();
}

void signal_handler2(int signo)
{
	exec_time--;
	if(exec_time == 0)
	{
		exit(0);
	}
}
