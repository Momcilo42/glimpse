#ifndef SYS_HPP_
#define SYS_HPP_

#include <iostream>
#include <vector>

struct disk_device {
    std::string id = "";
    std::string vendor = "";
    std::string model = "";
    std::string serial = "";
    std::string subsystem = "";
    uint64_t size = 0; // in B
    bool connected = false;
};

struct ip_data {
    std::string address = "";
    std::string network = "";
    std::string netmask = "";
};

struct net_device {
    std::string id = "";
    struct ip_data v4 = {};
    struct ip_data v6 = {};
    std::string mac = "";
    std::string ssid = "";
    std::string dev_type = ""; // TODO read from /sys/class/net/wlp0s20f3/uevent
    bool connected = false;
    // get speed (1000/100/10)
    //  cat /sys/class/net/eno1/speed
};

struct dri_client {
    std::string command = "";
    uint64_t pid = 0;
    uint32_t dev = 0;
    char master;
    char a;
    uint64_t uid = 0;
    uint64_t magic = 0;
};

// Get data from /etc/os-release
void get_os_release(std::string *release);
void get_kernel_release(std::string *release);

// Get data from /sys/devices/virtual/dmi/id/product_name
void get_product_name(std::string *name);
// Get data from /sys/block
void get_non_virtual_block_devices(std::vector<struct disk_device> *devices);
// Get data from /sys/class/net
void get_non_virtual_network_devices(std::vector<struct net_device> *devices);
// Get data from /sys/class/net/{id}/address
void get_sys_net_mac_addr(const std::string id, std::string *mac);
// Get data from /sys/block/{device}/size
void get_sys_block_size(const std::string id, uint64_t *size);
// Get data from /sys/block/{device}/device/vendor
void get_sys_block_device_vendor(const std::string id, std::string *vendor);
// Get data from /sys/block/{device}/device/model
void get_sys_block_device_model(const std::string id, std::string *model);
// Get data from /sys/block/{device}/device/serial
void get_sys_block_device_serial(const std::string id, std::string *serial);
// Get data from /sys/block/{device}/device/subsystem
void get_sys_block_device_subsystem(const std::string id, std::string *subsystem);
// Get data from /sys/bus/pci/devices/{device}/vendor
void get_sys_pci_device_vendor(const std::string id, std::string *vendor);
// Get data from /sys/bus/pci/devices/{device}/uevent
void get_sys_pci_device_driver(const std::string id, std::string *driver);
// Get data from /sys/kernel/debug/dri/{id}/clients
void get_sys_dri_clients(const std::string id, std::vector<struct dri_client> *clients);
// Get data from /sys/kernel/debug/dri/{id}/i915_frequency_info
void get_sys_dri_freq(const std::string id, uint32_t *freq);
// Get data from /sys/class/drm/card{id}/device/pp_dpm_sclk
void get_sys_drm_freq(const std::string id, uint32_t *freq);
// Wrapper for two funcs above
void get_sys_freq(const std::string id, uint32_t *freq);

#endif // SYS_HPP