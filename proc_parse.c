/* Author: Phillip Stewart
CPSC 351 Project 4, proc_parse
*/

#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "unistd.h"

#define BUF_LEN 120


// #### Part 1 functions ######################################################
int get_cores();
void print_processor();
void print_version();
void print_memory_usage();
void print_uptime();


int get_cores() {
	FILE* fp = fopen("/proc/cpuinfo", "r");
	char buffer[BUF_LEN];
	int cores = 0;

	while (fgets(buffer, BUF_LEN, fp) != NULL) {
		if (strncmp(buffer, "processor\t", 10) == 0) {
			++cores;
		}
	}
	fclose(fp);
	return cores;
}


void print_processor() {
	FILE* fp = fopen("/proc/cpuinfo", "r");
	char buffer[BUF_LEN];

	while (fgets(buffer, BUF_LEN, fp) != NULL) {
		if (strncmp(buffer, "model name", 10) == 0) {
			char* cpu_model = strchr(buffer, ':') + 2;
			cpu_model[strcspn(cpu_model, "\n")] = 0;
			printf("Processor:       %s\n", cpu_model);
			break;
		}
	}
	fclose(fp);
	printf("Processor cores: %d\n", get_cores());
}


void print_version() {
	FILE* fp = fopen("/proc/version_signature", "r");
	char buffer[BUF_LEN];

	fgets(buffer, BUF_LEN, fp);
	buffer[strcspn(buffer, "\n")] = 0;
	printf("Kernel version:  %s\n", buffer);
	fclose(fp);
}


void print_memory_usage() {
	FILE* fp = fopen("/proc/meminfo", "r");
	char buffer[BUF_LEN];

	fgets(buffer, BUF_LEN, fp);
	buffer[strcspn(buffer, "\n")] = 0;
	char* c = buffer;
	while (*c != ':') c++;
	c++;
	while (*c == ' ') c++;
	printf("Total RAM:       %s\n", c);
	fclose(fp);
}


void print_uptime() {
	FILE* fp = fopen("/proc/uptime", "r");
	float up, idle;

	fscanf(fp, "%f %f", &up, &idle);
	int seconds = (int)up;
	int hours = seconds / 3600;
	seconds %= 3600;
	int minutes = seconds / 60;
	seconds %= 60;

	printf("Uptime:          %02dH:%02dM:%02dS\n", hours, minutes, seconds);
	fclose(fp);
}


// #### Part 2 functions ######################################################
/* A ton of globals to track it all */
int read_rate, print_rate;
int ticks, last_printed;
int user_ticks, sys_ticks, idle_ticks;
int last_u, last_s, last_i;
float user_percent, sys_percent, idle_percent;
int free_mem;
float mem_percent;
int disk_reads, disk_writes, last_dr, last_dw;
float disk_read_rate, disk_write_rate;
int context_switches, last_c_sw;
float context_switch_rate;
int num_processes, last_num_p;
float process_creation_rate;

void update_lasts();
void set_initial();
void readers();
void read_processor_ticks();
void read_memory_percents();
void read_disk_stats();
void read_context_switches();
void read_process_creations();
void calc_percents();
void writers();
void print_processor_percents();
void print_memory_percents();
void print_disk_rate();
void print_context_switch_rate();
void print_process_creation_rate();


void update_lasts() {
	last_printed = ticks;
	last_u = user_ticks;
	last_s = sys_ticks;
	last_i = idle_ticks;

	last_dr = disk_reads;
	last_dw = disk_writes;

	last_c_sw = context_switches;
	last_num_p = num_processes;
}


void set_initial() {
	readers();
	update_lasts();
}


void readers() {
	read_processor_ticks();
	read_memory_percents();
	read_disk_stats();
	read_context_switches();
	read_process_creations();
}


void read_processor_ticks() {
	FILE* fp = fopen("/proc/stat", "r");
	char buffer[BUF_LEN];
	//cpu,user,nice,system,idle,iowait,irq,softrig,steal,guest,guest_nice
	fgets(buffer, BUF_LEN, fp);
	char* c = strtok(buffer, " ");
	int i;
	ticks = 0;
	for (i=0; i < 9; i++) {
		c = strtok(NULL, " ");
		ticks += atoi(c);
		if (i == 0) {//User
			user_ticks = atoi(c);
		} else if (i == 2) {//System
			sys_ticks = atoi(c);
		} else if (i == 3) {//Idle
			idle_ticks = atoi(c);
		}
	}
	fclose(fp);
}


