/* signal test */
/* sigaction */
#include "t.h"
#include "queue.h"
#include "msg.h"
#include "fs.h"

QUEUE *qwait;
QUEUE *qready;
QUEUE *qinode;
FILE *stream;
int ptr;
int wptr;
int msgq;
int key = 0x32143153;
int num = 0;
int num_file = 0;
char buf[256];

struct sigaction old_sa;
struct sigaction new_sa;
struct msgbuf msg;
struct partition party;
struct dentry dentry;
struct blocks userbuf;
pcb_t PCB[MAXPROC];
user uProc;

int main()
{
        memset(&new_sa, 0, sizeof(new_sa));
        msgq = msgget( key, IPC_CREAT | 0666);
        memset(&msg, 0, sizeof(msg));
        qwait = CreateQueue();
        qready = CreateQueue();
	qinode = CreateQueue();
        srand((unsigned)time(NULL));
	mount();

	printf("---------------SuperBlock Information----------------\n");
	printf("Partition type:--------- %d\n", party.s.partition_type);
	printf("Block size:------------- %d\n", party.s.block_size);
	printf("Inode size:------------- %d\n", party.s.inode_size);
	printf("Directory file inode:--- %d\n", party.s.first_inode);
	printf("Number of inodes:------- %d\n", party.s.num_inodes);
	printf("Number of inode blocks:- %d\n", party.s.num_inode_blocks);
	printf("Number of free inodes:-- %d\n", party.s.num_free_inodes);
	printf("Number of blocks:------- %d\n", party.s.num_blocks);
	printf("Number of free blocks:-- %d\n", party.s.num_free_blocks);
	printf("First data block:------- %d\n", party.s.first_data_block);
	printf("Partition name:--------- %s\n", party.s.volume_name);
	printf("\n");
	
	for(int i = 0; i < 224; i++)
	{
		printf("---inode #%d---\n", i);
		printf("Mode:----------- %d\n", party.inode_table[i].mode);
		printf("Locked:--------- %d\n", party.inode_table[i].locked);
		printf("Date:----------- %d\n", party.inode_table[i].date);
		printf("File Size:------ %d\n", party.inode_table[i].size);
		printf("Indirect block:- %d\n", party.inode_table[i].indirect_block);
		printf("Blocks:--------- ");
		for(int j = 0; j < 6; j++)
		{
			printf("%d ", party.inode_table[i].blocks[j]);
		}
		printf("\n\n");
	}

        pid_t pid;

        for(int k=0; k<MAXPROC; k++)
        {
                uProc.exec_time = EXE;//(rand() % 10) + 1;
                uProc.ppid = getpid();
                uProc.IO = 1;
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
        if(emptyQueue(qready)){
		return;
	}
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
				int value = open();
				if(value == 1)
				{
					read_ker();
					close_ker();
				}
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
	if((strncmp(PCB[0].pathname, "exit", 4)==0))
        {
                for(int k=0; k<MAXPROC; k++)
                {
                        kill(PCB[k].pid, SIGINT);
                }
                DestroyQueue(qready);
                DestroyQueue(qwait);
                printf("Tick : %d, OS exit\n", tick);
		fclose(stream);
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
		printf("File name : ");
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1] = '\0';
		memcpy(&uProc.pathname, &buf, sizeof(buf));
                uProc.exec_time = EXE;//(rand() % 10) + 1;
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
			memcpy(&PCB[k].pathname, &msg.pathname, sizeof(msg.pathname));
                       // printf("mother CPU : %d IO : %d, index : %d\n", PCB[k].exec_time, PCB[k].IO_time, k);
                }
        }
}

int msgsend()
{
        int ret = -1;
        msg.mtype = 0;
        msg.pid = getpid();
        msg.io_time = uProc.IO;
        msg.cpu_time = uProc.exec_time;
	memcpy(&msg.pathname, &uProc.pathname, sizeof(uProc.pathname));
	msg.O_RD = uProc.O_RD;
        while(ret == -1)
        {
                ret = msgsnd(msgq, &msg, sizeof(msg), 0);
        }
        return 0;
}

