#ifndef NET_HPP_
#define NET_HPP_

#include "tab.hpp"
#include "../proc.hpp"
#include "../sys.hpp"

#include <map>

struct net_interface_ext {
        struct net_interface net = {};
        uint64_t last_checked = 0; // in seconds
        _Float32 upload_per_s = 0; // B/s
        _Float32 download_per_s = 0; // B/s
};

struct net_process {
	struct process process = {0};
    std::string user = "";
    std::string type = "";
    std::string node = "";
    std::string name = "";
    std::string connection = "";
};

class NET : public Tab {
public:
    NET();
    ~NET() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    void find_net_devices();
    void find_net_processes();
    void find_net_interfaces();
    void update_net_interfaces();
private:
    std::vector<struct net_process> processes;
    std::vector<struct net_device> devices;
    std::vector<struct net_interface_ext> interfaces;
};

#endif // NET_HPP_