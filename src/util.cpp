#include "util.hpp"

#include <signal.h>
#include <iomanip> // setprecision
#include <chrono>
#include <iostream>

extern "C" {
	#include <unistd.h> // getuid()
	#include <getopt.h> // longopts, shortopts
}

bool quit = false;
bool lock = false;

void sig_handler(int signo) {
	std::cout << "Signal " << signo << " detected. Quitting" << std::endl;
	quit = true;
}

void setup_signal_handler() {
	struct sigaction act;
	act.sa_handler = sig_handler;
	act.sa_flags = SA_NODEFER | SA_SIGINFO;
	/* Signals blocked during the execution of the handler. */
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGTERM);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}

void check_root() {
	if (getuid()) {
		std::cout << "This program needs root priviledges to run" << std::endl;
		exit(1);
	}
}

void help(const std::string prog) {
	std::cout << "glimpse [" << prog << "] version: " << VERSION << std::endl;
	std::cout << "[-h\t--help]\t\tPrint this message" << std::endl;
	std::cout << "[-v\t--version]\tPrint version" << std::endl;
}

void read_args(int argc, char *argv[]) {
	static const char *shortopts = "hv";
	static const struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'}
	};
	std::string prog = "Unknown prog name";
	if (argc >= 1) {
        std::string path(argv[0]);
		prog = path.substr(path.find_last_of("/\\") + 1);
    }

	int character;
	while ((character = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (character) {
		case 'h':
			help(prog);
			exit(0);
		case 'v':
			std::cout << prog << " version: " << VERSION << std::endl;
			exit(0);
		default:
			return;
		}
	}
}

std::string format_time(uint64_t time_s) {
	if (!time_s)
		return "00:00:00";

    int seconds = time_s % 60;
    int minutes = (time_s / 60) % 60;
    int hours = (time_s / 3600) % 24;
    int days = (time_s / 86400);

    std::string s(std::to_string(seconds));
    std::string m(std::to_string(minutes));
    std::string h(std::to_string(hours));
    std::string d(std::to_string(days));

    if (seconds < 10)
        s = "0" + s;
    if (minutes < 10)
        m = "0" + m;
    if (hours < 10)
        h = "0" + h;

    if (days)
        return std::string(d + "d:" + h + ":" + m + ":" + s);
    else
        return std::string(h + ":" + m + ":" + s);
}

std::string format_size(const uint64_t size) {
	char order[] = {'K', 'M', 'G', 'T', 'P', 'E'};
	double new_size = size;
	int i = -1;
	while (new_size >= 1000) {
		i++;
		new_size /= 1000;
	}

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << new_size;
    std::string ret = stream.str();

	if (i >= 0)
		ret += order[i];

	return ret;
}

std::string center_string(std::string str, uint32_t width) {
	std::string ret = "";
	uint32_t str_width = str.size();

	if (width < str_width) {
		ret = str.substr(0, width);
		return "";
	}

	for (uint32_t i = 0; i < (width - str_width) / 2; i++)
		ret += " ";
	ret += str;

	return ret;
}

std::string get_current_time_str() {
    time_t tt = std::time(0);   // get time now
    char buf[100] = {0};
    std::strftime(buf, sizeof(buf), "%Y_%b_%d - %H:%M:%S", std::localtime(&tt));
    return std::string(buf);
}