#include "proc.hpp"

#include <iomanip>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <fstream>

extern "C" {
	#include <errno.h> // errno
	#include <stdlib.h> // strtol()
	#include <dirent.h> // DIR, struct dirent, opendir()
}

void find_processes(std::vector<struct process> *processes) {
	DIR *proc_dir;
	struct dirent *file;

	/* Get list of processes from render node */
	std::vector<uint64_t> pids;
	pids.reserve(150);

	proc_dir = opendir("/proc");
	if(proc_dir == NULL) {
		// perror("Unable to read directory proc");
		return;
	}

	/* Read the contents of the directory */
	while((file = readdir(proc_dir))) {
		/* Ignore self links and hidden files */
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		int pid = 0;
		// Check if name is a pid number
    	if ((pid = atoi(file->d_name)) == 0)
			continue;

		process p;
		p.pid = pid;
		get_process_name(pid, &p.name);
		get_calling_command(pid, &p.cmd);
		p.is_alive = true;
		processes->push_back(p);
	}

	closedir(proc_dir);
}

void get_calling_command(const int32_t pid, std::string *cmd) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/cmdline");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *cmd;
}

void get_process_name(const int32_t pid, std::string *name) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/comm");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    infile >> *name;
}

void read_cpuinfo(std::vector<struct cpuinfo_core> *info) {
	std::string filepath("/proc/cpuinfo");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	const std::string processor__str = "processor";
	const std::string vendor_id__str = "vendor_id";
	const std::string cpu_family__str = "cpu family";
	const std::string model__str = "model";
	const std::string model_name__str = "model name";
	const std::string stepping__str = "stepping";
	const std::string microcode__str = "microcode";
	const std::string cpu_MHz__str = "cpu MHz";
	const std::string cache_size__str = "cache size";
	const std::string physical_id__str = "physical id";
	const std::string siblings__str = "siblings";
	const std::string core_id__str = "core id";
	const std::string cpu_cores__str = "cpu cores";
	const std::string apicid__str = "apicid";
	const std::string initial_apicid__str = "initial apicid";
	const std::string fpu__str = "fpu";
	const std::string fpu_exception__str = "fpu_exception";
	const std::string cpuid_level__str = "cpuid level";
	const std::string wp__str = "wp";
	const std::string flags__str = "flags";
	const std::string bugs__str = "bugs";
	const std::string bogomips__str = "bogomips";
	const std::string clflush_size__str = "clflush size";
	const std::string cache_alignment__str = "cache_alignment";
	const std::string address_sizes__str = "address sizes";
	const std::string power_management__str = "power management";

    struct cpuinfo_core core;

    std::string line;
    std::string type;
    std::string value;
    int delim_pos;
    while (std::getline(infile, line)) {
        if (line.empty()) {
			info->push_back(core);
            continue;
		}
        delim_pos = line.find_first_of(":\t");
        type = line.substr(0, delim_pos);
        delim_pos = line.find_first_of(":");
        line = line.substr(delim_pos+1);
        if (line.empty())
			value = "";
		else {
			// Offset by 1 for the space after :
			value = line.substr(1);
		}

        if (type == processor__str) {
			core.processor = std::stoul(value);
		} else if (type == vendor_id__str) {
			core.vendor_id = value;
		} else if (type == cpu_family__str) {
			core.cpu_family = std::stoul(value);
		} else if (type == model__str) {
			core.model = std::stoul(value);
		} else if (type == model_name__str) {
			core.model_name = value;
		} else if (type == stepping__str) {
			core.stepping = std::stoul(value);
		} else if (type == microcode__str) {
			core.microcode = value;
		} else if (type == cpu_MHz__str) {
			core.cpu_MHz = std::stof(value);
		} else if (type == cache_size__str) {
			core.cache_size = value;
		} else if (type == physical_id__str) {
			core.physical_id = std::stoul(value);
		} else if (type == siblings__str) {
			core.siblings = std::stoul(value);
		} else if (type == core_id__str) {
			core.core_id = std::stoul(value);
		} else if (type == cpu_cores__str) {
			core.cpu_cores = std::stoul(value);
		} else if (type == apicid__str) {
			core.apicid = std::stoul(value);
		} else if (type == initial_apicid__str) {
			core.initial_apicid = std::stoul(value);
		} else if (type == fpu__str) {
			core.fpu = value;
		} else if (type == fpu_exception__str) {
			core.fpu_exception = value;
		} else if (type == cpuid_level__str) {
			core.cpuid_level = std::stoul(value);
		} else if (type == wp__str) {
			core.wp = value;
		} else if (type == flags__str) {
			core.flags = value;
		} else if (type == bugs__str) {
			core.bugs = value;
		} else if (type == bogomips__str) {
			core.bogomips = std::stof(value);
		} else if (type == clflush_size__str) {
			core.clflush_size = std::stoul(value);
		} else if (type == cache_alignment__str) {
			core.cache_alignment = std::stoul(value);
		} else if (type == address_sizes__str) {
			core.address_sizes = value;
		} else if (type == power_management__str) {
			core.power_management = value;
		}
	}
}

