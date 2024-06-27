#include "overview.hpp"

#include "../sys.hpp"
#include "../util.hpp"
#include "../proc.hpp"

#include <array>
#include <memory> // unique_ptr
#include <sstream>
#include <iostream> // std::ifstream
#include <fstream>	// std::ifstream

#define COLUMN_1    0
#define COLUMN_2    COLUMN_1+10
#define COLUMN_3    COLUMN_2+10
#define COLUMN_4    COLUMN_3+10
#define COLUMN_5    COLUMN_4+18

uint64_t Overview::get_pid_at_pos() {
    return 0;
}

struct monitor {
	std::string manufacturer = "";
	std::string model = "";
	bool built_in = false;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct active_user {
	std::string user = "";
	std::string tty = "";
	std::string date = "";
	std::string time = "";
	// ‘.’  	means the user was active in the last minute.
	// ‘old’	means the user has been idle for more than 24 hours.
	std::string last_active = "";
	uint64_t active_connection_pid = 0;
	std::string connection = "";
};

bool is_file_empty(std::string path) {
	bool ret = false;
	std::ifstream pFile;
	pFile.open(path);
	ret = pFile.peek() == std::ifstream::traits_type::eof();
	pFile.close();
	return ret;
}

void get_active_display_ports(std::vector<std::string> *active_ports) {
	// Find all edid files
	// TODO maybe change to manual search like in get_non_virtual_block_devices not to rely on find
	std::string cmd = "find -L /sys/class/drm/ -maxdepth 2 -name edid 2>/dev/null";
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

	int delim_pos = -1;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		// Get line
        line = buffer.data();
        // Remove new line character
        delim_pos = line.find_first_of("\n");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);
		if (!is_file_empty(line))
			active_ports->push_back(line);
	}
}

void check_pnp_ids(std::string *id) {
    std::ifstream infile("/usr/share/hwdata/pnp.ids");
    if (!infile.is_open())
		return;

	std::string line;
	int delim_pos = -1;
	std::string manufacturer_id;
	std::string manufacturer_name;
	// Read data from the file object and put it into a string.
	while (getline(infile, line)) {
        delim_pos = line.find_first_of("\t");
        if (delim_pos != -1) {
            manufacturer_id = line.substr(0, delim_pos);
            manufacturer_name = line.substr(delim_pos+1);
		}
		if (manufacturer_id.compare(*id) == 0) {
			*id = manufacturer_name;
			break;
		}
	}
}

void get_monitor_data_name(std::string *path, struct monitor *m) {
	std::string cmd = "edid-decode --skip-hex-dump --skip-sha " + *path;
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

	int delim_pos = -1;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		// Get line
        line = buffer.data();
        // Remove new line character
        delim_pos = line.find_first_of("\n");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);

		std::string manufacturer_str = "Manufacturer: ";
		delim_pos = line.find(manufacturer_str);
        if (delim_pos == -1)
            continue;

		line = line.substr(delim_pos + manufacturer_str.length());
		m->manufacturer = line;

		check_pnp_ids(&m->manufacturer);
		break;
	}

	// TODOM find model
}

void get_monitor_data_native_res(std::string *path, struct monitor *m) {
	std::string cmd = "edid-decode -n --skip-hex-dump --skip-sha " + *path + " | tail -n 1";
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

	int delim_pos = -1;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		// Get line
        line = buffer.data();
        // Remove new line character
        delim_pos = line.find_first_of("\n");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);

        // Remove empty space before
        delim_pos = line.find_first_not_of(" ");
        if (delim_pos != -1)
            line = line.substr(delim_pos);
        // Remove empty space after
        delim_pos = line.find_first_of(" ");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);

		// Separate width and height
        delim_pos = line.find_first_of("x");
        if ((delim_pos == -1) || line.empty())
			break;

        // Width
		m->width = std::atoi(line.substr(0, delim_pos).c_str());
        // Height
		m->height = std::atoi(line.substr(delim_pos+1).c_str());

		break;
	}
}

