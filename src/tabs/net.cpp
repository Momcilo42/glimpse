#include "net.hpp"

#include "../util.hpp"

#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory> // unique_ptr
#include <array>    // std::array

#define COLUMN_1    0                   // pid
#define COLUMN_2    COLUMN_1+8          // name
#define COLUMN_3    COLUMN_2+25         // user
#define COLUMN_4    COLUMN_3+15         // type
#define COLUMN_5    COLUMN_4+6          // node
#define COLUMN_6    COLUMN_5+5          // connection
#define COLUMN_7    COLUMN_6+13          // name

uint64_t NET::get_pid_at_pos() {
    return processes.at(proc_table_pos).process.pid;
}

// TODO replace with IOCTL
void get_ssid(const std::string interface_name, std::string *ssid) {
    std::string cmd = std::string("iwgetid") + " " + interface_name + " -r";
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        *ssid = buffer.data();
}

void NET::find_net_devices() {
    std::vector<struct net_device> new_devices;
    get_non_virtual_network_devices(&new_devices);

    // Check if devices are still or just connected
    struct net_device dev;
	for (struct net_device &dd : devices) {
        dd.connected = false;
        for (int j = 0; j < (int)new_devices.size(); j++) {
            dev = new_devices.at(j);
            if (dd.id == dev.id) {
                // Found
                dd.connected = true;
                std::vector<struct net_device>::iterator it = (new_devices.begin() + j);
                new_devices.erase(it);
                break;
            }
        }
    }
	for (struct net_device &dd : new_devices) {
        devices.push_back(dd);
    }

    // Clear disconnected disks
    erase_from_vector<struct net_device>(&devices, [](struct net_device dev) {
			return (!dev.connected);
		});

    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

	for (struct net_device &dev : devices) {
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL)
                continue;
            if(dev.id != ifa->ifa_name)
                continue;

            // Only check internet networks
            if ((ifa->ifa_addr->sa_family != AF_INET) &&
                (ifa->ifa_addr->sa_family != AF_INET6))
                continue;

            if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
                struct sockaddr_in *addr = (struct sockaddr_in *)(ifa->ifa_addr);
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
                dev.v4.address = std::string(ip);

                char netmask[INET_ADDRSTRLEN];
                struct sockaddr_in *mask = (struct sockaddr_in *)(ifa->ifa_netmask);
                inet_ntop(AF_INET, &(mask->sin_addr), netmask, INET_ADDRSTRLEN);
                dev.v4.netmask = std::string(netmask);

                char network[INET_ADDRSTRLEN];
                struct in_addr network_addr;
                network_addr.s_addr = addr->sin_addr.s_addr & mask->sin_addr.s_addr;
                inet_ntop(AF_INET, &network_addr, network, INET_ADDRSTRLEN);
                dev.v4.network = std::string(network);
            } else if (ifa->ifa_addr->sa_family == AF_INET6) { // IPv6
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *)(ifa->ifa_addr);
                char ip[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(addr->sin6_addr), ip, INET6_ADDRSTRLEN);
                dev.v6.address = std::string(ip);

                char netmask[INET6_ADDRSTRLEN];
                struct sockaddr_in6 *mask = (struct sockaddr_in6 *)(ifa->ifa_netmask);
                inet_ntop(AF_INET, &(mask->sin6_addr), netmask, INET6_ADDRSTRLEN);
                dev.v6.netmask = std::string(netmask);

                // Calculate network prefix
                struct in6_addr network_prefix;
                memset(&network_prefix, 0, sizeof(network_prefix));
                // Calculate network prefix length
                int prefix_length = 0;
                for (int i = 0; i < 8; ++i) {
                    if (mask->sin6_addr.s6_addr[i] == 0xFF) {
                        prefix_length += 8;
                    } else {
                        uint8_t byte = mask->sin6_addr.s6_addr[i];
                        int j = 7;
                        while (byte & (1 << j)) {
                            prefix_length++;
                            j--;
                        }
                        break;
                    }
                }
                // Calculate network prefix
                for (int i = 0; i < 16; ++i) {
                    network_prefix.s6_addr[i] = addr->sin6_addr.s6_addr[i] & mask->sin6_addr.s6_addr[i];
                }

                // Convert network prefix back to string
                char network[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &network_prefix, network, INET6_ADDRSTRLEN);
                dev.v6.network = std::string(network);
            }
        }
        get_ssid(dev.id, &dev.ssid);
    }

    freeifaddrs(ifaddr);
}