void read_meminfo(struct meminfo *info) {
	std::string filepath("/proc/meminfo");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	const std::string MemTotal__str = "MemTotal";
	const std::string MemFree__str = "MemFree";
	const std::string MemAvailable__str = "MemAvailable";
	const std::string Buffers__str = "Buffers";
	const std::string Cached__str = "Cached";
	const std::string SwapCached__str = "SwapCached";
	const std::string Active__str = "Active";
	const std::string Inactive__str = "Inactive";
	const std::string Active_anon__str = "Active(anon)";
	const std::string Inactive_anon__str = "Inactive(anon)";
	const std::string Active_file__str = "Active(file)";
	const std::string Inactive_file__str = "Inactive(file)";
	const std::string Unevictable__str = "Unevictable";
	const std::string Mlocked__str = "Mlocked";
	const std::string SwapTotal__str = "SwapTotal";
	const std::string SwapFree__str = "SwapFree";
	const std::string Zswap__str = "Zswap";
	const std::string Zswapped__str = "Zswapped";
	const std::string Dirty__str = "Dirty";
	const std::string Writeback__str = "Writeback";
	const std::string AnonPages__str = "AnonPages";
	const std::string Mapped__str = "Mapped";
	const std::string Shmem__str = "Shmem";
	const std::string KReclaimable__str = "KReclaimable";
	const std::string Slab__str = "Slab";
	const std::string SReclaimable__str = "SReclaimable";
	const std::string SUnreclaim__str = "SUnreclaim";
	const std::string KernelStack__str = "KernelStack";
	const std::string PageTables__str = "PageTables";
	const std::string NFS_Unstable__str = "NFS_Unstable";
	const std::string Bounce__str = "Bounce";
	const std::string WritebackTmp__str = "WritebackTmp";
	const std::string CommitLimit__str = "CommitLimit";
	const std::string Committed_AS__str = "Committed_AS";
	const std::string VmallocTotal__str = "VmallocTotal";
	const std::string VmallocUsed__str = "VmallocUsed";
	const std::string VmallocChunk__str = "VmallocChunk";
	const std::string Percpu__str = "Percpu";
	const std::string HardwareCorrupted__str = "HardwareCorrupted";
	const std::string AnonHugePages__str = "AnonHugePages";
	const std::string ShmemHugePages__str = "ShmemHugePages";
	const std::string ShmemPmdMapped__str = "ShmemPmdMapped";
	const std::string FileHugePages__str = "FileHugePages";
	const std::string FilePmdMapped__str = "FilePmdMapped";
	const std::string HugePages_Total__str = "HugePages_Total";
	const std::string HugePages_Free__str = "HugePages_Free";
	const std::string HugePages_Rsvd__str = "HugePages_Rsvd";
	const std::string HugePages_Surp__str = "HugePages_Surp";
	const std::string Hugepagesize__str = "Hugepagesize";
	const std::string Hugetlb__str = "Hugetlb";
	const std::string DirectMap4k__str = "DirectMap4k";
	const std::string DirectMap2M__str = "DirectMap2M";
	const std::string DirectMap1G__str = "DirectMap1G";

    std::string line;
    std::string type;
    std::string value;
    int delim_pos;
    while (std::getline(infile, line)) {
        delim_pos = line.find_first_of(":\t");
        type = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_last_of(" ");
		// Remove kB for values that print it
		if (!isdigit(line.at(delim_pos+1))) {
			line = line.substr(0, delim_pos);
        	delim_pos = line.find_last_of(" ");
		}
        value = line.substr(delim_pos+1);

		if (type == MemTotal__str) {
			info->MemTotal = std::stoul(value);
		} else if (type == MemFree__str) {
			info->MemFree = std::stoul(value);
		} else if (type == MemAvailable__str) {
			info->MemAvailable = std::stoul(value);
		} else if (type == Buffers__str) {
			info->Buffers = std::stoul(value);
		} else if (type == Cached__str) {
			info->Cached = std::stoul(value);
		} else if (type == SwapCached__str) {
			info->SwapCached = std::stoul(value);
		} else if (type == Active__str) {
			info->Active = std::stoul(value);
		} else if (type == Inactive__str) {
			info->Inactive = std::stoul(value);
		} else if (type == Active_anon__str) {
			info->Active_anon = std::stoul(value);
		} else if (type == Inactive_anon__str) {
			info->Inactive_anon = std::stoul(value);
		} else if (type == Active_file__str) {
			info->Active_file = std::stoul(value);
		} else if (type == Inactive_file__str) {
			info->Inactive_file = std::stoul(value);
		} else if (type == Unevictable__str) {
			info->Unevictable = std::stoul(value);
		} else if (type == Mlocked__str) {
			info->Mlocked = std::stoul(value);
		} else if (type == SwapTotal__str) {
			info->SwapTotal = std::stoul(value);
		} else if (type == SwapFree__str) {
			info->SwapFree = std::stoul(value);
		} else if (type == Zswap__str) {
			info->Zswap = std::stoul(value);
		} else if (type == Zswapped__str) {
			info->Zswapped = std::stoul(value);
		} else if (type == Dirty__str) {
			info->Dirty = std::stoul(value);
		} else if (type == Writeback__str) {
			info->Writeback = std::stoul(value);
		} else if (type == AnonPages__str) {
			info->AnonPages = std::stoul(value);
		} else if (type == Mapped__str) {
			info->Mapped = std::stoul(value);
		} else if (type == Shmem__str) {
			info->Shmem = std::stoul(value);
		} else if (type == KReclaimable__str) {
			info->KReclaimable = std::stoul(value);
		} else if (type == Slab__str) {
			info->Slab = std::stoul(value);
		} else if (type == SReclaimable__str) {
			info->SReclaimable = std::stoul(value);
		} else if (type == SUnreclaim__str) {
			info->SUnreclaim = std::stoul(value);
		} else if (type == KernelStack__str) {
			info->KernelStack = std::stoul(value);
		} else if (type == PageTables__str) {
			info->PageTables = std::stoul(value);
		} else if (type == NFS_Unstable__str) {
			info->NFS_Unstable = std::stoul(value);
		} else if (type == Bounce__str) {
			info->Bounce = std::stoul(value);
		} else if (type == WritebackTmp__str) {
			info->WritebackTmp = std::stoul(value);
		} else if (type == CommitLimit__str) {
			info->CommitLimit = std::stoul(value);
		} else if (type == Committed_AS__str) {
			info->Committed_AS = std::stoul(value);
		} else if (type == VmallocTotal__str) {
			info->VmallocTotal = std::stoul(value);
		} else if (type == VmallocUsed__str) {
			info->VmallocUsed = std::stoul(value);
		} else if (type == VmallocChunk__str) {
			info->VmallocChunk = std::stoul(value);
		} else if (type == Percpu__str) {
			info->Percpu = std::stoul(value);
		} else if (type == HardwareCorrupted__str) {
			info->HardwareCorrupted = std::stoul(value);
		} else if (type == AnonHugePages__str) {
			info->AnonHugePages = std::stoul(value);
		} else if (type == ShmemHugePages__str) {
			info->ShmemHugePages = std::stoul(value);
		} else if (type == ShmemPmdMapped__str) {
			info->ShmemPmdMapped = std::stoul(value);
		} else if (type == FileHugePages__str) {
			info->FileHugePages = std::stoul(value);
		} else if (type == FilePmdMapped__str) {
			info->FilePmdMapped = std::stoul(value);
		} else if (type == HugePages_Total__str) {
			info->HugePages_Total = std::stoul(value);
		} else if (type == HugePages_Free__str) {
			info->HugePages_Free = std::stoul(value);
		} else if (type == HugePages_Rsvd__str) {
			info->HugePages_Rsvd = std::stoul(value);
		} else if (type == HugePages_Surp__str) {
			info->HugePages_Surp = std::stoul(value);
		} else if (type == Hugepagesize__str) {
			info->Hugepagesize = std::stoul(value);
		} else if (type == Hugetlb__str) {
			info->Hugetlb = std::stoul(value);
		} else if (type == DirectMap4k__str) {
			info->DirectMap4k = std::stoul(value);
		} else if (type == DirectMap2M__str) {
			info->DirectMap2M = std::stoul(value);
		} else if (type == DirectMap1G__str) {
			info->DirectMap1G = std::stoul(value);
		}
	}
}

