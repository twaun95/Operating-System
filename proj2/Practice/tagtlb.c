/* signal test */
/* sigaction */
#include "t.h"
#include "queue.h"
#include "msg.h"
#define NUM 10

QUEUE *qfree;
QUEUE *qwait;
QUEUE *qready;
int ptr;
int wptr;
int msgq;
int msgq1;
int key = 0x32143153;
int key1 = 0x32140301;
int i = 0;

struct sigaction old_sa;
struct sigaction new_sa;
struct msgbuf msg;
struct msgaddr msg1;
pcb_t PCB[MAXPROC];
user uProc;
table **pagetable;
int *pageframe;
typedef struct TLB_{
	int tag;
	int pn;
	int pfn;
	int full;
}Tlb;
Tlb tlb[64];

int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	msgq = msgget( key, IPC_CREAT | 0666);
	memset(&msg, 0, sizeof(msg));
	msgq1 = msgget( key1, IPC_CREAT | 0666);
	memset(&msg1, 0, sizeof(msg1));
	pageframe = (int*)malloc(sizeof(int) * 0xA0000);
	pagetable = (table**)malloc(sizeof(table*) * MAXPROC);
	for(int k =0; k<MAXPROC; k++)
	{
		pagetable[k] = (table*)malloc(sizeof(table) * 0x100000); 
	}	

	qwait = CreateQueue();
	qready = CreateQueue();
	qfree = CreateQueue();
	srand((unsigned)time(NULL));
	pid_t pid;

	for(int i=0; i<0xA0000; i++)
	{
		Enqueue(qfree, i << 12 | 0x000);
	}
        for(int k=0; k<MAXPROC; k++)
        {
		for(int i=0; i<NUM; i++)
		{
			uProc.VA[i] = (rand() % 0x100000000);
		}
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
			PCB[k].TTBR = k;
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

void make_table()
{
	int index;
	int offset;
	int valid;
	int Address;
	int HIT;
	int tlbindex;

	for(int k=0; k<MAXPROC; k++)
	{
		if(PCB[k].pid == msg1.pid)
		{
			memcpy(&PCB[k].VA, &msg1.VA, sizeof(msg1.VA));
		}
	}

	for(int k=0; k<NUM; k++)
	{
		index = (PCB[ptr].VA[k] & 0xFFFFF000) >> 12;
		offset = (PCB[ptr].VA[k] & 0xFFF);
		valid = pagetable[PCB[ptr].TTBR][index].pfn & 0x00000001;
		HIT = 0;
		
		for(int k=0; k<64; k++){
			if((tlb[k].tag == PCB[ptr].TTBR)&&(tlb[k].full == 1)&&(tlb[k].pn == index)){
				HIT = 1;
				tlbindex = k;
			}
		}

		if(HIT){
			Address = (tlb[tlbindex].pfn & 0xFFFFF000) | offset;
			printf("HIT!!, Tick %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, k, PCB[ptr].VA[k], Address);
			printf("--> VA: 0X%08X, TTBR : %d index : 0X%08X --------------- PA : 0X%08X tlb[%d], TTBR : %d pn : 0X%08X pfn : 0X%08X\n",PCB[ptr].VA[k], PCB[ptr].TTBR, index, Address, tlbindex, tlb[tlbindex].tag, tlb[tlbindex].pn, tlb[tlbindex].pfn);
		}

		else if(!HIT){
			if(valid == 0)
			{
			//need mapping
				pagetable[PCB[ptr].TTBR][index].pfn = Dequeue(qfree) | 0x1;
				Address = (pagetable[PCB[ptr].TTBR][index].pfn & 0xFFFFF000) | offset;
				valid = 1;
				printf("Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);
			
			}
			else if(valid == 1)
			{
				Address = (pagetable[PCB[ptr].TTBR][index].pfn & 0xFFFFF000)| offset;
				tlb[i].pn = index;
				tlb[i].pfn = pagetable[PCB[ptr].TTBR][index].pfn;
				tlb[i].full = 1;
				tlb[i].tag = PCB[ptr].TTBR;
				i++;
				if(i==64){
					i=0;
				}	
				printf("Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);

			}	


		}
	}
}

void signal_handler(int signo)
{
        tick++;
        if(tick == 50)
        {
                for(int k=0; k<MAXPROC; k++)
                {
                        kill(PCB[k].pid, SIGINT);
                }
                DestroyQueue(qready);
                DestroyQueue(qwait);
		DestroyQueue(qfree);
                printf("Tick : %d, OS exit\n", tick);
                exit(0);
        }
	do_wait();
	RR();
	msgrecieve1();
	make_table();
}

void signal_handler2(int signo)
{
        uProc.exec_time--;
	/*for(int k = 0; k < NUM; k++)
	{
		uProc.VA[k] = (rand() % 0x100000000);
	}*/
	msgsend1();
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
        while(ret == -1)
	{
        	ret = msgsnd(msgq, &msg, sizeof(msg), 0);
	}
        return 0;
}

int msgsend1()
{
	int ret = -1;
	msg1.mtype = 0;
	msg1.pid = getpid();
	memcpy(&msg1.VA, &uProc.VA, sizeof(uProc.VA));
	while(ret == -1)
	{
		ret = msgsnd(msgq1, &msg1, sizeof(msg1), 0);
	}
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

int msgrecieve1()
{
	int ret = -1;
	while(ret == -1)
	{
		ret = msgrcv(msgq1, &msg1, sizeof(msg1), 0, MSG_NOERROR);
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
