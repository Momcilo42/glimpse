#ifndef GPU_HPP_
#define GPU_HPP_

#include "tab.hpp"
#include "../proc.hpp"

struct gpu_process {
	struct process process = {0};
	_Float32 usage_percent = 0;
    uint64_t usage_ns = 0;
    // system uptime when last checked
    uint64_t last_checked = 0; // in seconds
    int32_t vram = -1; // in B
    uint64_t uptime = 0;
    std::string card_num = "";
};

struct gpu_device {
    std::string name = "";
    std::string vendor = "";
    std::string card_num = "";
    std::string pci_address = "";
    uint32_t clock = 0; // in MHz
    std::string driver = "";
};

class GPU : public Tab {
public:
    GPU();
    ~GPU() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    void update_gpu_process(struct gpu_process *proc);
    void find_gpu_processes();
private:
    std::vector<struct gpu_process> processes;
    std::vector<struct gpu_device> devices;
};

#endif // GPU_HPP_