void read_cpu_stat(struct cpu_stat *stats) {
	std::string filepath("/proc/stat");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::string cpu;

    infile >> cpu
		>> stats->user
		>> stats->nice
		>> stats->system
		>> stats->idle
		>> stats->iowait
		>> stats->irq
		>> stats->softirq
		>> stats->steal
		>> stats->guest
		>> stats->guest_nice;
}

void read_net_dev(std::vector<struct net_interface> *net_vec) {
	std::string filepath(std::string("/proc/net/dev"));
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::string line;
	// Skip the header lines
    std::getline(infile, line);
    std::getline(infile, line);

	struct net_interface net;
	while (infile) {
		net = {};
		infile >> net.interface;
		if (net.interface == "")
			continue;
		// Remove : from interface name
		net.interface = net.interface.substr(0, net.interface.size()-1);
		infile >> net.rx_bytes
			>> net.rx_packets
			>> net.rx_errs
			>> net.rx_drop
			>> net.rx_fifo
			>> net.rx_frame
			>> net.rx_compressed
			>> net.rx_multicast
			>> net.tx_bytes
			>> net.tx_packets
			>> net.tx_errs
			>> net.tx_drop
			>> net.tx_fifo
			>> net.tx_colls
			>> net.tx_carrier
			>> net.tx_compressed;
		net_vec->push_back(net);
	}
}