void NET::find_net_interfaces() {
    std::vector<struct net_interface> net_vec;
    read_net_dev(&net_vec);

	for (struct net_interface &pn : net_vec) {
        struct net_interface_ext pne = {};
        pne.net = pn;
        get_uptime(&pne.last_checked);
        pne.download_per_s = 0;
        pne.upload_per_s = 0;
        interfaces.push_back(pne);
    }
}

void NET::update_net_interfaces() {
    std::vector<struct net_interface> net_vec;
    read_net_dev(&net_vec);

    std::vector<struct net_interface_ext> net_ifs = {};
    net_ifs.clear();
	for (struct net_interface &pn : net_vec) {
        struct net_interface_ext pne = {};
        pne.net = pn;
        get_uptime(&pne.last_checked);
        pne.download_per_s = 0;
        pne.upload_per_s = 0;
        net_ifs.push_back(pne);
    }

    uint64_t old_last_checked;
    uint64_t new_last_checked;
    uint64_t time_delta;
    double up = 0;
    double down = 0;
    for (uint32_t i = 0; i < interfaces.size(); i++) {
        old_last_checked = interfaces.at(i).last_checked;
        new_last_checked = net_ifs.at(i).last_checked;

        up = 0;
        down = 0;
        time_delta = new_last_checked - old_last_checked;
        if (time_delta) {
            up = (net_ifs.at(i).net.tx_bytes - interfaces.at(i).net.tx_bytes)*1.0 / time_delta;
            down = (net_ifs.at(i).net.rx_bytes - interfaces.at(i).net.rx_bytes)*1.0 / time_delta;
        } else {
            up = 0;
            down = 0;
        }
        interfaces.at(i).upload_per_s = up;
        interfaces.at(i).download_per_s = down;

        interfaces.at(i).net = net_ifs.at(i).net;
    }
}

void find_processes_with_connections(std::vector<struct net_process> *proc_vec) {
    const char* cmd = "lsof -i";
    std::array<char, 256> buffer;
    std::string line;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    int delim_pos;
    std::string type;
    std::string value;
    struct net_process proc = {};
    // Skip the header
    // COMMAND       PID            USER   FD   TYPE   DEVICE SIZE/OFF NODE NAME
    fgets(buffer.data(), buffer.size(), pipe.get());
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        line = buffer.data();
        proc = {};
        proc.process.is_alive = true;

        // COMMAND
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);

        // PID
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);
        proc.process.pid = value.empty()? -1 : std::stoi(value);
		get_process_name(proc.process.pid, &proc.process.name);
		get_calling_command(proc.process.pid, &proc.process.cmd);

        // USER
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);
        proc.user = value;

        // FD
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);

        // TYPE
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);
        proc.type = value;

        // DEVICE
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);

        // SIZE/OFF
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);

        // NODE
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        line = line.substr(delim_pos);
        proc.node = value;

        // NAME
        delim_pos = line.find_first_of(" ");
        value = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" ");
        if (delim_pos != -1)
            line = line.substr(delim_pos);
        proc.name = value;

        delim_pos = line.find_first_of("(");
        if (delim_pos != -1) {
            proc.connection = line.substr(delim_pos+1);
            proc.connection = proc.connection.substr(0, proc.connection.find_first_of(")"));
        }

        proc_vec->push_back(proc);
    }
}

void NET::find_net_processes() {
	// Reset is_alive for all processes
	for (struct net_process &p : processes)
			p.process.is_alive = false;

    // Get currently active processes
    std::vector<struct net_process> proc_vec;
    find_processes_with_connections(&proc_vec);

    // Check if processes are new or already in vector
	for (struct net_process &netproc : processes) {
        for (int j = 0; j < (int)proc_vec.size(); j++) {
            if (netproc.process.pid == proc_vec.at(j).process.pid) {
                // Found
                netproc.process.is_alive = true;
                proc_vec.erase(proc_vec.begin() + j);
                break;
            }
        }
    }

    // All processes that haven't been found and removed should be added to processes vector
	for (struct net_process &p : proc_vec)
        processes.push_back(p);

    // Clear dead processes
    erase_from_vector<struct net_process>(&processes, [](struct net_process p) {
			return (!p.process.is_alive);
		});
}

