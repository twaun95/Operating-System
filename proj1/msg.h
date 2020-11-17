struct msgbuf {
	int mtype;

	// pid will sleep for io_time
	int pid;
	int io_time;
	int cpu_time;
};
