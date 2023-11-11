#include "disk.hpp"

#include "../util.hpp"

#include <memory> // unique_ptr

#define COLUMN_1    0                   // pid
#define COLUMN_2    COLUMN_1+8          // name
#define COLUMN_3    COLUMN_2+30         // read_bytes
#define COLUMN_4    COLUMN_3+8          // write_bytes
#define COLUMN_5    COLUMN_4+8          // read / s
#define COLUMN_6    COLUMN_5+11         // write / s
#define COLUMN_7    COLUMN_6+11         // uptime
#define COLUMN_8    COLUMN_7+13         // cmd

uint64_t DISK::get_pid_at_pos() {
    return processes.at(proc_table_pos).process.pid;
}

void DISK::find_disks() {
    std::vector<struct disk_device> new_devices;
    get_non_virtual_block_devices(&new_devices);

    // Check if disks are still or just connected
    struct disk_device disk;
	for (struct disk_device &dd : devices) {
        dd.connected = false;
        for (int j = 0; j < (int)new_devices.size(); j++) {
            disk = new_devices.at(j);
            if (dd.id == disk.id) {
                // Found
                dd.connected = true;
                std::vector<struct disk_device>::iterator it = (new_devices.begin() + j);
                new_devices.erase(it);
                break;
            }
        }
    }
	for (struct disk_device &dd : new_devices)
        devices.push_back(dd);

    // Clear disconnected disks
    erase_from_vector<struct disk_device>(&devices, [](struct disk_device disk) {
			return (!disk.connected);
		});

	for (struct disk_device &d : devices) {
        if (d.vendor == "")
			get_sys_block_device_vendor(d.id, &(d.vendor));
        if (d.model == "")
			get_sys_block_device_model(d.id, &(d.model));
        if (d.serial == "")
			get_sys_block_device_serial(d.id, &(d.serial));
        if (d.subsystem == "")
			get_sys_block_device_subsystem(d.id, &(d.subsystem));
        if (d.size == 0)
			get_sys_block_size(d.id, &(d.size));
    }
}

// TODO check if usable since its not really standardized
// void DISK::get_udevadm_data() {
//     const char* cmd = "udevadm info --query=property --name ";
//     std::array<char, 128> buffer;
//     std::string line;
//     std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
//     if (!pipe) {
//         throw std::runtime_error("popen() failed!");
//     }
// 	for (struct disk_device &d : devices) {
//         std::string command(cmd + d.id);
//         std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
//         if (!pipe) {
//             throw std::runtime_error("popen() failed!");
//         }
//
//         if (d.vendor == "")
// 			get_sys_block_device_vendor(d.id, &(d.vendor));
//         if (d.model == "")
// 			get_sys_block_device_model(d.id, &(d.model));
//         if (d.serial == "")
// 			get_sys_block_device_serial(d.id, &(d.serial));
//         if (d.subsystem == "")
// 			get_sys_block_device_subsystem(d.id, &(d.subsystem));
//     }
// }

void update_disk_process(struct disk_process *proc) {
    uint64_t old_uptime = proc->uptime;
    struct pid_io old_io = proc->io;

    get_pid_uptime(proc->process.pid, &proc->uptime);

    struct pid_status status = {};
    read_pid_status(proc->process.pid, &status);
    proc->swap = status.VmSwap.empty() ? 0 : std::stoul(status.VmSwap);

    read_pid_io(proc->process.pid, &proc->io);

    uint64_t time_delta = proc->uptime - old_uptime;
    if (time_delta) {
        proc->read_per_s = (proc->io.read_bytes - old_io.read_bytes)*1.0 / time_delta;
        proc->write_per_s = (proc->io.write_bytes - old_io.write_bytes)*1.0 / time_delta;
    } else {
        proc->read_per_s = 0;
        proc->write_per_s = 0;
    }
}

void DISK::find_disk_processes() {
	// Reset is_alive for all processes
	for (struct disk_process &p : processes)
			p.process.is_alive = false;

    // Get currently active processes
    std::vector<struct process> proc_vec;
    find_processes(&proc_vec);

    // Check if processes are new or already in vector
    struct process proc;
	for (struct disk_process &diskproc : processes) {
        for (int j = 0; j < (int)proc_vec.size(); j++) {
            proc = proc_vec.at(j);
            if (diskproc.process.pid == proc.pid) {
                // Found
                diskproc.process.is_alive = true;
                std::vector<struct process>::iterator it = (proc_vec.begin() + j);
                proc_vec.erase(it);
                break;
            }
        }
    }

    // All processes that haven't been found and removed should be added to processes vector
	for (struct process &p : proc_vec) {
        struct disk_process cp = {0};
        cp.process = p;
        processes.push_back(cp);
    }

    // Clear dead processes
    erase_from_vector<struct disk_process>(&processes, [](struct disk_process p) {
			return (!p.process.is_alive);
		});

    // Get rest of the fields
	for (struct disk_process &proc : processes)
       update_disk_process(&proc);
}

DISK::DISK() {
    // Need to have some time between sampling, if it's too close samples are basically 0
	wtimeout(tab_window, 1000);

    update();
};

void DISK::update() {
    // Info
    find_disks();
    set_info_block_size(devices.size());

    find_disk_processes();
    sort_vector<struct disk_process>(&processes,
        [](const struct disk_process p1, const struct disk_process p2) {
            return (p1.read_per_s + p1.write_per_s) > (p2.read_per_s + p2.write_per_s);
        });

    // If a disk was removed extend proc block up
    uint32_t previous_size = process_vector_size;
    process_vector_size = processes.size();
    if (process_vector_size != previous_size)
        for (uint32_t i = 0; i < process_vector_size; i++) {
            // Move cursor to beginning of row
            mvcur(tab_window->_curx, tab_window->_cury, info_block_start+i, 0);
            // Erase row
		    wclrtoeol(tab_window);
        }

    for (int i = 0; i < (int)devices.size(); i++) {
        mvwprintw(tab_window, info_block_start+i, 0, "%s:", devices.at(i).id.c_str());
        if (devices.at(i).vendor != "")
            mvwprintw(tab_window, info_block_start+i, 10, "[%s] %s %s %sB %s",
                devices.at(i).subsystem.c_str(), devices.at(i).vendor.c_str(),
                devices.at(i).model.c_str(), format_size(devices.at(i).size).c_str(),
                devices.at(i).serial.c_str());
        else
            mvwprintw(tab_window, info_block_start+i, 10, "[%s] %s %sB %s",
                devices.at(i).subsystem.c_str(),
                devices.at(i).model.c_str(), format_size(devices.at(i).size).c_str(),
                devices.at(i).serial.c_str());
    }

    // Proc
    uint32_t pos = proc_table_top;
    struct disk_process *proc;
    for (uint32_t i = 0; i <= proc_block_size; i++) {
        proc = &processes.at(pos);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_1, "%lu", proc->process.pid);
        /* Clear extra characters if previous value was longer */
		wclrtoeol(tab_window);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_2, "%s", proc->process.name.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_3, "%s", format_size(proc->io.read_bytes).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", format_size(proc->io.write_bytes).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_5, "%s/s", format_size(proc->read_per_s).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_6, "%s/s", format_size(proc->write_per_s).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_7, "%s", format_time(proc->uptime).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_8, "%s", proc->process.cmd.c_str());
        pos++;
        if (pos >= processes.size())
            break;
    }
    /* Invert the highlight of the currently selected process */
    mvwchgat(tab_window, proc_block_start+proc_table_pos, 0, -1, A_REVERSE, 0, NULL);
}