NET::NET() {
    find_net_interfaces();

    // Need to have some time between sampling, if it's too close samples are basically 0
	wtimeout(tab_window, 1000);

    update();
};

void NET::update() {
    // Info
    find_net_devices();
	update_net_interfaces();

    uint32_t old_size = processes.size();
    find_net_processes();
    if (old_size != processes.size())
        sort_vector<struct net_process>(&processes,
            [](const struct net_process p1, const struct net_process p2) {
                int val1 = 0;
                if (p1.connection == "ESTABLISHED")
                    val1 = 2;
                else if (p1.connection == "LISTEN")
                    val1 = 1;

                int val2 = 0;
                if (p2.connection == "ESTABLISHED")
                    val2 = 2;
                else if (p2.connection == "LISTEN")
                    val2 = 1;

                return val1 > val2;
            });

    // If a network device was removed extend proc block up
    uint32_t previous_size = process_vector_size;
    process_vector_size = processes.size();
    if (process_vector_size != previous_size)
        for (uint32_t i = 0; i < process_vector_size; i++) {
            // Move cursor to beginning of row
            mvcur(getcurx(tab_window), getcury(tab_window), info_block_start+i, 0);
            // Erase row
		    wclrtoeol(tab_window);
        }

    int info_offset = 0;
    for (int i = 0; i < (int)devices.size(); i++) {
        mvwprintw(tab_window, info_block_start+info_offset, 0, "%s (%s)",
            devices.at(i).id.c_str(), devices.at(i).mac.c_str());
        if (((devices.at(i).v4.netmask == "255.255.255.255") || (devices.at(i).v4.netmask == "")) &&
           ((devices.at(i).v6.netmask == "255.255.255.255") || (devices.at(i).v6.netmask == ""))) {
            mvwprintw(tab_window, info_block_start+info_offset+1, 10, "not connected");
            devices.at(i).connected = false;
            info_offset += 2;
            continue;
        }
        info_offset += 1;

        if (!devices.at(i).ssid.empty()) {
            mvwprintw(tab_window, info_block_start+info_offset, 35, "net: %s", devices.at(i).ssid.c_str());
        }

	    for (struct net_interface_ext &net_if : interfaces) {
            if (net_if.net.interface != devices.at(i).id)
                continue;
            mvwprintw(tab_window, info_block_start+info_offset +1, 35, "Up:   %s    ",
                format_size(net_if.upload_per_s).c_str());
            mvwprintw(tab_window, info_block_start+info_offset +2, 35, "Down: %s    ",
                format_size(net_if.download_per_s).c_str());
        }

        if (devices.at(i).v4.netmask != "255.255.255.255") {
            mvwprintw(tab_window, info_block_start+info_offset, 10, "ipv4: %s", devices.at(i).v4.address.c_str());
            mvwprintw(tab_window, info_block_start+info_offset+1, 10, "      %s", devices.at(i).v4.network.c_str());
            mvwprintw(tab_window, info_block_start+info_offset+2, 10, "      %s", devices.at(i).v4.netmask.c_str());
            info_offset += 3;
        }
        if (devices.at(i).v6.netmask != "255.255.255.255") {
            mvwprintw(tab_window, info_block_start+info_offset, 10, "ipv6: %s", devices.at(i).v6.address.c_str());
            mvwprintw(tab_window, info_block_start+info_offset+1, 10, "      %s", devices.at(i).v6.network.c_str());
            mvwprintw(tab_window, info_block_start+info_offset+2, 10, "      %s", devices.at(i).v6.netmask.c_str());
            info_offset += 3;
        }
    }
    set_info_block_size(info_offset);

    // Proc
    uint32_t pos = proc_table_top;
    struct net_process *proc;
    for (uint32_t i = 0; i <= proc_block_size; i++) {
        proc = &processes.at(pos);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_1, "%lu", proc->process.pid);
        /* Clear extra characters if previous value was longer */
		wclrtoeol(tab_window);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_2, "%s", proc->process.name.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_3, "%s", proc->user.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", proc->type.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_5, "%s", proc->node.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_6, "%s", proc->connection.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_7, "%s", proc->name.c_str());
        pos++;
        if (pos >= processes.size())
            break;
    }
    /* Invert the highlight of the currently selected process */
    mvwchgat(tab_window, proc_block_start+proc_table_pos, 0, -1, A_REVERSE, 0, NULL);
}
