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
int t=0;

struct sigaction old_sa;
struct sigaction new_sa;
struct msgbuf msg;
struct msgaddr msg1;
pcb_t PCB[MAXPROC];
user uProc;
table **pagetable;
int *pageframe;

Tlb tlb[64];
Directory **directory;
secondtable **table2;

int main()
{
        memset(&new_sa, 0, sizeof(new_sa));
        msgq = msgget( key, IPC_CREAT | 0666);
        memset(&msg, 0, sizeof(msg));
	msgq1 = msgget( key1, IPC_CREAT | 0666);
	memset(&msg1, 0, sizeof(msg1));
	pageframe = (int*)malloc(sizeof(int) * 0x80000);
	directory = (Directory**)malloc(sizeof(Directory*)*MAXPROC);
	for(int k=0; k<MAXPROC; k++)
	{
		directory[k] = (Directory*)malloc(sizeof(Directory)* 0x400);
	}
	table2 = (secondtable**)malloc(sizeof(secondtable*)*0x400);
	for(int k=0; k<0x400; k++)
	{
		table2[k] = (secondtable*)malloc(sizeof(secondtable)*0x400);
	}
        qwait = CreateQueue();
        qready = CreateQueue();
	qfree = CreateQueue();
        srand((unsigned)time(NULL));
        pid_t pid;

	for(int i=0; i<0x80000; i++)
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

void make_table()
{
	int dirindex;
	int index;
	int offset;
	int dir_valid;
	int page_valid;
	int Address;
	int HIT = 0;
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
		dirindex = (PCB[ptr].VA[k] & 0xFFC00000) >> 22;//Directory's index
		index = (PCB[ptr].VA[k] & 0x3FF000) >> 12;//Second table's index
		offset = (PCB[ptr].VA[k] & 0xFFF);
	        dir_valid = directory[PCB[ptr].TTBR][dirindex].dn & 0x00000001;//Directory's valid
		page_valid = table2[directory[PCB[ptr].TTBR][dirindex].dn >> 22][index].pfn & 0x00000001;//Second table's valid

		HIT = 0;
		for(int k=0; k<64; k++){//check hit
			if((tlb[k].tag == PCB[ptr].TTBR)&&(tlb[k].full == 1) && (tlb[k].pn == index)){//check TTBR, full, pagenumber
				HIT = 1;//find in TLB
				tlbindex = k;//check tlb' index when hit
			}
		}

		if(HIT){//if hit
			Address = (tlb[tlbindex].pfn << 12) | offset;//get physical address
			printf("HIT! Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);
		}
		
		else if(!HIT){//if miss
			if(dir_valid == 0)//if not mapping in Directory
			{
				directory[PCB[ptr].TTBR][dirindex].dn = (Dequeue(qfree) & 0xFFC00000) | 0x1;//get Directory value from free list and add valid bit
			
				if(page_valid == 0){//if not mapping in Second table
					table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn = Dequeue(qfree) | 0x1;// get page frame number from free list and add valid bit
					Address = (table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn & 0xFFFFF000) | offset;//get physical address
					page_valid = 1;
					printf("Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);
				}
				dir_valid = 1;
			}

			else if(dir_valid == 1)//if mapping in Directory
			{
				if(page_valid == 0){//if not mapping in Second table
					table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn = Dequeue(qfree) | 0x1;
					Address = (table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn & 0xFFFFF000) | offset;//get physical address
					page_valid = 1;
					printf("Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);
				}

				else if(page_valid == 1){//if mapping in Second table
					Address = (table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn & 0xFFFFF000) | offset;
					tlb[t].pn = index;//update tlb page number
					tlb[t].pfn = (table2[(directory[PCB[ptr].TTBR][dirindex].dn) >> 22][index].pfn & 0xFFFFF000) >> 12;//update tlb page frame number  
					tlb[t].full = 1;//update tlb full
					tlb[t].tag = PCB[ptr].TTBR;//update tlb TTBR tag
					t++;//next tlb index
					if(t == 64){
						t=0;//go first index
					}
					printf("Tick %d, TTBR %d = VA[%d] : 0X%08X -------- Address : 0X%08X\n", tick, PCB[ptr].TTBR, k, PCB[ptr].VA[k], Address);
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
/*	for(int k = 0; k < NUM; k++)
	{
		uProc.VA[k] = (rand() % 0x100000000);
	}
*/	msgsend1();
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
