#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <iostream>
#include <vector>
#include <algorithm> // std::sort
#include <chrono>

#define VERSION			"0.1beta"

extern bool lock;

void setup_signal_handler();
void check_root();
void help(const std::string prog);
void read_args(int argc, char *argv[]);
std::string format_time(uint64_t time_s);
std::string format_size(uint64_t size);
std::string center_string(const std::string str, const uint32_t width);
std::string get_current_time(time_t *time = nullptr);

template<typename T>
void sort_vector(std::vector<T> *vec, bool (*sort_criteria)(T a, T b)) {
	if (lock)
		return;
    std::sort(vec->begin(), vec->end(), sort_criteria);
}

template<typename T>
void erase_from_vector(std::vector<T> *vec, bool (*erase_criteria)(T a)) {
	vec->erase(std::remove_if(vec->begin(), vec->end(),
		erase_criteria), vec->end());
}

#endif // UTIL_HPP_