void read_pid_io(int32_t pid, struct pid_io *io) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/io");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	const std::string rchar__str = "rchar";
	const std::string wchar__str = "wchar";
	const std::string syscr__str = "syscr";
	const std::string syscw__str = "syscw";
	const std::string read_bytes__str = "read_bytes";
	const std::string write_bytes__str = "write_bytes";
	const std::string cancelled_write_bytes__str = "cancelled_write_bytes";

    std::string line;
    std::string type;
    std::string value;
    int delim_pos;
    while (std::getline(infile, line)) {
        delim_pos = line.find_first_of(":");
        type = line.substr(0, delim_pos);
        // Account for space after:
        value = line.substr(delim_pos+2);

		if (type == rchar__str) {
			io->rchar= std::stoul(value);
		} else if (type == wchar__str) {
			io->wchar= std::stoul(value);
		} else if (type == syscr__str) {
			io->syscr= std::stoul(value);
		} else if (type == syscw__str) {
			io->syscw= std::stoul(value);
		} else if (type == read_bytes__str) {
			io->read_bytes= std::stoul(value);
		} else if (type == write_bytes__str) {
			io->write_bytes= std::stoul(value);
		} else if (type == cancelled_write_bytes__str) {
			io->cancelled_write_bytes= std::stoul(value);
		}
	}
}

