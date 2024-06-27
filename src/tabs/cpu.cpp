#include "cpu.hpp"

#include "../util.hpp"

#include <dirent.h> // DIR, struct dirent, opendir()
#include <unistd.h>
#include <bits/stdc++.h> // sort
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip> // put_time

#define COLUMN_1    0
#define COLUMN_2    COLUMN_1+8
#define COLUMN_3    COLUMN_2+30
#define COLUMN_4    COLUMN_3+8
#define COLUMN_5    COLUMN_4+13

uint64_t CPU::get_pid_at_pos() {
    return processes.at(proc_table_pos).process.pid;
}

void get_temps(std::vector<double> *temps) {
	DIR *dir;
	struct dirent *file;
    FILE *fp_type;
    FILE *fp_val;
	const std::string thermal_path = "/sys/class/thermal/";
	std::string temp_type_path;
	std::string temp_val_path;
	char type[100];
    float value = 0;
    const std::string desired_type = "x86_pkg_temp";

    temps->clear();

    dir = opendir(thermal_path.c_str());
	if(dir == NULL) {
		// perror("Unable to read directory /sys/class/thermal/");
		return;
	}

	/* Read the contents of the directory */
	while((file = readdir(dir))) {
		/* Ignore self links/hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		/* Append fd/fdinfo to path*/
		temp_type_path = thermal_path + file->d_name + "/type";
		temp_val_path = thermal_path + file->d_name + "/temp";

        fp_type = fopen(temp_type_path.c_str(), "r");
		if (fp_type == NULL) {
			// fprintf(stderr, "Can't open %s\n", temp_type_path.c_str());
			exit(1);
		}

        // TODO check if can be switched to std::getline ... possibly remove cstring include after
	    if (fscanf(fp_type, "%s%*[^\n]", type) > 0) {
            if (!strncmp(type, desired_type.c_str(), desired_type.size())) {
                fp_val = fopen(temp_val_path.c_str(), "r");
                if (fp_val == NULL) {
                    // fprintf(stderr, "Can't open %s\n", temp_val_path.c_str());
                    exit(1);
                }
                if (fscanf(fp_val, "%f%*[^\n]", &value) > 0) {
                    temps->push_back(value / 1000);
                }
                fclose(fp_val);
            }
        }
        fclose(fp_type);
	}
	closedir(dir);
}

void CPU::update_cpu_process(struct cpu_process *proc) {
    uint64_t previous_cpu_uptime = proc->cpu_uptime;
    struct pid_stat previous_pid_stats = proc->stats;

    read_pid_stat(proc->process.pid, &proc->stats);

    // Convert from ticks to seconds
    proc->stats.starttime = proc->stats.starttime / ticks_per_s;
    proc->uptime = system_uptime - proc->stats.starttime;

    get_uptime(&proc->cpu_uptime);

    double cpu_time_delta = (proc->cpu_uptime - previous_cpu_uptime);
    double proc_stat_times = proc->stats.utime + proc->stats.stime +
                               proc->stats.cutime + proc->stats.cstime;
    double previous_stat_times = previous_pid_stats.utime + previous_pid_stats.stime +
                                   previous_pid_stats.cutime + previous_pid_stats.cstime;
    double proc_time_delta = static_cast<double>((proc_stat_times - previous_stat_times)) / ticks_per_s;
    double usage = static_cast<double>(proc_time_delta / cpu_time_delta) * 100;
    // If really quick refresh happens protect against divide by 0
    proc->usage_percent = cpu_time_delta ? usage : 0;
}

void CPU::find_cpu_processes() {
	// Reset is_alive for all processes
	for (struct cpu_process &p : processes)
			p.process.is_alive = false;

    // Get currently active processes
    std::vector<struct process> proc_vec;
    find_processes(&proc_vec);

    // Check if processes are new or already in vector
    struct process proc;
	for (struct cpu_process &cpuproc : processes) {
        for (int j = 0; j < (int)proc_vec.size(); j++) {
            proc = proc_vec.at(j);
            if (cpuproc.process.pid == proc.pid) {
                // Found
                cpuproc.process.is_alive = true;
                std::vector<struct process>::iterator it = (proc_vec.begin() + j);
                proc_vec.erase(it);
                break;
            }
        }
    }

    // All processes that haven't been found and removed should be added to processes vector
	for (struct process &p : proc_vec) {
        struct cpu_process cp = {0};
        cp.process = p;
        processes.push_back(cp);
    }

    // Clear dead processes
    erase_from_vector<struct cpu_process>(&processes, [](struct cpu_process p) {
			return (!p.process.is_alive);
		});

    // Get rest of the fields for cpuproc
	for (struct cpu_process &cpuproc : processes)
        CPU::update_cpu_process(&cpuproc);
}

CPU::CPU() {
    // Need to have some time between sampling, if it's too close samples are basically 0
	wtimeout(tab_window, 1000);
    set_info_block_size(5);

    ticks_per_s = sysconf(_SC_CLK_TCK);

    update();
}

void CPU::update() {
	find_cpu_processes();
    sort_vector<struct cpu_process>(&processes,
        [](const struct cpu_process p1, const struct cpu_process p2) {
            return p1.usage_percent > p2.usage_percent;
        });
    process_vector_size = processes.size();

    // Info
    std::vector<struct cpuinfo_core> cores;
    read_cpuinfo(&cores);
    mvwprintw(tab_window, info_block_start, 0, "Model: %s", cores[0].model_name.c_str());
    uint8_t num_sockets = cores.back().physical_id + 1;
    mvwprintw(tab_window, info_block_start+1, 0, "Cores/Threads: %u/%lu", cores[0].cpu_cores*num_sockets, cores.size());
    std::vector<double> temps = {};
    get_temps(&temps);
    mvwprintw(tab_window, info_block_start+2, 0, "Temp:");
    for (uint32_t i = 0; i < temps.size(); i++) {
        mvwprintw(tab_window, info_block_start+2, 5+i*7, " %.1f°C        ", (double)temps[i]);
        wrefresh(tab_window);
    }
    double avg_clock = 0;
    for (int i = 0; i < (int)cores.size(); i++)
        avg_clock += cores[i].cpu_MHz;
    avg_clock /= cores[0].siblings;
    mvwprintw(tab_window, info_block_start+3, 0, "Avg clock speed: %.2fMHz", (double)avg_clock);
    // system_uptime get updated through update_cpu_process
    get_uptime(&system_uptime);
    mvwprintw(tab_window, info_block_start+4, 0, "Uptime: %s", format_time(system_uptime).c_str());

    // Proc
    uint32_t pos = proc_table_top;
    struct cpu_process *proc;
    for (uint32_t i = 0; i <= proc_block_size; i++) {
        if (pos >= processes.size()) {
            mvwprintw(tab_window, proc_block_start+i, 0, " ");
		    wclrtoeol(tab_window);
            continue;
        }
        proc = &processes.at(pos);
        // CPU::update_cpu_process(proc);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_1, "%lu", proc->process.pid);
        /* Clear extra characters if previous value was longer */
		wclrtoeol(tab_window);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_2, "%s", proc->process.name.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_3, "%6.2f", (double)proc->usage_percent);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", format_time(proc->uptime).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_5, "%s", proc->process.cmd.c_str());
        pos++;
        if (pos >= processes.size())
            break;
    }
    /* Invert the highlight of the currently selected process */
    mvwchgat(tab_window, proc_block_start+proc_table_pos, 0, -1, A_REVERSE, 0, NULL);
};