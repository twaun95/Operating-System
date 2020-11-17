#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

FILE *stream;
size_t len = 0;
struct partition party;
int res;
struct dentry dentry;
char buf[256];
struct blocks *ptr;
int num;
//char userbuf[1024];
struct blocks userbuf;
int check();

int main(void)
{
/*
	FILE *stream;
	size_t len = 0;
	struct partition party;
	struct dentry dentry;
	int res;
	char buf[256];
	struct blocks *ptr;
	int num;
*/

//mount
	stream = fopen("disk.img", "r");
	if (stream == NULL) exit(EXIT_FAILURE);

	res = fread(&party, sizeof(party), 1, stream);//read stream and load to party
	printf("res = %d\n", party.s.first_inode);//check party->superblock->firstinode


	//found firstinode, go inode_table's size, we know 1block is 1024byte
	//root inode's size is 0xcc0 = 3264 -> 3block(0,1,2)=if + 1block(3)=else
	//next, first inode' size is 0x28= <1024 -> 1block(4)
	//num = datablock 의총 사용되는개수== last entry number 
	if((party.inode_table[party.s.first_inode].size % 1024)==0)
	{
		num = party.inode_table[party.s.first_inode].size / 1024;
	}
	else
	{
		num = (party.inode_table[party.s.first_inode].size / 1024) + 1;
		printf("num = %d\n", num);
	}

	//root directory file, show all file in directory file --> ls
	for(int i=0; i<num; i++)
	{
		//dentry 시작주소set
		//
		fseek(stream, 0x2000 +(i * 0x400), SEEK_SET);
		for(int k=0; k<32; k++)
		{
			//struct dentry dentry[k] = party.data_blocks[i];
			fread(&dentry, sizeof(dentry), 1, stream);
			printf("name : %s\n", dentry.name);
		}
	}
//open

	
	check();//;
//	printf("I-node = %d\n", dentry.inode);
//	printf("inode's mode = %d\n", (party.inode_table[dentry.inode].size));

//read
//in open stage, we found inodenumber, go inode_table, find num
	if((party.inode_table[dentry.inode].size % 1024)==0)
	{
		num = party.inode_table[dentry.inode].size / 1024;
	}	
	else
	{
		num = (party.inode_table[dentry.inode].size / 1024) + 1; 
	}
	//find data
	for(int i=0; i<num; i++)
	{
		fseek(stream, 0x2000 + ((party.inode_table[dentry.inode].blocks[i])*0x400), SEEK_SET);
		fread(&userbuf, sizeof(userbuf), 1, stream); 
		//printf("??= %d\n", (party.inode_table[dentry.inode].blocks[i]));
		printf("%s", userbuf.d);	
	}

//close
	fclose(stream);
	exit(EXIT_SUCCESS);

	return 0;
}

//open
int check()
{
//user want to find file => buf
	printf("File name : ");
	fgets(buf, sizeof(buf), stdin);
	buf[strlen(buf)-1] = '\0';
//	puts(buf);
//in directory file, find file that user want to find
	for(int i=0; i<num; i++)
	{
		fseek(stream, 0x2000 +(i * 0x400), SEEK_SET);
		for(int k=0; k<32; k++)
		{
			fread(&dentry, sizeof(dentry), 1, stream);

			if(strncmp(dentry.name, buf, sizeof(buf)) == 0)//if find
			{
				//know that file's name, inode
				printf("Found! File name : %s\n", dentry.name);
				printf("I-node = %d\n", dentry.inode);
				return 1;
			}
		}
	}		
	printf("Not Found\n");//if not find
	return 0;		
}



