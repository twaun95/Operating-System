#define MAX_PROC 10
#define DEFAULT_TQ 3
void RR();
void do_child();

typedef struct pcb_{
        int pid;
        int remaining_tq;
        int exec_time;
	int IO_time;
}pcb_t;

typedef struct node{
        pcb_t data;
        struct node* next;
} QUEUE_NODE;

typedef struct
{
        QUEUE_NODE* front;
        QUEUE_NODE* rear;
        int count;
} QUEUE;

QUEUE* CreateQueue(void){

        QUEUE* pNewQueue=(QUEUE*)malloc(sizeof(QUEUE));
        if(pNewQueue==NULL)
                return NULL;

        pNewQueue->count=0;
        pNewQueue->front=pNewQueue->rear=NULL;

        return pNewQueue;
};

void Enqueue(QUEUE* pQueue, pcb_t item){

        QUEUE_NODE* Newptr=(QUEUE_NODE*)malloc(sizeof(QUEUE_NODE));
        if (Newptr==NULL)
                return ;

        Newptr->data = item;
        Newptr->next = NULL;

        if(pQueue->count == 0){
                pQueue->front=pQueue->rear=Newptr;
        }
        else{
                pQueue->rear->next=Newptr;
                pQueue->rear=Newptr;
        }

        pQueue->count++;
};

pcb_t Dequeue(QUEUE* pQueue){

        pcb_t rc;
        rc.pid = 0;
        rc.remaining_tq = 0;
        QUEUE_NODE* delLoc=NULL;
        pcb_t item;

        if(pQueue->count==0)
                return rc;

        delLoc=pQueue->front;
        item=delLoc->data;

        if(pQueue->count==1){
                pQueue->front=pQueue->rear=NULL;
        }
        else{
                pQueue->front=delLoc->next;
        }

        free(delLoc);
        pQueue->count--;

        return item;
};
