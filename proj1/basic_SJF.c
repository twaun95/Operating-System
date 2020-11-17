/* signal test */
/* sigaction */
#include "sched.h"
#include "queue.h" // queue fuction
#include "msg.h" // msg structure

pcb_t PCB[MAXPROC]; // only for parent
int exec_time = 0; // only for child
int set[MAXPROC]; // get the exec_time

QUEUE *qwait;
QUEUE *qready;
int ptr;
int wait_time[MAXPROC];
float aver_time;

int main()
{
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	qwait = CreateQueue(); // create wait queue
	qready = CreateQueue(); // create ready queue
	srand((unsigned)time(NULL));
	for(int k=0; k<MAXPROC; k++)
	{
		set[k] = (rand() % 15) + 1; // get the set randomly
	}
	order(); // ascending order the set for the exec_time
	pid_t pid;
	for(int k=0; k<MAXPROC; k++)
	{
		exec_time = set[k]; // get the exec_time for set that already ascending order
		pid=fork();
		if(pid<0){
			perror("fork fail");
		}
		else if(pid == 0){//child
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGUSR1, &new_sa, &old_sa); // set SIGUSR1 signal as new_sa function
			while(1);
			exit(0);
		}
		else{//parent
			PCB[k].pid = pid;
			PCB[k].remaining_tq = DEFAULT_TQ;
			PCB[k].cpu_time = exec_time;
			PCB[k].exec_time = exec_time;
			Enqueue(qready, k); // put the index of PCB in the ready queue
		}
	}
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa); // set SIGALRM signal as new_sa function
	fire(1); // give siganl to SIGALRM to work
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
			if(set[k] > set[k+1])
			{
				temp = set[k];
				set[k] = set[k+1];
				set[k+1] = temp;
			}
		}
	}
}

void fire(int interval_sec)
{
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = 10; // give 10Microsecond for signal
	new_itimer.it_value.tv_sec = 0;//interval_sec;
	new_itimer.it_value.tv_usec = 10;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}

void SJF() // Short Job First algorithm
{
	ptr = qready->front->data; // get the first data of ready queue
	if(PCB[ptr].exec_time > 0)
	{
		PCB[ptr].exec_time--;
		printf("Tick : %d Proc[%d].exe : %d\n", tick, ptr, PCB[ptr].exec_time);
		kill(PCB[ptr].pid, SIGUSR1);
		if(PCB[ptr].exec_time == 0)
		{
			Dequeue(qready); // user process works done then terminate
			wait_time[ptr] = tick - PCB[ptr].cpu_time; // how long does it take for waiting
		}
	}
}

void signal_handler(int signo)
{
	tick++; // count the tick
	if(emptyQueue(qready)) // ready queue is empty then kernal has nothing to work so exit
	{
		DestroyQueue(qready); // destroy ready queue
		DestroyQueue(qwait); // destroy wait queue
		for(int k=0; k<MAXPROC; k++)
		{
			aver_time += wait_time[k]; // get the average waiting time for process
		}
		aver_time = aver_time/MAXPROC;
		printf("Average waiting time : %f microseconds\n", aver_time);
		exit(0);
	}
	SJF();
}

void signal_handler2(int signo)
{
	exec_time--;
	//printf("child exe : %d\n", exec_time);
	if(exec_time == 0)
	{
		exit(0);
	}
}
