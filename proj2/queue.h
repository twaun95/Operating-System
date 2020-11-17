typedef struct node{
        int data;
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

void Enqueue(QUEUE* pQueue, int item){

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

int Dequeue(QUEUE* pQueue){

        QUEUE_NODE* delLoc=NULL;
        int item;

        if(pQueue->count==0)
                return 0;

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

int emptyQueue(QUEUE *queue)
{
        return(queue->count == 0);
}

QUEUE *DestroyQueue(QUEUE *queue)
{
        QUEUE_NODE *deletePtr;

        if(queue)
        {
                while(queue->front != NULL)
                {
                queue->front->data = 0;
                deletePtr = queue->front;
                queue->front = queue->front->next;
                free(deletePtr);
                }
        free(queue);
        }
return NULL;
}