void read_memory_percents() {
	FILE* fp = fopen("/proc/meminfo", "r");
	char buffer[BUF_LEN];

	fgets(buffer, BUF_LEN, fp);
	buffer[strcspn(buffer, "\n")] = 0;
	char* c = buffer;
	while (*c != ':') c++;
	c++;
	while (*c == ' ') c++;
	int total_mem = atoi(c);

	fgets(buffer, BUF_LEN, fp);
	buffer[strcspn(buffer, "\n")] = 0;
	c = buffer;
	while (*c != ':') c++;
	c++;
	while (*c == ' ') c++;
	free_mem = atoi(c);
	mem_percent = (float)free_mem / (float)total_mem * 100;
	fclose(fp);
}


void read_disk_stats() {
	FILE* fp = fopen("/proc/diskstats", "r");
	char buffer[BUF_LEN];
	char* c;
	disk_reads = 0;
	disk_writes = 0;

	while (fgets(buffer, BUF_LEN, fp) != NULL) {
		// #4 = reads, #8 = writes
		c = strtok(buffer, " ");
		int i;
		for (i=0; i < 3; i++) {
			c = strtok(NULL, " ");
		}
		disk_reads += atoi(c);
		for (i=0; i<4; i++) {
			c = strtok(NULL, " ");
		}
		disk_writes += atoi(c);
	}
	fclose(fp);
}


void read_context_switches() {
	FILE* fp = fopen("/proc/stat", "r");
	char buffer[BUF_LEN];

	while (fgets(buffer, BUF_LEN, fp) != NULL) {
		if (strncmp(buffer, "ctxt", 4) == 0) {
			sscanf(buffer, "ctxt %d", &context_switches);
			break;
		}
	}
	fclose(fp);
}


void read_process_creations() {
	FILE* fp = fopen("/proc/stat", "r");
	char buffer[BUF_LEN];

	while (fgets(buffer, BUF_LEN, fp) != NULL) {
		if (strncmp(buffer, "processes", 9) == 0) {
			sscanf(buffer, "processes %d", &num_processes);
			break;
		}
	}
	fclose(fp);
}


void calc_percents() {
	int diff = ticks - last_printed;
	if (diff == 0) {
		user_percent = 0.0;
		sys_percent = 0.0;
		idle_percent = 0.0;
	} else {
		float tick_diff = (float)(diff);
		user_percent = (float)(user_ticks - last_u) * 100 / tick_diff;
		sys_percent = (float)(sys_ticks - last_s) * 100 / tick_diff;
		idle_percent = (float)(idle_ticks - last_i) * 100 / tick_diff;
	}
	disk_read_rate = (disk_reads - last_dr) / (float)print_rate;
	disk_write_rate = (disk_writes - last_dw) / (float)print_rate;
	context_switch_rate = (context_switches - last_c_sw) / (float)print_rate;
	process_creation_rate = (num_processes - last_num_p) / (float)print_rate;
}


void writers() {
	print_processor_percents();
	print_memory_percents();
	print_disk_rate();
	print_context_switch_rate();
	print_process_creation_rate();
	printf("\n");
}


void print_processor_percents() {
	printf("User: %0.02f  System: %0.02f  Idle: %0.02f\n",
			user_percent, sys_percent, idle_percent);
}


void print_memory_percents() {
	printf("Free memory: %d kB  (%0.02f%%)\n", free_mem, mem_percent);
}


void print_disk_rate() {
	printf("Disk read: %0.02f, write: %0.02f  (sectors per second)\n",
		disk_read_rate, disk_write_rate);
}


void print_context_switch_rate() {
	printf("Context switches: %0.02f (switches per second)\n",
		context_switch_rate);
}


void print_process_creation_rate() {
	printf("Process creation rate: %0.02f (processes per second)\n",
		process_creation_rate);
}


// #### Usage #################################################################
void print_usage(char* prog_name) {
	printf("Usage:\n");
	printf("\tShow values: $ %s\n", prog_name);
	printf("\tShow rates : $ %s <read_rate> <print_rate>\n", prog_name);
}


// #### Main ##################################################################
int main(int argc, char** argv)
{
	if (argc == 1) {
		// Part 1
		print_processor();
		print_version();
		print_memory_usage();
		print_uptime();
	} else if (argc == 3){
		// Part 2
		set_initial();
		read_rate = atoi(argv[1]);
		print_rate = atoi(argv[2]);
		int seconds = 0;
		while (1) {
			if (seconds % read_rate == 0) {
				readers();
			}

			if (seconds % print_rate == 0) {
				calc_percents();
				writers();
				update_lasts();
			}

			++seconds;
			sleep(1);
		}
	} else {
		print_usage(argv[0]);
	}

	return 0;
}