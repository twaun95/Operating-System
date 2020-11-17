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
#define MAXPROC 1
#define DEFAULT_TQ 3
#define EXE 3

void signal_handler(int signo);
void signal_handler2(int signo);
void signal_handler3(int signo);
void order();
void FIFO();
void SJF();
void RR();
void do_wait();
void make_table();
int tick = 0;
void fire(int interval_sec);
int msgsend();
int msgsend1();
int msgrecieve();
int msgrecieve1();
int open();
int read_ker();
int mount();
int close_ker();

typedef struct TABLE_{
        int pfn;
}table;

typedef struct DIRECTORY_{
        int dn;
}Directory;

typedef struct TABLE2_{
        int pfn;
}secondtable;

typedef struct FRAME_{
        int data[0x1000];
}frame;

typedef struct DISK_TABLE{
        //int index;
        //int TTBR;
        int where;
}disk;

typedef struct TLB_{
        int pn;
        int pfn;
        int full;
        int tag;
}Tlb;

// only for parent!
typedef struct pcb_{
        int pid;
        int remaining_tq;
        int exec_time;
        int cpu_time;
        int IO_time;
        int VA[10];
        int TTBR;
	int inode[102];
	char pathname[16];
	int inode_num;
	int O_RD;
}pcb_t;


// only for child!
typedef struct child{
        int exec_time;
        int IO;
        int ppid;
        int VA[10];
	int inode;
	char pathname[16];
	int O_RD;
	int inode_num;
}user;
