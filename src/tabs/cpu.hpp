#ifndef CPU_HPP_
#define CPU_HPP_

#include "tab.hpp"
#include "../proc.hpp"

#include <string>
#include <vector>

struct cpu_process {
	struct process      process = {0};
    struct pid_stat     stats = {};
	_Float32            usage_percent = 0;
    uint64_t            uptime = 0; // in seconds
    // cpu_uptime when last checked
    uint64_t            cpu_uptime = 0; // in seconds
};

class CPU : public Tab {
public:
    CPU();
    ~CPU() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    void update_cpu_process(struct cpu_process *proc);
    void find_cpu_processes();
private:
    std::vector<struct cpu_process> processes;
    struct cpu_stat cpu_stats;
    uint16_t ticks_per_s;
    // Start time in seconds since boot
    uint64_t system_uptime;
};

#endif // CPU_HPP_