void is_monitor_built_in(std::string *path, struct monitor *m) {
	const std::string built_in_identifiers[] = { "eDP", "LVDS" };

	for (std::string identifier : built_in_identifiers) {
		if (path->find(identifier) != std::string::npos) {
			m->built_in = true;
			break;
		}
	}
}

void get_monitor_data(std::string *path, struct monitor *m) {
	get_monitor_data_name(path, m);
	is_monitor_built_in(path, m);
	get_monitor_data_native_res(path, m);
}

void get_monitors(std::vector<struct monitor> *monitors) {
	std::vector<std::string> active_ports;
	get_active_display_ports(&active_ports);

	for (auto path : active_ports) {
		struct monitor m;
		get_monitor_data(&path, &m);
		monitors->push_back(m);
	}
}

void get_active_users(std::vector<struct active_user> *users) {
    std::string cmd = "who -u";
    std::array<char, 128> buffer;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

	struct active_user user = {};
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		std::istringstream iss(buffer.data());
		user = {};

		iss >> user.user >> user.tty >> user.date >> user.time
			>> user.last_active >> user.active_connection_pid >> user.connection;

		if (user.connection != "")
			user.connection = user.connection.substr(1, user.connection.size()-2);
		else
			user.connection = "Direct";
		users->push_back(user);
	}

}

Overview::Overview() {
	find_processes(&processes);
	process_vector_size = processes.size();

	mvwprintw(tab_window, info_block_start, 0, "Loading");
	wrefresh(tab_window);

	int row = 0;

	std::string product_name;
	get_product_name(&product_name);
	std::string kernel_release;
	get_kernel_release(&kernel_release);
	std::string os_release;
	get_os_release(&os_release);
	mvwprintw(tab_window, info_block_start + row++, 0, "Product: %s", product_name.c_str());
	mvwprintw(tab_window, info_block_start + row++, 0, "Kernel: %s", kernel_release.c_str());
	mvwprintw(tab_window, info_block_start + row++, 0, "OS: %s", os_release.c_str());

	std::vector<struct monitor> monitors = {};
	get_monitors(&monitors);
	for (int i = 0; i < (int)monitors.size(); i++)
		mvwprintw(tab_window, info_block_start + row++, 0, "Monitor: %s %s @%dx%d %s",
			monitors[i].manufacturer.c_str(), monitors[i].model.c_str(),
			monitors[i].width, monitors[i].height,
			monitors[i].built_in ? "built-in" : "");

	set_info_block_size(row);
	update();
}

void Overview::update() {
	int row = 1;

	mvwprintw(tab_window, proc_block_start + row++, 0, "Current server time is: %s",
		get_current_time_str().c_str());
    uint64_t system_uptime;
    get_uptime(&system_uptime);
    mvwprintw(tab_window, proc_block_start + row++, 0, "Uptime: %s\n", format_time(system_uptime).c_str());

	row++;
	std::vector<struct active_user> users = {};
	get_active_users(&users);
	for (int i = 0; i < (int)users.size(); i++) {
		mvwprintw(tab_window, proc_block_start + row, COLUMN_1, "%s", users[i].user.c_str());
		mvwprintw(tab_window, proc_block_start + row, COLUMN_2, "%s", users[i].tty.c_str());
		mvwprintw(tab_window, proc_block_start + row, COLUMN_3, "%s", users[i].last_active.c_str());
		mvwprintw(tab_window, proc_block_start + row, COLUMN_4, "%s", users[i].connection.c_str());
		std::string command = "";
		get_process_name(users[i].active_connection_pid, &command);
		mvwprintw(tab_window, proc_block_start + row, COLUMN_5, "%s", command.c_str());
		row++;
	}
	for (int i = row; i < (int)proc_block_size; i++) {
		mvwprintw(tab_window, proc_block_start+i, 0, " ");
		wclrtoeol(tab_window);
	}
}