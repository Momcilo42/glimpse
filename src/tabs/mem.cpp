#include "mem.hpp"

#include "../util.hpp"
#include "../id_lists/jedec.hpp"

#include <memory> // unique_ptr
#include <unistd.h> // getpagesize()
#include <sstream> // stringstream

#define COLUMN_1    0                   // pid
#define COLUMN_2    COLUMN_1+8          // name
#define COLUMN_3    COLUMN_2+30         // real/resident
#define COLUMN_4    COLUMN_3+8          // virt
#define COLUMN_5    COLUMN_4+8          // swap
#define COLUMN_6    COLUMN_5+8          // uptime
#define COLUMN_7    COLUMN_6+13         // cmd

uint64_t MEM::get_pid_at_pos() {
    return processes.at(proc_table_pos).process.pid;
}

void read_board_line(const std::string type, const std::string value, struct dmi_mem_board_data *board) {
    const std::string Location__str = "Location";
    const std::string Use__str = "Use";
    const std::string Error_Correction_Type__str = "Error Correction Type";
    const std::string Maximum_Capacity__str = "Maximum Capacity";
    const std::string Error_Information_Handle__str = "Error Information Handle";
    const std::string Number_Of_Devices__str = "Number Of Devices";

    if (type == Location__str) {
        board->Location = value;
    } else if (type == Use__str) {
        board->Use = value;
    } else if (type == Error_Correction_Type__str) {
        board->Error_Correction_Type = value;
    } else if (type == Maximum_Capacity__str) {
        int delim_pos = value.find_first_of(" ");
        if (delim_pos != -1)
            board->Maximum_Capacity = std::stoul(value.substr(0, delim_pos));
    } else if (type == Error_Information_Handle__str) {
        board->Error_Information_Handle = value;
    } else if (type == Number_Of_Devices__str) {
        board->Number_Of_Devices = std::stoul(value);
    }
}

void read_bank_line(const std::string type, const std::string value, struct dmi_mem_device *bank) {
    const std::string Array_Handle__str = "Array Handle";
    const std::string Error_Information_Handle__str = "Error Information Handle";
    const std::string Total_Width__str = "Total Width";
    const std::string Data_Width__str = "Data Width";
    const std::string Size__str = "Size";
    const std::string Form_Factor__str = "Form Factor";
    const std::string Set__str = "Set";
    const std::string Locator__str = "Locator";
    const std::string Bank_Locator__str = "Bank Locator";
    const std::string Type__str = "Type";
    const std::string Type_Detail__str = "Type Detail";
    const std::string Speed__str = "Speed";
    const std::string Manufacturer__str = "Manufacturer";
    const std::string Serial_Number__str = "Serial Number";
    const std::string Asset_Tag__str = "Asset Tag";
    const std::string Part_Number__str = "Part Number";
    const std::string Rank__str = "Rank";
    const std::string Configured_Memory_Speed__str = "Configured Memory Speed";
    const std::string Minimum_Voltage__str = "Minimum Voltage";
    const std::string Maximum_Voltage__str = "Maximum Voltage";
    const std::string Configured_Voltage__str = "Configured Voltage";

    if (type == Array_Handle__str) {
        bank->Array_Handle = value;
    } else if (type == Error_Information_Handle__str) {
        bank->Error_Information_Handle = value;
    } else if (type == Total_Width__str) {
        bank->Total_Width = value;
    } else if (type == Data_Width__str) {
        bank->Data_Width = value;
    } else if (type == Size__str) {
        int delim_pos = value.find_first_of(" ");
        if (delim_pos != -1) {
            if (isdigit(value.at(0)))
                bank->Size = std::stoul(value.substr(0, delim_pos));
            else
                bank->Size = 0;
        }
    } else if (type == Form_Factor__str) {
        bank->Form_Factor = value;
    } else if (type == Set__str) {
        bank->Set = value;
    } else if (type == Locator__str) {
        bank->Locator = value;
    } else if (type == Bank_Locator__str) {
        bank->Bank_Locator = value;
    } else if (type == Type__str) {
        bank->Type = value;
    } else if (type == Type_Detail__str) {
        bank->Type_Detail = value;
    } else if (type == Speed__str) {
        bank->Speed = value;
    } else if (type == Manufacturer__str) {
        bank->Manufacturer = value;
    } else if (type == Serial_Number__str) {
        bank->Serial_Number = value;
    } else if (type == Asset_Tag__str) {
        bank->Asset_Tag = value;
    } else if (type == Part_Number__str) {
        bank->Part_Number = value;
    } else if (type == Rank__str) {
        bank->Rank = value;
    } else if (type == Configured_Memory_Speed__str) {
        bank->Configured_Memory_Speed = value;
    } else if (type == Minimum_Voltage__str) {
        bank->Minimum_Voltage = value;
    } else if (type == Maximum_Voltage__str) {
        bank->Maximum_Voltage = value;
    } else if (type == Configured_Voltage__str) {
        bank->Configured_Voltage = value;
    }

}

