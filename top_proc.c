#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <time.h>

//provides information about processes using the /proc file system
#include <proc/readproc.h>

// proc_t information storage structure
typedef struct myproc_t
{
    int pid;              // POSIX thread ID
    float pcpu;           // CPU percentage
    unsigned long pmem;   // Memory in KB
    char cmd[16];         // process name
} myproc_t;

// Function prototypes
int myproc_comp_pcpu(const void *e1, const void *e2);
int myproc_comp_mem(const void *e1, const void *e2);
long long sample_processes(myproc_t **myprocs, unsigned int *myprocs_size, struct timespec sample_time);


int RAM_info()
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL)
    {
        perror("Sorry!");
    }

    int mem_total;
    int mem_free;

    char line[256];
    while (fgets(line, sizeof(line), meminfo))
    {
        sscanf(line, "MemTotal: %d kB", &mem_total);
        sscanf(line, "MemFree: %d kB", &mem_free);
    }
    // Unable to find lines we were looking for
    fclose(meminfo);

    // Return percentage of total memory being used
    return (mem_total - mem_free) * 100 / mem_total;
}


/// compare function for sorting proc_t* entries by thread ID, lowest first
int proc_comp_tid(const void *e1, const void *e2)
{
    proc_t *p1 = *(proc_t **)e1;
    proc_t *p2 = *(proc_t **)e2;

    if (p1->tid < p2->tid)
    {
        return -1;
    }
    else if (p1->tid > p2->tid)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/// compare function used to sort myproc_t entries by CPU usage in descending order
int myproc_comp_pcpu(const void *e1, const void *e2)
{
    myproc_t *p1 = (myproc_t *)e1;
    myproc_t *p2 = (myproc_t *)e2;

    if (p1->pcpu < p2->pcpu)
    {
        return 1;
    }
    else if (p1->pcpu > p2->pcpu)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/// compare function for sorting myproc_t entries by memory usage, highest first
int myproc_comp_mem(const void *e1, const void *e2)
{
    myproc_t *p1 = (myproc_t *)e1;
    myproc_t *p2 = (myproc_t *)e2;

    if (p1->pmem < p2->pmem)
    {
        return 1;
    }
    else if (p1->pmem > p2->pmem)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

// Sum up all different CPU times
unsigned long long get_total_cpu_time(int check)
{

    //open file /proc/stat which has information about kernel activity
    FILE *file = fopen("/proc/stat", "r");

    if (file == NULL)
    {
        perror("Sorry, unable to open stat file!\n");
        return 0;
    }

    char buffer[1024];
    unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guestnice = 0;

    //read data from file and write to buffer. returns number of bytes
    char *ret = fgets(buffer, sizeof(buffer) - 1, file);

    //if no bytes are read
    if (ret == NULL)
    {
        perror("Sorry, unable to read stat file!\n");
        fclose(file);
        return 0;
    }
    fclose(file);

    //read first line of data from buffer and save into variables.
    //this line contains the time the cpu has spent on each type of processes eg: user, kernel, idle etc.
    sscanf(buffer, "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestnice);

    // sum everything up (except guest and guestnice since they are already included
    // in user and nice, see http://unix.stackexchange.com/q/178045/20626)
    if (check == 0)
    {

        // sum the time of all the processes.
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    return user + nice + system;
}

// free up allocated memory
void freeproctab(proc_t **procs)
{
    proc_t **p;
    for (p = procs; *p; ++p)
    {
        freeproc(*p);
    }
    free(procs);
}

// main function to sample processes
long long sample_processes(myproc_t **myprocs, unsigned int *myprocs_size, struct timespec sample_time)
{
    unsigned long long atime[2];
    unsigned long long realtime;

    unsigned long long total_cpu_time = get_total_cpu_time(0);
    atime[0] = get_total_cpu_time(1);

    // number of cores
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    proc_t **procs1 = readproctab(PROC_FILLCOM | PROC_FILLSTAT | PROC_FILLSTATUS);
    if (!procs1)
    {
        perror("Sorry, readproctab() failed!");
        return;
    }
    unsigned int procs1_len;
    for (procs1_len = 0; procs1[procs1_len]; ++procs1_len)
    {
    }

    // sort processes using thread ID's
    qsort(procs1, procs1_len, sizeof(procs1[0]), proc_comp_tid);

    nanosleep(&sample_time, NULL);

    total_cpu_time = get_total_cpu_time(0) - total_cpu_time;
    atime[1] = get_total_cpu_time(1);
    realtime = (atime[1] - atime[0]) * 100 / total_cpu_time;

    proc_t **procs2 = readproctab(PROC_FILLCOM | PROC_FILLSTAT | PROC_FILLSTATUS);
    if (!procs2)
    {
        perror("Sorry, readproctab() failed!");
        return;
    }
    unsigned int procs2_len;
    for (procs2_len = 0; procs2[procs2_len]; ++procs2_len)
    {
    }

    qsort(procs2, procs2_len, sizeof(procs2[0]), proc_comp_tid);

    // use size of shorter list
    *myprocs_size = (procs1_len < procs2_len ? procs1_len : procs2_len);
    *myprocs = calloc(*myprocs_size * sizeof(myproc_t), sizeof(myproc_t));

    // merge
    unsigned int pos1 = 0, pos2 = 0;
    unsigned int newpos = 0;
    while (pos1 < procs1_len && pos2 < procs2_len)
    {
        if (procs1[pos1]->tid < procs2[pos2]->tid)
        {
            ++pos1;
        }
        else if (procs1[pos1]->tid > procs2[pos2]->tid)
        {
            ++pos2;
        }
        else
        {
            // in case a process is in both
            (*myprocs)[newpos].pid = procs2[pos2]->tid;
            (*myprocs)[newpos].pmem = procs2[pos2]->vm_rss;
            strncpy((*myprocs)[newpos].cmd, procs2[pos2]->cmd, 15);

            // calculate CPU usage during measurement
            unsigned long long cpu_time = ((procs2[pos2]->utime + procs2[pos2]->stime) - (procs1[pos1]->utime + procs1[pos1]->stime));
            (*myprocs)[newpos].pcpu = (cpu_time / (float)total_cpu_time) * 100.0 * num_cpus;

            ++pos1;
            ++pos2;
            ++newpos;
        }
    }
    //fprintf(stdout, "CPU Usage: %3lld%%\t\t", realtime);

    // free up allocated memory
    freeproctab(procs1);
    freeproctab(procs2);
    return realtime;
}