void read_pid_net(const int32_t pid, std::vector<struct net_interface> *net_vec) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/net/dev");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

    std::string line;
	// Skip the header lines
    std::getline(infile, line);
    std::getline(infile, line);

	struct net_interface net;
	while (infile) {
		net = {};
		infile >> net.interface;
		if (net.interface == "")
			continue;
		// Remove : from interface name
		net.interface = net.interface.substr(0, net.interface.size()-1);
		infile >> net.rx_bytes
			>> net.rx_packets
			>> net.rx_errs
			>> net.rx_drop
			>> net.rx_fifo
			>> net.rx_frame
			>> net.rx_compressed
			>> net.rx_multicast
			>> net.tx_bytes
			>> net.tx_packets
			>> net.tx_errs
			>> net.tx_drop
			>> net.tx_fifo
			>> net.tx_colls
			>> net.tx_carrier
			>> net.tx_compressed;
		net_vec->push_back(net);
	}
}

void read_pid_stat(int32_t pid, struct pid_stat *stats) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/stat");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	infile >> stats->pid
		>> stats->comm
		>> stats->state
		>> stats->ppid
		>> stats->pgrp
		>> stats->session
		>> stats->tty_nr
		>> stats->tpgid
		>> stats->flags
		>> stats->minflt
		>> stats->cminflt
		>> stats->majflt
		>> stats->cmajflt
		>> stats->utime
		>> stats->stime
		>> stats->cutime
		>> stats->cstime
		>> stats->priority
		>> stats->nice
		>> stats->num_threads
		>> stats->itrealvalue
		>> stats->starttime
		>> stats->vsize
		>> stats->rss
		>> stats->rsslim
		>> stats->startcode
		>> stats->endcode
		>> stats->startstack
		>> stats->kstkesp
		>> stats->kstkeip
		>> stats->signal
		>> stats->blocked
		>> stats->sigignore
		>> stats->sigcatch
		>> stats->wchan
		>> stats->nswap
		>> stats->cnswap
		>> stats->exit_signal
		>> stats->processor
		>> stats->rt_priority
		>> stats->policy
		>> stats->delayacct_blkio_ticks
		>> stats->guest_time
		>> stats->cguest_time;
}

void read_pid_statm(const int32_t pid, struct pid_statm *statm) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/statm");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	infile >> statm->size
		>> statm->resident
		>> statm->share
		>> statm->text
		>> statm->lib
		>> statm->data
		>> statm->dt;
}

