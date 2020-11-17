struct msgbuf {
        int mtype;

        // pid will sleep for io_time
        int pid;
        int io_time;
        int cpu_time;
	char pathname[16];
	int O_RD;
	int inode_num;
};

struct msgaddr {
        int mtype;
        int pid;
        int VA[10];
};
