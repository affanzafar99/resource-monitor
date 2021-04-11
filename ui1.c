#include <stdio.h>
#include "top_proc.c"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <proc/readproc.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


void cpu_top5();
void mem_top5();
void cpu_graph();
void mem_graph();
void gotoxy(int, int);
void sig_handler(int);
int writepipe(long long, char[]);


int sig = 0;
int x = 0;
bool script=false;
int main()
{
    
    while (1){
    	
        printf("Hello, welcome to our activity monitor!\n\n1. Top 5 CPU heavy processes\n"
        "2. Top 5 Memory heavy processes\n3. CPU Graph\n4. Memory Graph\n\nChoice: ");
        scanf("%d", &x);
        sig = 0;

        switch(x)
        {
            case 1: 
               printf("TOP 5 CPU");
                cpu_top5();
                break;
            case 2:
                printf("TOP 5 MEM");
                mem_top5();
                break;
            case 3:
                printf("CPU Graph");
                if(!script){
                	system("python3 cpu_graph.py &");
	                script=true;
                }
                cpu_graph();
                break;
            case 4:
                printf("Mem Graph");
                if(!script){
                	system("python3 mem_graph.py &");
	                script=true;
                }
                mem_graph();
                break;
            default: 
                printf("You did not enter 1!");
        }
        system("clear");
    }
}

void gotoxy(int x, int y)
{
	printf("%c[%d;%df", 0x1B, y, x);
}

void cpu_top5()
{
    system("clear");
    signal(SIGINT, sig_handler);
	
    while (sig != 1)
	{

		// define time interval for sampling processes
		struct timespec tm;
		tm.tv_sec = 0;
		tm.tv_nsec = 500 * 1000 * 1000; /* conversion to 500 ms */

		// myproc_t instance to store sampled information
		myproc_t *myprocs = NULL;
		unsigned int myprocs_len = 0;
		unsigned int i = 0;
		gotoxy(0, 2);

		// primary function to sample processes
		sample_processes(&myprocs, &myprocs_len, tm);
		//printf("Memory Usage: %d%%\n\n", RAM_info());

		// sort by decreasing CPU usage
		qsort(myprocs, myprocs_len, sizeof(myprocs[0]), myproc_comp_pcpu);

		// print
		printf("%-10s %-10s %-10s %-20s\n", "PID", "CPU \%", "Memory(KB)", "Process");
		for (i = 0; i < myprocs_len && i < 5; ++i)
		{
			if (strlen(myprocs[i].cmd) == 0)
			{
				break;
			}

			// display sorted processes in table
			printf("%-10d %-10.1f %-10lu %-20s\n", myprocs[i].pid, myprocs[i].pcpu, myprocs[i].pmem / 1000, myprocs[i].cmd);
		}

		printf("\n");

		//
		fflush(stdout);

		// deallocate memory allocated to myprocs, for next iteration
		free(myprocs);
		//wait for 1 second before running again
		sleep(1);
	}
}


void mem_top5()
{
    system("clear");
    signal(SIGINT, sig_handler);
    while (sig != 1)
	{
		// define time interval for sampling processes
		struct timespec tm;
		tm.tv_sec = 0;
		tm.tv_nsec = 500 * 1000 * 1000; /* conversion to 500 ms */

		// myproc_t instance to store sampled information
		myproc_t *myprocs = NULL;
		unsigned int myprocs_len = 0;
		unsigned int i = 0;

		gotoxy(0, 4);

		// primary function to sample processes
		sample_processes(&myprocs, &myprocs_len, tm);
		//printf("Memory Usage: %d%%\n\n", RAM_info());

		// sort by decreasing Memory usage
		qsort(myprocs, myprocs_len, sizeof(myprocs[0]), myproc_comp_mem);

		printf("%-10s %-10s %-10s %-20s\n", "PID", "CPU \%", "Memory(KB)", "Process");
		for (i = 0; i < myprocs_len && i < 5; ++i)
		{
			if (strlen(myprocs[i].cmd) == 0)
			{
				break;
			}
			// display sorted processes in table
			printf("%-10d %-10.1f %-10lu %-20s\n", myprocs[i].pid, myprocs[i].pcpu, myprocs[i].pmem / 1000, myprocs[i].cmd);
		}

		//
		fflush(stdout);

		// deallocate memory allocated to myprocs, for next iteration
		free(myprocs);
		//wait for 1 second before running again
		sleep(1);
    }
}





void cpu_graph(){
    signal(SIGINT, sig_handler);
    struct timespec tm;
	tm.tv_sec  = 0;
	tm.tv_nsec = 1000 * 1000 * 1000;
	myproc_t* myprocs = NULL;
	unsigned int myprocs_len = 0;

    //system("python3 cpu_graph.py &");
	while(sig!=1)
	{
		long long output = sample_processes(&myprocs, &myprocs_len, tm);
        	writepipe(output,"pipe/cpu_pipe");
        	sleep(1);
	}
}

void mem_graph(){
    signal(SIGINT, sig_handler);
    
	while(sig!=1)
	{
		//printf("Memory Usage: %d%%\n\n",RAM_info());
        	writepipe(RAM_info(),"pipe/mem_pipe");
        	usleep(500*1000);
	}
}


int writepipe(long long a, char pipename[])
{
    int fd;

    if(mkfifo(pipename, 0777)==-1) 
    {
        if(errno != EEXIST)
        {
            //printf("Could not create FIFO file.\n");
            return -1;
        }
    }
    
    fd = open(pipename, O_WRONLY);
    
    char b[2];
    sprintf(b,"%lld",a);
    char *x;
    x=b;
    
    //printf("%lld \n",a);
	
    
    if(write(fd, b, sizeof(b)) == -1)
    {
        printf("Write error\n");
        return 1;
    }

}

void sig_handler(int signum){
    sig = 1;
    x=6;
    script=false;
}