void read_pid_status(const int32_t pid, struct pid_status *status) {
	std::string filepath(std::string("/proc/") + std::to_string(pid) + "/status");
    std::ifstream infile(filepath);
    if (!infile.is_open())
		return;

	const std::string Name__str = "Name";
	const std::string Umask__str = "Umask";
	const std::string State__str = "State";
	const std::string Tgid__str = "Tgid";
	const std::string Ngid__str = "Ngid";
	const std::string Pid__str = "Pid";
	const std::string PPid__str = "PPid";
	const std::string TracerPid__str = "TracerPid";
	const std::string Uid__str = "Uid";
	const std::string Gid__str = "Gid";
	const std::string FDSize__str = "FDSize";
	const std::string Groups__str = "Groups";
	const std::string NStgid__str = "NStgid";
	const std::string NSpid__str = "NSpid";
	const std::string NSpgid__str = "NSpgid";
	const std::string NSsid__str = "NSsid";
	const std::string VmPeak__str = "VmPeak";
	const std::string VmSize__str = "VmSize";
	const std::string VmLck__str = "VmLck";
	const std::string VmPin__str = "VmPin";
	const std::string VmHWM__str = "VmHWM";
	const std::string VmRSS__str = "VmRSS";
	const std::string RssAnon__str = "RssAnon";
	const std::string RssFile__str = "RssFile";
	const std::string RssShmem__str = "RssShmem";
	const std::string VmData__str = "VmData";
	const std::string VmStk__str = "VmStk";
	const std::string VmExe__str = "VmExe";
	const std::string VmLib__str = "VmLib";
	const std::string VmPTE__str = "VmPTE";
	const std::string VmSwap__str = "VmSwap";
	const std::string HugetlbPages__str = "HugetlbPages";
	const std::string CoreDumping__str = "CoreDumping";
	const std::string THP_enabled__str = "THP_enabled";
	const std::string Threads__str = "Threads";
	const std::string SigQ__str = "SigQ";
	const std::string SigPnd__str = "SigPnd";
	const std::string ShdPnd__str = "ShdPnd";
	const std::string SigBlk__str = "SigBlk";
	const std::string SigIgn__str = "SigIgn";
	const std::string SigCgt__str = "SigCgt";
	const std::string CapInh__str = "CapInh";
	const std::string CapPrm__str = "CapPrm";
	const std::string CapEff__str = "CapEff";
	const std::string CapBnd__str = "CapBnd";
	const std::string CapAmb__str = "CapAmb";
	const std::string NoNewPrivs__str = "NoNewPrivs";
	const std::string Seccomp__str = "Seccomp";
	const std::string Seccomp_filters__str = "Seccomp_filters";
	const std::string Speculation_Store_Bypass__str = "Speculation_Store_Bypass";
	const std::string SpeculationIndirectBranch__str = "SpeculationIndirectBranch";
	const std::string Cpus_allowed__str = "Cpus_allowed";
	const std::string Cpus_allowed_list__str = "Cpus_allowed_list";
	const std::string Mems_allowed__str = "Mems_allowed";
	const std::string Mems_allowed_list__str = "Mems_allowed_list";
	const std::string voluntary_ctxt_switches__str = "voluntary_ctxt_switches";
	const std::string nonvoluntary_ctxt_switches__str = "nonvoluntary_ctxt_switches";

    std::string line;
    std::string type;
    std::string value;
    int delim_pos;
    while (std::getline(infile, line)) {
        delim_pos = line.find_first_of(":");
        type = line.substr(0, delim_pos);
        line = line.substr(delim_pos+1);
        delim_pos = line.find_first_not_of(" \t");
		if (delim_pos != -1)
	        value = line.substr(delim_pos);
		else
			value = line;

		if (type == Name__str) {
			status->Name = value;
		} else if (type == Umask__str) {
			status->Umask = value;
		} else if (type == State__str) {
			status->State = value;
		} else if (type == Tgid__str) {
			status->Tgid = value;
		} else if (type == Ngid__str) {
			status->Ngid = value;
		} else if (type == Pid__str) {
			status->Pid = value;
		} else if (type == PPid__str) {
			status->PPid = value;
		} else if (type == TracerPid__str) {
			status->TracerPid = value;
		} else if (type == Uid__str) {
			status->Uid = value;
		} else if (type == Gid__str) {
			status->Gid = value;
		} else if (type == FDSize__str) {
			status->FDSize = value;
		} else if (type == Groups__str) {
			status->Groups = value;
		} else if (type == NStgid__str) {
			status->NStgid = value;
		} else if (type == NSpid__str) {
			status->NSpid = value;
		} else if (type == NSpgid__str) {
			status->NSpgid = value;
		} else if (type == NSsid__str) {
			status->NSsid = value;
		} else if (type == VmPeak__str) {
			status->VmPeak = value;
		} else if (type == VmSize__str) {
			status->VmSize = value;
		} else if (type == VmLck__str) {
			status->VmLck = value;
		} else if (type == VmPin__str) {
			status->VmPin = value;
		} else if (type == VmHWM__str) {
			status->VmHWM = value;
		} else if (type == VmRSS__str) {
			status->VmRSS = value;
		} else if (type == RssAnon__str) {
			status->RssAnon = value;
		} else if (type == RssFile__str) {
			status->RssFile = value;
		} else if (type == RssShmem__str) {
			status->RssShmem = value;
		} else if (type == VmData__str) {
			status->VmData = value;
		} else if (type == VmStk__str) {
			status->VmStk = value;
		} else if (type == VmExe__str) {
			status->VmExe = value;
		} else if (type == VmLib__str) {
			status->VmLib = value;
		} else if (type == VmPTE__str) {
			status->VmPTE = value;
		} else if (type == VmSwap__str) {
			status->VmSwap = value;
		} else if (type == HugetlbPages__str) {
			status->HugetlbPages = value;
		} else if (type == CoreDumping__str) {
			status->CoreDumping = value;
		} else if (type == THP_enabled__str) {
			status->THP_enabled = value;
		} else if (type == Threads__str) {
			status->Threads = value;
		} else if (type == SigQ__str) {
			status->SigQ = value;
		} else if (type == SigPnd__str) {
			status->SigPnd = value;
		} else if (type == ShdPnd__str) {
			status->ShdPnd = value;
		} else if (type == SigBlk__str) {
			status->SigBlk = value;
		} else if (type == SigIgn__str) {
			status->SigIgn = value;
		} else if (type == SigCgt__str) {
			status->SigCgt = value;
		} else if (type == CapInh__str) {
			status->CapInh = value;
		} else if (type == CapPrm__str) {
			status->CapPrm = value;
		} else if (type == CapEff__str) {
			status->CapEff = value;
		} else if (type == CapBnd__str) {
			status->CapBnd = value;
		} else if (type == CapAmb__str) {
			status->CapAmb = value;
		} else if (type == NoNewPrivs__str) {
			status->NoNewPrivs = value;
		} else if (type == Seccomp__str) {
			status->Seccomp = value;
		} else if (type == Seccomp_filters__str) {
			status->Seccomp_filters = value;
		} else if (type == Speculation_Store_Bypass__str) {
			status->Speculation_Store_Bypass = value;
		} else if (type == SpeculationIndirectBranch__str) {
			status->SpeculationIndirectBranch = value;
		} else if (type == Cpus_allowed__str) {
			status->Cpus_allowed = value;
		} else if (type == Cpus_allowed_list__str) {
			status->Cpus_allowed_list = value;
		} else if (type == Mems_allowed__str) {
			status->Mems_allowed = value;
		} else if (type == Mems_allowed_list__str) {
			status->Mems_allowed_list = value;
		} else if (type == voluntary_ctxt_switches__str) {
			status->voluntary_ctxt_switches = value;
		} else if (type == nonvoluntary_ctxt_switches__str) {
			status->nonvoluntary_ctxt_switches = value;
		}
	}
}

void get_uptime(uint64_t *uptime) {
	std::string filepath("/proc/uptime");
    std::ifstream infile(filepath);
    if (!infile.is_open())
        return;

    infile >> *uptime;
}

void get_pid_uptime(const uint64_t pid, uint64_t *pid_uptime) {
    struct pid_stat stats;
    read_pid_stat(pid, &stats);
    // Convert from ticks to seconds
    uint32_t ticks_per_s = sysconf(_SC_CLK_TCK);
    // Start time in seconds since boot
    uint64_t system_uptime;
    get_uptime(&system_uptime);
    stats.starttime = stats.starttime / ticks_per_s;
    *pid_uptime = system_uptime - stats.starttime;
}
