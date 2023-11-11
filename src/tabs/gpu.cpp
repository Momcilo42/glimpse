#include "gpu.hpp"

#include "../sys.hpp"
#include "../util.hpp"

#include <memory> // unique_ptr
#include <dirent.h> // DIR, struct dirent, opendir()
#include <cstring> // strncmp
#include <fstream> // getline

#define COLUMN_1    0           // pid
#define COLUMN_2    COLUMN_1+8  // name
#define COLUMN_3    COLUMN_2+30 // usage
#define COLUMN_4    COLUMN_3+8  // vram
#define COLUMN_5    COLUMN_4+8  // card
#define COLUMN_6    COLUMN_5+8  // uptime
#define COLUMN_7    COLUMN_6+13 // command

uint64_t GPU::get_pid_at_pos() {
    return processes.at(proc_table_pos).process.pid;
}

void get_pid_gpu_vram(const uint64_t pid, int32_t *vram) {
	std::string proc_path;
	std::string proc_path_fd;
	std::string proc_path_fdinfo;
	std::string fd_path;
	std::string fdinfo_path;
	int len = 0;
    constexpr int size = 1024;
	char symlink[size];

	proc_path = "/proc/" + std::to_string(pid);
	proc_path_fd = proc_path + "/fd";
	proc_path_fdinfo = proc_path + "/fdinfo";
    // Get fd-s that link to the GPUs render node
    std::vector<std::string> render_fds = {};
	DIR *fd_dir = opendir(proc_path_fd.c_str());
	struct dirent *file;
	if(fd_dir == NULL) {
		// perror("Unable to read directory");
		return;
	}

	/* Read the contents of the fd directory */
	while((file = readdir(fd_dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		fd_path = proc_path_fd + "/" + file->d_name;

		/* Read the actual symlink to get path */
		if ((len = readlink(fd_path.c_str(), symlink, size)) != -1)
			symlink[len] = '\0';
		else {
			// fprintf(stderr, "Can't read link %s\n", fdinfo_path.c_str());
			continue;
		}

		/* Check if the process has opened render or primary node */
        char symlink[size];
        readlink(fd_path.c_str(), symlink, size);
        std::string sym_link(symlink);
        if (sym_link.find("/dev/dri/renderD") != std::string::npos)
            render_fds.push_back(file->d_name);
    }
	closedir(fd_dir);
    if (render_fds.empty())
        return;
    // Check their appropriate fdinfo for drm-engine-render/drm-engine-gfx
    std::string line = "";
    int delim_pos;
    *vram = -1;
    for (std::string fdinfo_node : render_fds) {
        fdinfo_path = proc_path_fdinfo + "/" + fdinfo_node;
        std::ifstream infile(fdinfo_path);
        while (std::getline(infile, line)) {
            if (line.find("drm-memory-vram") == std::string::npos)
                continue;
            delim_pos = line.find_last_of(" ");
            line = line.substr(0, delim_pos);
            delim_pos = line.find_last_of(" \t");
            line = line.substr(delim_pos+1);
            *vram = std::stoull(line);
            break;
        }
        if (*vram)
            break;
    }
}

void get_pid_gpu_usage(const uint64_t pid, uint64_t *usage) {
	std::string proc_path;
	std::string proc_path_fd;
	std::string proc_path_fdinfo;
	std::string fd_path;
	std::string fdinfo_path;
	int len = 0;
    constexpr int size = 1024;
	char symlink[size];

	proc_path = "/proc/" + std::to_string(pid);
	proc_path_fd = proc_path + "/fd";
	proc_path_fdinfo = proc_path + "/fdinfo";
    // Get fd-s that link to the GPUs render node
    std::vector<std::string> render_fds = {};
	DIR *fd_dir = opendir(proc_path_fd.c_str());
	struct dirent *file;
	if(fd_dir == NULL) {
		// perror("Unable to read directory");
		return;
	}

	/* Read the contents of the fd directory */
	while((file = readdir(fd_dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		fd_path = proc_path_fd + "/" + file->d_name;

		/* Read the actual symlink to get path */
		if ((len = readlink(fd_path.c_str(), symlink, size)) != -1)
			symlink[len] = '\0';
		else {
			// fprintf(stderr, "Can't read link %s\n", fdinfo_path.c_str());
			continue;
		}

		/* Check if the process has opened render or primary node */
        char symlink[size];
        readlink(fd_path.c_str(), symlink, size);
        std::string sym_link(symlink);
        if (sym_link.find("/dev/dri/renderD") != std::string::npos)
            render_fds.push_back(file->d_name);
    }
	closedir(fd_dir);
    if (render_fds.empty())
        return;
    // Check their appropriate fdinfo for drm-engine-render/drm-engine-gfx
    std::string line = "";
    int delim_pos;
    *usage = 0;
    for (std::string fdinfo_node : render_fds) {
        fdinfo_path = proc_path_fdinfo + "/" + fdinfo_node;
        std::ifstream infile(fdinfo_path);
        while (std::getline(infile, line)) {
            if ((line.find("drm-engine-render") == std::string::npos) &&
                (line.find("drm-engine-gfx") == std::string::npos))
                continue;
            delim_pos = line.find_last_of(" ");
            line = line.substr(0, delim_pos);
            delim_pos = line.find_last_of(" \t");
            line = line.substr(delim_pos+1);
            *usage = std::stoull(line);
            break;
        }
        if (*usage)
            break;
    }
}

void GPU::update_gpu_process(struct gpu_process *proc) {
    uint64_t old_last_checked = proc->last_checked;
    uint64_t old_usage_ns = proc->usage_ns;

    get_pid_uptime(proc->process.pid, &proc->uptime);

    get_uptime(&proc->last_checked);
    get_pid_gpu_usage(proc->process.pid, &proc->usage_ns);
    _Float32 cpu_time_delta = (proc->last_checked - old_last_checked);
    _Float32 gpu_time_delta = (proc->usage_ns - old_usage_ns);
    // Convert to seconds
    gpu_time_delta /= 1000000000.0;
    _Float32 usage = gpu_time_delta / cpu_time_delta * 100.0;
    // If really quick refresh happens protect against divide by 0
    proc->usage_percent = cpu_time_delta ? usage : 0;

    get_pid_gpu_vram(proc->process.pid, &proc->vram);
}

void GPU::find_gpu_processes() {
	// Reset is_alive for all processes
	for (struct gpu_process &p : processes)
			p.process.is_alive = false;

    struct card_process {
        struct process proc = {};
        std::string card = "";
    };

    // Get currently active processes
    std::vector<struct dri_client> clients = {};
    struct card_process new_proc = {};
    std::vector<struct card_process> proc_vec = {};
	for (struct gpu_device &dev : devices) {
        clients = {};
        // Remove "card" from name to get id number
        get_sys_dri_clients(dev.card_num.substr(4), &clients);
	    for (struct dri_client &client : clients) {
            new_proc.proc.pid = client.pid;
            new_proc.proc.name = client.command;
            new_proc.proc.is_alive = true;
            get_calling_command(client.pid, &new_proc.proc.cmd);
            new_proc.card = dev.card_num;
            proc_vec.push_back(new_proc);
        }
    }

    // Check if processes are new or already in vector
    struct card_process proc;
	for (struct gpu_process &gpuproc : processes) {
        for (int j = 0; j < (int)proc_vec.size(); j++) {
            proc = proc_vec.at(j);
            if (gpuproc.process.pid == proc.proc.pid) {
                // Found
                gpuproc.process.is_alive = true;
                gpuproc.card_num = proc.card;
                proc_vec.erase(proc_vec.begin() + j);
                break;
            }
        }
    }

    // All processes that haven't been found and removed should be added to processes vector
	for (struct card_process &p : proc_vec) {
        struct gpu_process gp = {0};
        gp.process = p.proc;
        gp.card_num = p.card;
        processes.push_back(gp);
    }

    // Clear dead processes
    erase_from_vector<struct gpu_process>(&processes, [](struct gpu_process p) {
			return (!p.process.is_alive);
		});

    // Get rest of the fields for cpuproc
	for (struct gpu_process &gpuproc : processes)
        GPU::update_gpu_process(&gpuproc);
}

void get_gpu_model_name(std::string pci_addr, std::string *name) {
    int delim_pos = pci_addr.find_first_of(":");
    if (delim_pos != -1)
        pci_addr = pci_addr.substr(delim_pos+1);
    std::string cmd = "lspci | grep VGA | grep " + pci_addr;
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    if (fgets(buffer.data(), buffer.size(), pipe.get()) == nullptr) {
        // Try looking for headless GPUs
        cmd = "lspci | grep \"Display controller\" | grep " + pci_addr;
        std::unique_ptr<FILE, decltype(&pclose)> pipe2(popen(cmd.c_str(), "r"), pclose);
        if (!pipe2) {
            throw std::runtime_error("popen() failed!");
        }
        if (fgets(buffer.data(), buffer.size(), pipe2.get()) == nullptr)
            return;
    }

    *name = buffer.data();
    // Remove " (rev x)"
    *name = name->substr(0, name->find_last_of("(")-1);
    // Remove pci addr
    *name = name->substr(pci_addr.size()+1);
    // Remove label
    *name = name->substr(name->find_first_of(":")+2);
    // Remove new line if it's at the end
    *name = name->substr(0, name->find_last_of("\n"));
}

void get_pci_addr_from_card(std::string card, std::string *addr) {
    constexpr int size = 1024;
    char symlink[size];
	std::string link_path = "/sys/class/drm/" + card;
    readlink(link_path.c_str(), symlink, size);

    std::string sym_link(symlink);

    sym_link = sym_link.substr(0, sym_link.find("/drm"));
    sym_link = sym_link.substr(sym_link.find_last_of("/")+1);

    *addr = sym_link;
}

void get_gpu_devices(std::vector<struct gpu_device> *devices) {
	DIR *dir;
	struct dirent *file;
	const std::string path = "/dev/dri/";

    dir = opendir(path.c_str());
	if(dir == NULL) {
		// perror("Unable to read directory /dev/dri/");
		return;
	}

    std::vector<std::string> cards = {};

	/* Read the contents of the directory */
	while((file = readdir(dir))) {
        std::string name(file->d_name);
        if (name.find("card") == std::string::npos)
            continue;
        cards.push_back(name);
    }
	closedir(dir);
    if(cards.empty())
        return;

    struct gpu_device gpu = {};
    for (std::string card : cards) {
        gpu = {};
        gpu.card_num = card;
        get_pci_addr_from_card(card, &gpu.pci_address);
        get_sys_pci_device_vendor(gpu.pci_address, &gpu.vendor);
        get_sys_pci_device_driver(gpu.pci_address, &gpu.driver);
        get_gpu_model_name(gpu.pci_address, &gpu.name);
        // Remove vendor name if it's in the name
        if (gpu.name.size() && (gpu.name.size() >= gpu.vendor.size()) &&
            (gpu.vendor == gpu.name.substr(0, gpu.vendor.size())))
            gpu.name = gpu.name.substr(gpu.vendor.size()+1);
        devices->push_back(gpu);
    }
}

GPU::GPU() {
    // Need to have some time between sampling, if it's too close samples are basically 0
	wtimeout(tab_window, 1000);

    get_gpu_devices(&devices);

    int offset = 0;
    for (struct gpu_device gpu : devices) {
        mvwprintw(tab_window, info_block_start + offset++, 0, "%s: [%s] Driver: %s",
            gpu.card_num.c_str(), gpu.pci_address.c_str(), gpu.driver.c_str());
        mvwprintw(tab_window, info_block_start + offset++, gpu.card_num.size()+2, "%s",
            gpu.name.c_str());
    }
    set_info_block_size(offset);

    update();
};

void GPU::update() {
	find_gpu_processes();
    sort_vector<struct gpu_process>(&processes,
        [](const struct gpu_process p1, const struct gpu_process p2) {
            return p1.usage_percent > p2.usage_percent;
        });
    for (uint32_t i = 0; i < devices.size(); i++) {
        struct gpu_device *gpu = &devices[i];
        get_sys_freq(gpu->card_num.substr(4), &gpu->clock);
        int width = gpu->card_num.size() + 3 +
                    gpu->pci_address.size() +  10 +
                    gpu->driver.size() + 1;
        mvwprintw(tab_window, info_block_start + 2*i, width, "@ %uMHz     ", gpu->clock);
    }

    // Proc
    uint32_t pos = proc_table_top;
    struct gpu_process *proc;
    for (uint32_t i = 0; i <= proc_block_size; i++) {
        if (pos >= processes.size()) {
            mvwprintw(tab_window, proc_block_start+i, 0, " ");
		    wclrtoeol(tab_window);
            continue;
        }
        proc = &processes.at(pos);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_1, "%lu", proc->process.pid);
        /* Clear extra characters if previous value was longer */
		wclrtoeol(tab_window);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_2, "%s", proc->process.name.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_3, "%6.2f", proc->usage_percent);
        if (proc->vram == -1)
            mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", "Absent");
        else
            mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", format_size(proc->vram).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_5, "%s", proc->card_num.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_6, "%s", format_time(proc->uptime).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_7, "%s", proc->process.cmd.c_str());
        pos++;
        if (pos >= processes.size())
            break;
    }
    /* Invert the highlight of the currently selected process */
    mvwchgat(tab_window, proc_block_start+proc_table_pos, 0, -1, A_REVERSE, 0, NULL);

    // If a process ends, clear the unused line
    uint32_t previous_size = process_vector_size;
    process_vector_size = processes.size();
    if (process_vector_size != previous_size)
        for (uint32_t i = process_vector_size; i < proc_block_size; i++) {
            mvwprintw(tab_window, proc_block_start+i, 0, " ");
            // Erase row
		    wclrtoeol(tab_window);
        }
}