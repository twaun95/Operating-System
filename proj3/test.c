#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "t.h"
#include "queue.h"
#include "msg.h"

FILE *stream;
struct partition party;
struct dentry dentry;
struct blocks userbuf;
int open();
int read_pro();
int mount();

pcb_t PCB[MAXPROC];
user uProc;
int res;
int num;
char buf[256];

int main(void)
{
	pid_t pid;

	for(int k=0; k<MAXPROC; k++)
	{
		uProc.ppid = getpid();
		pid=fork();
		if(pid<0){
			perror("fork fail");
		}
		else if(pid == 0){//child
			
			while(1);
			exit(0);
		}
		else{//parent
			PCB[k].pid = pid;
			PCB[k].TTBR = k;
		}
	}

	//mount
	mount();

	//open	
	open();
	//read
	read_pro();

	//close
	fclose(stream);
	kill(PCB[0].pid, SIGINT);

	exit(EXIT_SUCCESS);

	return 0;
}


int open()
{

	printf("File name : ");
	fgets(buf, sizeof(buf), stdin);
	buf[strlen(buf)-1] = '\0';

	for(int i=0; i<num; i++)
	{
		fseek(stream, 0x2000 +(i * 0x400), SEEK_SET);
		for(int k=0; k<32; k++)
		{
			fread(&dentry, sizeof(dentry), 1, stream);

			if(strncmp(dentry.name, buf, sizeof(buf)) == 0)
			{
				printf("Found! File name : %s\n", dentry.name);
				printf("I-node = %d\n", dentry.inode);
				return 0;
			}
		}
	}		
	printf("Not Found\n");
	return 0;		
}

int read_pro()
{
        if((party.inode_table[dentry.inode].size % 1024)==0)
        {
                num = party.inode_table[dentry.inode].size / 1024;
        }
        else
        {
                num = (party.inode_table[dentry.inode].size / 1024) + 1;
        }

        for(int i=0; i<num; i++)
        {
                fseek(stream, 0x2000 + ((party.inode_table[dentry.inode].blocks[i])*0x400), SEEK_SET);
                fread(&userbuf, sizeof(userbuf), 1, stream);
                //printf("??= %d\n", (party.inode_table[dentry.inode].blocks[i]));
                printf("%s", userbuf.d);
        }
}

int mount()
{
        stream = fopen("disk.img", "r");
        if (stream == NULL) exit(EXIT_FAILURE);

        res = fread(&party, (sizeof(party.s)+sizeof(party.inode_table)) , 1, stream);
        printf("res = %d\n", party.s.first_inode);

        if((party.inode_table[party.s.first_inode].size % 1024)==0)
        {
                num = party.inode_table[party.s.first_inode].size / 1024;
        }
        else
        {
                num = (party.inode_table[party.s.first_inode].size / 1024) + 1;
                printf("%d\n", num);
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