void MEM::dmi_decode_mem() {
    const char* cmd = "dmidecode --type memory";
    std::array<char, 128> buffer;
    std::string line;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    struct dmi_mem_board_data board = {};
    struct dmi_mem_device bank = {};
    // First 3 lines are dmidecode info
    // 4th line is a break
    // 5th line starts the Physical Memory Array
    // At the end of each section is an empty line
    // Sections start with handle info followed by the section name in the next line
    int delim_pos;
    std::string type;
    std::string value;
    bool read_board = false;
    bool read_bank = false;
    // Skip the first 3 lines which are dmidecode version data
    fgets(buffer.data(), buffer.size(), pipe.get());
    fgets(buffer.data(), buffer.size(), pipe.get());
    fgets(buffer.data(), buffer.size(), pipe.get());
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        line = buffer.data();
        // Remove new line character
        delim_pos = line.find_first_of("\n");
        if (delim_pos != -1)
            line = line.substr(0, delim_pos);

        if (line.empty()) {
            if (read_board) {
                board_data.push_back(board);
                board = {};
            } else if (read_bank) {
                banks.push_back(bank);
                bank = {};
            }

            read_board = false;
            read_bank = false;

            continue;
        }

        // Ignore Handle rows as they're a header for the actual data
        if (line.find("Handle") != std::string::npos)
            continue;

        if (line.find("Memory Device") != std::string::npos) {
            read_bank = true;
            continue;
        } else if (line.find("Physical Memory Array") != std::string::npos) {
            read_board = true;
            continue;
        }

        // Remove initial tab
        delim_pos = line.find_first_of("\t");
        if (delim_pos != -1)
            line = line.substr(delim_pos+1);

        delim_pos = line.find_first_of(":");
        type = line.substr(0, delim_pos);
        // Account for space after:
        value = line.substr(delim_pos+2);

        if (read_board)
            read_board_line(type, value, &board);
        else if (read_bank)
            read_bank_line(type, value, &bank);
    }
}

void MEM::update_mem_process(struct mem_process *proc) {
    struct pid_stat stats;
    read_pid_stat(proc->process.pid, &stats);
    struct pid_statm statm = {};
    read_pid_statm(proc->process.pid, &statm);
    struct pid_status status = {};
    read_pid_status(proc->process.pid, &status);

    get_pid_uptime(proc->process.pid, &proc->uptime);

    proc->virt = statm.size * page_size;
    proc->real = statm.resident * page_size;
    proc->swap = status.VmSwap.empty() ? 0 : std::stoul(status.VmSwap);

}