int msgrecieve()
{
        int ret = -1;
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
        new_itimer.it_interval.tv_usec = 1000;
        new_itimer.it_value.tv_sec = 0;//interval_sec;
        new_itimer.it_value.tv_usec = 1000;
        setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}

int mount()
{
        stream = fopen("disk.img", "r");
        if (stream == NULL) exit(EXIT_FAILURE);

        fread(&party, (sizeof(party.s)+sizeof(party.inode_table)) , 1, stream);

        if((party.inode_table[party.s.first_inode].size % 1024)==0)
        {
                num = party.inode_table[party.s.first_inode].size / 1024;
        }
        else
        {
                num = (party.inode_table[party.s.first_inode].size / 1024) + 1;
        }
	

        for(int i=0; i<num; i++)
        {
                fseek(stream, 0x2000 +(i * 0x400), SEEK_SET);
                for(int k=0; k<32; k++)
                {
                        fread(&dentry, sizeof(dentry), 1, stream);
                        if(dentry.name_len == 0)return 0;
			
                        printf("name : %s\n", dentry.name);
                }
        }
}

int open()
{
        for(int i=0; i<num; i++)
        {
                fseek(stream, 0x2000 +(i * 0x400), SEEK_SET);
                for(int k=0; k<32; k++)
                {
                        fread(&dentry, sizeof(dentry), 1, stream);
                        if(strncmp(dentry.name, PCB[0].pathname, sizeof(PCB[0].pathname)) == 0)
                        {
				//printf("path : %s\n", PCB[0].pathname);
				printf("File name:-------- %s\n", dentry.name);
				printf("Inode number:----- %d\n", dentry.inode);
				printf("Dentry Size:------ %d\n", dentry.dir_length);
				printf("File name length:- %d\n", dentry.name_len);
				printf("File Type:-------- %d\n", dentry.file_type);
				printf("Mode:------------- %d\n", party.inode_table[dentry.inode].mode);
				printf("Locked:----------- %d\n", party.inode_table[dentry.inode].locked);
				printf("Date:------------- %d\n", party.inode_table[dentry.inode].date);
				printf("File Size:-------- %d\n", party.inode_table[dentry.inode].size);
				printf("Indirect block:--- %d\n", party.inode_table[dentry.inode].indirect_block);
				printf("Blocks:----------- ");
				for(int j = 0; j < 6; j++)
				{
					printf("%d ", party.inode_table[dentry.inode].blocks[j]);
				}
				printf("\n");
				for(int a=0; a<qinode->count; a++)
				{
					if(qinode->front->data == dentry.inode)
					{
						return 1;
					}
					else
					{
						Enqueue(qinode, Dequeue(qinode));
					}
				}
				Enqueue(qinode, dentry.inode);
				printf("Open file inode num : %d\n", dentry.inode);
                                return 1;
                        }
                }
        }
        printf("Not Found\n");
        return 0;
}

int read_ker()
{
        if((party.inode_table[dentry.inode].size % 1024)==0)
        {
                num_file = party.inode_table[dentry.inode].size / 1024;
        }
        else
        {
                num_file = (party.inode_table[dentry.inode].size / 1024) + 1;
        }

        for(int i=0; i<num_file; i++)
        {
                fseek(stream, 0x2000 + ((party.inode_table[dentry.inode].blocks[i])*0x400), SEEK_SET);
                fread(&userbuf, sizeof(userbuf), 1, stream);
                printf("%s\n", userbuf.d);
        }
}

int close_ker()
{
	for(int k=0; k<qinode->count; k++)
	{
		if(qinode->front->data == dentry.inode)
		{
			Dequeue(qinode);
		}
		else
		{
			Enqueue(qinode, Dequeue(qinode));
		}
	}
}
