#include "overview.hpp"

#include "../sys.hpp"
#include "../util.hpp"
#include "../proc.hpp"

#include <array>
#include <memory> // unique_ptr
#include <sstream>

#define COLUMN_1    0
#define COLUMN_2    COLUMN_1+10
#define COLUMN_3    COLUMN_2+10
#define COLUMN_4    COLUMN_3+10
#define COLUMN_5    COLUMN_4+18

uint64_t Overview::get_pid_at_pos() {
    return 0;
}

struct monitor {
	std::string model = "";
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

void get_monitors(std::vector<struct monitor> *monitors) {
    std::string cmd = "get-edid -q 2>/dev/null | parse-edid 2>/dev/null";
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

	struct monitor m = {};

	bool reading_monitor = false;
	std::vector<std::string> modes;
	std::string preferred_mode = "";
	int delim_pos = -1;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        line = buffer.data();
        // Remove new line character
        delim_pos = line.find_first_of("\n");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);

		if (line == "Section \"Monitor\"") {
			reading_monitor = true;
			continue;
		}

		if ((line == "EndSection") && reading_monitor) {
			for (int i = 0; i < (int)modes.size(); i++)
       			if (modes[i].find(preferred_mode) != std::string::npos) {
					line = modes[i].substr(modes[i].find_last_of("\"")+2);
					break;
				}

			std::istringstream iss(line);
			std::string s;
			std::vector<int> numbers = {};
			while (std::getline(iss, s, ' '))
				numbers.push_back(std::stoi(s));
			// Remove clock
			numbers.erase(numbers.begin());

			m.width = numbers[0];
			m.height = numbers[numbers.size()/2];

			monitors->push_back(m);
			m = {};
			continue;
		}

        if (line.find("ModelName") != std::string::npos) {
			line = line.substr(0, line.find_last_of("\""));
			line = line.substr(line.find_last_of("\"")+1);
			m.model = line;
			continue;
		}

        if (line.find("PreferredMode") != std::string::npos) {
			line = line.substr(0, line.find_last_of("\""));
			line = line.substr(line.find_last_of("\"")+1);
			preferred_mode = line;
			continue;
		}

		// Modeline is in format:
		// Modeline  "(Label)" (clk)     (x-resolutions)        (y-resolutions)
        if (line.find("Modeline") != std::string::npos) {
			line = line.substr(0, line.find_last_of("1234567890"));
			line = line.substr(line.find_last_of("\t")+1);
			modes.push_back(line);
		}
	}
}

void get_active_users(std::vector<struct active_user> *users) {
    std::string cmd = "who -u";
    std::array<char, 128> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
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
		mvwprintw(tab_window, info_block_start + row++, 0, "Monitor: %s @%dx%d",
			monitors[i].model.c_str(), monitors[i].width, monitors[i].height);

	set_info_block_size(row);
	update();
}

void Overview::update() {
	int row = 1;

	mvwprintw(tab_window, proc_block_start + row++, 0, "Current server time is: %s",
		get_current_time().c_str());
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