void MEM::find_mem_processes() {
	// Reset is_alive for all processes
	for (struct mem_process &p : processes)
			p.process.is_alive = false;

    // Get currently active processes
    std::vector<struct process> proc_vec;
    find_processes(&proc_vec);

    // Check if processes are new or already in vector
    struct process proc;
	for (struct mem_process &memproc : processes) {
        for (int j = 0; j < (int)proc_vec.size(); j++) {
            proc = proc_vec.at(j);
            if (memproc.process.pid == proc.pid) {
                // Found
                memproc.process.is_alive = true;
                std::vector<struct process>::iterator it = (proc_vec.begin() + j);
                proc_vec.erase(it);
                break;
            }
        }
    }

    // All processes that haven't been found and removed should be added to processes vector
	for (struct process &p : proc_vec) {
        struct mem_process cp = {0};
        cp.process = p;
        processes.push_back(cp);
    }

    // Clear dead processes
    erase_from_vector<struct mem_process>(&processes, [](struct mem_process p) {
			return (!p.process.is_alive);
		});

    // Get rest of the fields for memproc
	for (struct mem_process &memproc : processes)
        MEM::update_mem_process(&memproc);
}

MEM::MEM() {
    dmi_decode_mem();
    page_size = getpagesize();

    const int bank_offset = 2;

    // Need to have some time between sampling, if it's too close samples are basically 0
	wtimeout(tab_window, 1000);
    set_info_block_size(bank_offset+banks.size());

    if (board_data[0].Maximum_Capacity > 1024)
        mvwprintw(tab_window, info_block_start, 0, "ECC support: %s Max capacity: %uTiB",
                  board_data[0].Error_Correction_Type.c_str(), board_data[0].Maximum_Capacity / 1024);
    else
        mvwprintw(tab_window, info_block_start, 0, "ECC support: %s Max capacity: %uGB",
                  board_data[0].Error_Correction_Type.c_str(), board_data[0].Maximum_Capacity);
    for (int i = 0; i < (int)banks.size(); i++)
        if (banks.at(i).Size)
            mvwprintw(tab_window, info_block_start+i+bank_offset, 0, "%s: %s %uGB @%s %s %s",
                banks.at(i).Locator.c_str(), convert_id_to_vendor(banks.at(i).Manufacturer).c_str(), banks.at(i).Size,
                banks.at(i).Speed.c_str(), banks.at(i).Total_Width == banks.at(i).Data_Width ? "" : "ECC",
                banks.at(i).Part_Number.c_str());
        else
            mvwprintw(tab_window, info_block_start+i+bank_offset, 0, "%s: Not installed", banks.at(i).Locator.c_str());

    update();
}

void MEM::update() {
    find_mem_processes();
    process_vector_size = processes.size();
    sort_vector<struct mem_process>(&processes,
        [](const struct mem_process p1, const struct mem_process p2) {
            return p1.real > p2.real;
        });

    read_meminfo(&info);
    mvwprintw(tab_window, info_block_start+1, 0, "Usage: %u/%u MB\tSwap: %u/%u MB",
        (info.MemTotal-info.MemAvailable) / 1000, info.MemTotal / 1000, (info.SwapTotal - info.SwapFree) / 1000, info.SwapTotal/1000);

    // Proc
    uint32_t pos = proc_table_top;
    struct mem_process *proc;
    for (uint32_t i = 0; i <= proc_block_size; i++) {
        proc = &processes.at(pos);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_1, "%lu", proc->process.pid);
        /* Clear extra characters if previous value was longer */
		wclrtoeol(tab_window);
        mvwprintw(tab_window, proc_block_start+i, COLUMN_2, "%s", proc->process.name.c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_3, "%s", format_size(proc->real).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_4, "%s", format_size(proc->virt).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_5, "%s", format_size(proc->swap).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_6, "%s", format_time(proc->uptime).c_str());
        mvwprintw(tab_window, proc_block_start+i, COLUMN_7, "%s", proc->process.cmd.c_str());
        pos++;
        if (pos >= processes.size())
            break;
    }
    /* Invert the highlight of the currently selected process */
    mvwchgat(tab_window, proc_block_start+proc_table_pos, 0, -1, A_REVERSE, 0, NULL);
}