#ifndef MEM_HPP_
#define MEM_HPP_

#include "tab.hpp"
#include "../proc.hpp"

struct mem_process {
	struct process      process = {0};
    uint64_t            virt = 0; // in B
    uint64_t            real = 0; // in B
	uint64_t			swap = 0; // in kB
    uint64_t            uptime = 0; // in seconds
};

struct dmi_mem_board_data {
	std::string Location = "";
	std::string Use = "";
	std::string Error_Correction_Type = "";
	uint32_t Maximum_Capacity = 0; // in GB
	std::string Error_Information_Handle = "";
	uint8_t Number_Of_Devices = 0;
};
struct dmi_mem_device {
    std::string Array_Handle = "";
	std::string Error_Information_Handle = "";
	std::string Total_Width = "";
	std::string Data_Width = "";
	uint32_t Size = 0; // in GB
	std::string Form_Factor = "";
	std::string Set = "";
	std::string Locator = "";
	std::string Bank_Locator = "";
	std::string Type = "";
	std::string Type_Detail = "";
	std::string Speed = "";
	std::string Manufacturer = "";
	std::string Serial_Number = "";
	std::string Asset_Tag = "";
	std::string Part_Number = "";
	std::string Rank = "";
	std::string Configured_Memory_Speed = "";
	std::string Minimum_Voltage = "";
	std::string Maximum_Voltage = "";
	std::string Configured_Voltage = "";
};

class MEM : public Tab {
public:
    MEM();
    ~MEM() {};

    void update() override;
    uint64_t get_pid_at_pos() override;
private:
    void dmi_decode_mem();
    void update_mem_process(struct mem_process *proc);
    void find_mem_processes();
private:
    std::vector<struct mem_process> processes;
    std::vector<struct dmi_mem_board_data> board_data;
    std::vector<struct dmi_mem_device> banks;
    struct meminfo info;
    uint32_t page_size;
};

#endif // MEM_HPP_