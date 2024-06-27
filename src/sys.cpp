#include "sys.hpp"

#include <fstream>
#include <sys/utsname.h>
#include <dirent.h> // DIR, struct dirent, opendir()
#include <cstring> // strncmp
#include <unistd.h> // readlink

#include "id_lists/pci_vendors.hpp"

void get_os_release(std::string *release) {
	std::string filepath("/etc/os-release");
    std::ifstream infile(filepath);
    if (!infile.is_open())
        return;
    std::string token;
    // Skip descriptor
    std::getline(infile, token, '\"');

    std::getline(infile, *release, '\"');
}

void get_kernel_release(std::string *release) {
    struct utsname osInfo{};
    uname(&osInfo);
    *release = osInfo.release;
}

void get_product_name(std::string *name) {
	std::string filepath("/sys/devices/virtual/dmi/id/product_name");
    std::ifstream infile(filepath);
    if (!infile.is_open())
        return;

    infile >> *name;
}

void get_non_virtual_block_devices(std::vector<struct disk_device> *devices) {
	DIR *dir;
	struct dirent *file;

	dir = opendir("/sys/block");
	if(dir == NULL) {
		// perror("Unable to read directory /sys/block");
		return;
	}

	/* Read the contents of the directory */
	while((file = readdir(dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

        std::string name(file->d_name);

        // Ignore loop devices
        if (name.find("loop") != std::string::npos)
            continue;

        struct disk_device disk;
        disk.id = name;
        disk.connected = true;

        devices->push_back(disk);
    }

	closedir(dir);
}

void get_non_virtual_network_devices(std::vector<struct net_device> *devices) {
	DIR *dir;
	struct dirent *file;
    constexpr int size = 1024;
	std::string filepath(std::string("/sys/class/net/"));

	dir = opendir(filepath.c_str());
	if(dir == NULL) {
		// perror("Unable to read directory /sys/class/net");
		return;
	}

	/* Read the contents of the directory */
	while((file = readdir(dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

        std::string link_path(filepath + file->d_name);

        char symlink[size];
        readlink(link_path.c_str(), symlink, size);

        std::string sym_link(symlink);

        // Ignore virtual devices
        if (sym_link.find("virtual") != std::string::npos)
            continue;

        struct net_device dev;
        dev.id = file->d_name;
        get_sys_net_mac_addr(dev.id, &dev.mac);
        dev.connected = true;

        devices->push_back(dev);
    }

	closedir(dir);
}

void get_sys_net_mac_addr(const std::string id, std::string *mac) {
	std::string filepath(std::string("/sys/class/net/") + id + "/address");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *mac;
}

void get_sys_block_size(const std::string id, uint64_t *size) {
	std::string filepath(std::string("/sys/block/") + id + "/size");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    // Size is in blocks/sectors which are 512 bytes on Linux
    infile >> *size;

    // Convert to bytes
    *size *= 512;
}

void get_sys_block_device_vendor(std::string id, std::string *vendor) {
	std::string filepath(std::string("/sys/block/") + id + "/device/vendor");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::getline(infile, *vendor);
    int delim_pos;
    // Find and trim leading whitespace
    delim_pos = vendor->find_first_not_of(" ");
    if (delim_pos != -1)
        *vendor = vendor->substr(delim_pos);
    // Find and trim trailing whitespace
    delim_pos = vendor->find_last_not_of(" ");
    if (delim_pos != -1)
        *vendor = vendor->substr(0, delim_pos + 1);
}

void get_sys_block_device_model(std::string id, std::string *model) {
	std::string filepath(std::string("/sys/block/") + id + "/device/model");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::getline(infile, *model);
    int delim_pos;
    // Find and trim leading whitespace
    delim_pos = model->find_first_not_of(" ");
    if (delim_pos != -1)
        *model = model->substr(delim_pos);
    // Find and trim trailing whitespace
    delim_pos = model->find_last_not_of(" ");
    if (delim_pos != -1)
        *model = model->substr(0, delim_pos + 1);
}

void get_sys_block_device_serial(std::string id, std::string *serial) {
	std::string filepath(std::string("/sys/block/") + id + "/device/serial");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *serial;
}

void get_sys_block_device_subsystem(const std::string id, std::string *subsystem) {
	std::string filepath(std::string("/sys/block/") + id + "/device/subsystem");

    constexpr int size = 1024;

	char symlink[size];
    readlink(filepath.c_str(), symlink, size);

    *subsystem = symlink;
    int delim_pos = subsystem->find_last_of("/");
    *subsystem = subsystem->substr(delim_pos + 1);

    // Remove random bits that can sometimes get read
    delim_pos = subsystem->find_last_of("abcdefghijklmonpqrst1234567890");
    if (delim_pos != -1)
        *subsystem = subsystem->substr(0, delim_pos + 1);
}

void get_sys_pci_device_vendor(const std::string id, std::string *vendor) {
	std::string filepath(std::string("/sys/bus/pci/devices/") + id + "/vendor");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *vendor;
    *vendor = convert_pci_id_to_vendor(*vendor);
}

void get_sys_pci_device_driver(const std::string id, std::string *driver) {
	std::string filepath(std::string("/sys/bus/pci/devices/") + id + "/uevent");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *driver;
    *driver = driver->substr(driver->find_first_of("=")+1);
}

void get_sys_dri_clients(const std::string id, std::vector<struct dri_client> *clients) {
	std::string filepath(std::string("/sys/kernel/debug/dri/") + id + "/clients");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    struct dri_client client = {};
    std::string line = "";
    std::string value = "";
    // Skip header row
    std::getline(infile, line);
    int delim_pos;
    while (std::getline(infile, line)) {
        client = {};

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.magic = value.empty()? -1 : std::stoi(value);

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.uid = value.empty()? -1 : std::stoi(value);

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.a = value[0];

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.master = value[0];

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.dev = value.empty()? -1 : std::stoi(value);

        delim_pos = line.find_last_of(" ");
        value = line.substr(delim_pos+1);
        line = line.substr(0, delim_pos);
        delim_pos = line.find_last_not_of(" ");
        line = line.substr(0, delim_pos+1);
        client.pid = value.empty()? -1 : std::stoi(value);

        delim_pos = line.find_first_not_of(" ");
        value = line.substr(delim_pos);
        client.command = value;

        clients->push_back(client);
    }
}

void get_sys_dri_freq(const std::string id, uint32_t *freq) {
	std::string filepath(std::string("/sys/kernel/debug/dri/") + id);
	DIR *dir;
	struct dirent *file;

	dir = opendir(filepath.c_str());
	if(dir == NULL) {
		// perror((std::string("Unable to read directory ") + filepath).c_str());
		return;
	}

    std::string freq_filename = "";
	/* Find *freq* file */
	while((file = readdir(dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
        freq_filename = file->d_name;
        if (freq_filename.find("freq") != std::string::npos)
            break;
        freq_filename = "";
    }

	closedir(dir);

    *freq = 0;
    if (freq_filename == "")
        return;

    filepath = filepath + "/" + freq_filename;
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::string line = "";
    int delim_pos;
    while (std::getline(infile, line)) {
        if (line.find("Current freq") == std::string::npos)
            continue;
        delim_pos = line.find_first_of(":");
        line = line.substr(delim_pos+2);
        delim_pos = line.find_first_of(" ");
        line = line.substr(0, delim_pos);
        break;
    }
    *freq = line.empty()? -1 : std::stoi(line);
}

void get_sys_drm_freq(const std::string id, uint32_t *freq) {
	std::string filepath(std::string("/sys/class/drm/card") + id + "/device/pp_dpm_sclk");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::string line = "";
    int delim_pos;
    while (std::getline(infile, line)) {
        if (line.find("*") == std::string::npos)
            continue;
        delim_pos = line.find_first_of(":");
        line = line.substr(delim_pos+2);
        delim_pos = line.find_first_not_of("1234567890");
        line = line.substr(0, delim_pos);
        break;
    }
    *freq = line.empty()? -1 : std::stoi(line);
}

void get_sys_freq(const std::string id, uint32_t *freq) {
    get_sys_dri_freq(id, freq);
    if (*freq == 0)
        get_sys_drm_freq(id, freq);
}
