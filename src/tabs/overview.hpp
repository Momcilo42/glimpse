#ifndef OVERVIEW_HPP_
#define OVERVIEW_HPP_

#include "tab.hpp"
#include "../proc.hpp"
#include <sys/utsname.h>

class Overview : public Tab {
public:
    Overview();
    ~Overview() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    std::vector<struct process> processes;
    struct utsname osInfo;
};

#endif // OVERVIEW_HPP_