#ifndef DISK_HPP_
#define DISK_HPP_

#include "tab.hpp"
#include "../proc.hpp"
#include "../sys.hpp"

struct disk_process {
	struct process      process = {0};
    uint64_t            uptime = 0; // in seconds
    struct pid_io       io = {0};
    double              read_per_s = 0;
    double              write_per_s = 0;
    uint64_t            swap = 0; // in kB
};

class DISK : public Tab {
public:
    DISK();
    ~DISK() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    void find_disks();
    // void get_udevadm_data();
    void find_disk_processes();
private:
    std::vector<struct disk_process> processes;
    std::vector<struct disk_device> devices;
};


#endif // DISK_HPP_