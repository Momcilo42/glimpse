#ifndef PROC_HPP_
#define PROC_HPP_

#include <iostream>
#include <vector>

extern "C" {
	#include <unistd.h> // getuid(), access()
}

// x86_64 Linux version
struct cpuinfo_core {
	uint32_t processor = 0;
	std::string vendor_id = "";
	uint32_t cpu_family = 0;
	uint32_t model = 0;
	std::string model_name = "";
	uint32_t stepping = 0;
	std::string microcode = "";
	_Float32 cpu_MHz = 0;
	std::string cache_size = "";
	uint32_t physical_id = 0;
	uint32_t siblings = 0;
	uint32_t core_id = 0;
	uint32_t cpu_cores = 0;
	uint32_t apicid = 0;
	uint32_t initial_apicid = 0;
	std::string fpu = "";
	std::string fpu_exception = "";
	uint32_t cpuid_level = 0;
	std::string wp = "";
	std::string flags = "";
	std::string bugs = "";
	_Float32 bogomips = 0;
	uint32_t clflush_size = 0;
	uint32_t cache_alignment = 0;
	std::string address_sizes = "";
	std::string power_management = "";
};

struct cpu_stat {
	// (1) Time spent in user mode.
	uint64_t user;
	// (2) Time spent in user mode with low priority (nice).
	uint64_t nice;
	// (3) Time spent in system mode.
	uint64_t system;
	// (4) Time spent in the idle task. This value should be USER_HZ times the second entry in the /proc/uptime pseudo-file.
	uint64_t idle;
	// (5) Time waiting for I/O to complete.
	uint64_t iowait;
	// (6) Time servicing interrupts.
	uint64_t irq;
	// (7) Time servicing softirqs.
	uint64_t softirq;
	// (8) Stolen time, which is the time spent in other operating systems when running in a virtualized environment
	uint64_t steal;
	// (9) Time spent running a virtual CPU for guest operating systems under the control of the Linux kernel.
	uint64_t guest;
	// (10) Time spent running a niced guest (virtual CPU for guest operating systems under the control of the Linux kernel).
	uint64_t guest_nice;

    uint64_t total() {
        // Guest time is already accounted in user
        user -= guest;
        nice -= guest_nice;
        uint64_t idlealltime = idle + iowait;
        uint64_t systemalltime = system + irq + softirq;
        uint64_t virtalltime = guest + guest_nice;
        uint64_t totaltime = user + nice + systemalltime + idlealltime + steal + virtalltime;
        return totaltime;
    }
};

// Values are in kB
struct meminfo {
	uint32_t MemTotal = 0;
	uint32_t MemFree = 0;
	uint32_t MemAvailable = 0;
	uint32_t Buffers = 0;
	uint32_t Cached = 0;
	uint32_t SwapCached = 0;
	uint32_t Active = 0;
	uint32_t Inactive = 0;
	uint32_t Active_anon = 0;
	uint32_t Inactive_anon = 0;
	uint32_t Active_file = 0;
	uint32_t Inactive_file = 0;
	uint32_t Unevictable = 0;
	uint32_t Mlocked = 0;
	uint32_t SwapTotal = 0;
	uint32_t SwapFree = 0;
	uint32_t Zswap = 0;
	uint32_t Zswapped = 0;
	uint32_t Dirty = 0;
	uint32_t Writeback = 0;
	uint32_t AnonPages = 0;
	uint32_t Mapped = 0;
	uint32_t Shmem = 0;
	uint32_t KReclaimable = 0;
	uint32_t Slab = 0;
	uint32_t SReclaimable = 0;
	uint32_t SUnreclaim = 0;
	uint32_t KernelStack = 0;
	uint32_t PageTables = 0;
	uint32_t NFS_Unstable = 0;
	uint32_t Bounce = 0;
	uint32_t WritebackTmp = 0;
	uint32_t CommitLimit = 0;
	uint32_t Committed_AS = 0;
	uint32_t VmallocTotal = 0;
	uint32_t VmallocUsed = 0;
	uint32_t VmallocChunk = 0;
	uint32_t Percpu = 0;
	uint32_t HardwareCorrupted = 0;
	uint32_t AnonHugePages = 0;
	uint32_t ShmemHugePages = 0;
	uint32_t ShmemPmdMapped = 0;
	uint32_t FileHugePages = 0;
	uint32_t FilePmdMapped = 0;
	uint32_t HugePages_Total = 0;
	uint32_t HugePages_Free = 0;
	uint32_t HugePages_Rsvd = 0;
	uint32_t HugePages_Surp = 0;
	uint32_t Hugepagesize = 0;
	uint32_t Hugetlb = 0;
	uint32_t DirectMap4k = 0;
	uint32_t DirectMap2M = 0;
	uint32_t DirectMap1G = 0;
};

// Values are in B
struct pid_io {
	/* The number of bytes which this task has caused to be read from
	 * storage. This is simply the sum of bytes which this process
	 * passed to read(2) and similar system calls. It includes things
	 * such as terminal I/O and is unaffected by whether or not actual
	 * physical disk I/O was required (the read might have been
	 * satisfied from pagecache).
	 */
	uint64_t rchar = 0;
	/* The number of bytes which this task has caused, or shall cause
	 * to be written to disk. Similar caveats apply here as with
	 * rchar.
	 */
	uint64_t wchar = 0;
	/* Attempt to count the number of read I/O operations—that is,
	 * system calls such as read(2) and pread(2).
	 */
	uint64_t syscr = 0;
	/* Attempt to count the number of write I/O operations—that is,
	 * system calls such as write(2) and pwrite(2).
	 */
	uint64_t syscw = 0;
	/* Attempt to count the number of bytes which this process really
	 * did cause to be fetched from the storage layer. This is
	 * accurate for block-backed filesystems.
	 */
	uint64_t read_bytes = 0;
	/* Attempt to count the number of bytes which this process caused
	 * to be sent to the storage layer.
	 */
	uint64_t write_bytes = 0;
	/* The big inaccuracy here is truncate. If a process writes 1 MB
	 * to a file and then deletes the file, it will in fact perform no
	 * writeout. But it will have been accounted as having caused 1 MB
	 * of write. In other words: this field represents the number of
	 * bytes which this process caused to not happen, by truncating
	 * pagecache. A task can cause "negative" I/O too. If this task
	 * truncates some dirty pagecache, some I/O which another task has
	 * been accounted for (in its write_bytes) will not be happening.
	 */
	uint64_t cancelled_write_bytes = 0;
};

struct net_interface {
	std::string interface = "";
	uint64_t rx_bytes = 0;
	uint64_t rx_packets = 0;
	uint64_t rx_errs = 0;
	uint64_t rx_drop = 0;
	uint64_t rx_fifo = 0;
	uint64_t rx_frame = 0;
	uint64_t rx_compressed = 0;
	std::string rx_multicast = "";
	uint64_t tx_bytes = 0;
	uint64_t tx_packets = 0;
	uint64_t tx_errs = 0;
	uint64_t tx_drop = 0;
	uint64_t tx_fifo = 0;
	uint64_t tx_colls = 0;
	uint64_t tx_carrier = 0;
	uint64_t tx_compressed = 0;
};

struct pid_stat {
    // (1) The process ID.
	int32_t pid;
    // (2) The filename of the executable, in parentheses. This is visible whether or not the executable is swapped out.
	std::string comm;
    // (3) One character from the string "RSDZTW" where R is running, S is sleeping in an interruptible wait, D is waiting in uninterruptible disk sleep, Z is zombie, T is traced or stopped (on a signal), and W is paging.
	char state;
    // (4) The PID of the parent.
	int32_t ppid;
    // (5) The process group ID of the process.
	int32_t pgrp;
    // (6) The session ID of the process.
	int32_t session;
    // (7) The controlling terminal of the process. (The minor device number is contained in the combination of bits 31 to 20 and 7 to 0; the major device number is in bits 15 to 8.)
	int32_t tty_nr;
    // (8) The ID of the foreground process group of the controlling terminal of the process.
	int32_t tpgid;
	// (9) The kernel flags word of the process. For bit meanings, see the PF_* defines in the Linux kernel source file include/linux/sched.h. Details depend on the kernel version.
	uint32_t flags;
    // (10) The number of minor faults the process has made which have not required loading a memory page from disk.
	uint64_t minflt;
    // (11) The number of minor faults that the process's waited-for children have made.
	uint64_t cminflt;
    // (12) The number of major faults the process has made which have required loading a memory page from disk.
	uint64_t majflt;
    // (13) The number of major faults that the process's waited-for children have made.
	uint64_t cmajflt;
    // (14) Amount of time that this process has been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)). This includes guest time, guest_time (time spent running a virtual CPU, see below), so that applications that are not aware of the guest time field do not lose that time from their calculations.
	uint64_t utime;
    // (15) Amount of time that this process has been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	uint64_t stime;
    // (16) Amount of time that this process's waited-for children have been scheduled in user mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)). (See also times(2).) This includes guest time, cguest_time (time spent running a virtual CPU, see below).
	uint64_t cutime;
    // (17) Amount of time that this process's waited-for children have been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	uint64_t cstime;
	// (18) For processes running a real-time scheduling policy (policy below; see sched_setscheduler(2)), this is the negated scheduling priority, minus one; that is, a number in the range -2 to -100, corresponding to real-time priorities 1 to 99. For processes running under a non-real-time scheduling policy, this is the raw nice value (setpriority(2)) as represented in the kernel. The kernel stores nice values as numbers in the range 0 (high) to 39 (low), corresponding to the user-visible nice range of -20 to 19.
	uint64_t priority;
    // (19) The nice value (see setpriority(2)), a value in the range 19 (low priority) to -20 (high priority).
    uint64_t nice;
	// (20) Number of threads in this process (since Linux 2.6). Before kernel 2.6, this field was hard coded to 0 as a placeholder for an earlier removed field.
	uint64_t num_threads;
	// (21) The time in jiffies before the next SIGALRM is sent to the process due to an interval timer. Since kernel 2.6.17, this field is no longer maintained, and is hard coded as 0.
	uint64_t itrealvalue;
	// (22) The time the process started after system boot. In kernels before Linux 2.6, this value was expressed in jiffies. Since Linux 2.6, the value is expressed in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	unsigned long long starttime;
    // (23) Virtual memory size in bytes.
	uint64_t vsize;
    // (24) Resident Set Size: number of pages the process has in real memory. This is just the pages which count toward text, data, or stack space. This does not include pages which have not been demand-loaded in, or which are swapped out.
	uint64_t rss;
    // (25) Current soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getrlimit(2).
    uint64_t rsslim;
	// (26) The address above which program text can run.
	uint64_t startcode;
    // (27) The address below which program text can run.
	uint64_t endcode;
	// (28) The address of the start (i.e., bottom) of the stack.
	uint64_t startstack;
    // (29) The current value of ESP (stack pointer), as found in the kernel stack page for the process.
	uint64_t kstkesp;
    // (30) The current EIP (instruction pointer).
	uint64_t kstkeip;
    // (31) The bitmap of pending signals, displayed as a decimal number. Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
	uint64_t signal;
    // (32) The bitmap of blocked signals, displayed as a decimal number. Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
    uint64_t blocked;
	// (33) The bitmap of ignored signals, displayed as a decimal number. Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
	uint64_t sigignore;
	// (34) The bitmap of caught signals, displayed as a decimal number. Obsolete, because it does not provide information on real-time signals; use /proc/[pid]/status instead.
	uint64_t sigcatch;
    // (35) This is the "channel" in which the process is waiting. It is the address of a system call, and can be looked up in a namelist if you need a textual name. (If you have an up-to-date /etc/psdatabase, then try ps -l to see the WCHAN field in action.)
	uint64_t wchan;
    // (36) Number of pages swapped (not maintained).
	uint64_t nswap;
    // (37) Cumulative nswap for child processes (not maintained).
	uint64_t cnswap;
	// (38) Signal to be sent to parent when we die.
	int32_t exit_signal;
	// (39) CPU number last executed on.
	int32_t processor;
	// (40) Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
	uint32_t rt_priority;
	// (41) Scheduling policy (see sched_setscheduler(2)). Decode using the SCHED_* constants in linux/sched.h.
	uint32_t policy;
	// (42) Aggregated block I/O delays, measured in clock ticks (centiseconds).
	unsigned long long delayacct_blkio_ticks;
	// (43) Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	uint64_t guest_time;
	// (44) Guest time of the process's children, measured in clock ticks (divide by sysconf(_SC_CLK_TCK)).
	int64_t cguest_time;
};

// Values are in memory pages
struct pid_statm {
	// (1) total program size (same as VmSize in /proc/[pid]/status)
	uint32_t size = 0;
	// (2) resident set size (same as VmRSS in /proc/[pid]/status)
	uint32_t resident = 0;
	// (3) shared pages (i.e., backed by a file)
	uint32_t share = 0;
	// (4) text (code)
	uint32_t text = 0;
	// (5) library (unused in Linux 2.6)
	uint32_t lib = 0;
	// (6) data + stack
	uint32_t data = 0;
	// (7) dirty pages (unused in Linux 2.6)
	uint32_t dt = 0;
};

struct pid_status {
	std::string Name = "";
	std::string Umask = "";
	std::string State = "";
	std::string Tgid = "";
	std::string Ngid = "";
	std::string Pid = "";
	std::string PPid = "";
	std::string TracerPid = "";
	std::string Uid = "";
	std::string Gid = "";
	std::string FDSize = "";
	std::string Groups = "";
	std::string NStgid = "";
	std::string NSpid = "";
	std::string NSpgid = "";
	std::string NSsid = "";
	std::string VmPeak = "";
	std::string VmSize = "";
	std::string VmLck = "";
	std::string VmPin = "";
	std::string VmHWM = "";
	std::string VmRSS = "";
	std::string RssAnon = "";
	std::string RssFile = "";
	std::string RssShmem = "";
	std::string VmData = "";
	std::string VmStk = "";
	std::string VmExe = "";
	std::string VmLib = "";
	std::string VmPTE = "";
	std::string VmSwap = "";
	std::string HugetlbPages = "";
	std::string CoreDumping = "";
	std::string THP_enabled = "";
	std::string Threads = "";
	std::string SigQ = "";
	std::string SigPnd = "";
	std::string ShdPnd = "";
	std::string SigBlk = "";
	std::string SigIgn = "";
	std::string SigCgt = "";
	std::string CapInh = "";
	std::string CapPrm = "";
	std::string CapEff = "";
	std::string CapBnd = "";
	std::string CapAmb = "";
	std::string NoNewPrivs = "";
	std::string Seccomp = "";
	std::string Seccomp_filters = "";
	std::string Speculation_Store_Bypass = "";
	std::string SpeculationIndirectBranch = "";
	std::string Cpus_allowed = "";
	std::string Cpus_allowed_list = "";
	std::string Mems_allowed = "";
	std::string Mems_allowed_list = "";
	std::string voluntary_ctxt_switches = "";
	std::string nonvoluntary_ctxt_switches = "";
};

struct process {
	uint64_t			pid = 0;
	std::string			name = "";
	std::string			cmd = "";
	bool				is_alive = false;
};

void find_processes(std::vector<struct process> *processes);

// Get data from /proc/{pid}/cmdline
void get_calling_command(const int32_t pid, std::string *cmd);
// Get data from /proc/{pid}/comm
void get_process_name(const int32_t pid, std::string *name);
// Get data from /proc/cpuinfo // x86_64 Linux version
void read_cpuinfo(std::vector<struct cpuinfo_core> *info);
// Get data from /proc/meminfo
void read_meminfo(struct meminfo *info);
// Get data from /proc/net/dev
void read_net_dev(std::vector<struct net_interface> *net_vec);
// Get data from /proc/stat
void read_cpu_stat(struct cpu_stat *stats);
// Get data from /proc/{pid}/io
void read_pid_io(const int32_t pid, struct pid_io *io);
// Get data from /proc/{pid}/net/dev
void read_pid_net(const int32_t pid, std::vector<struct net_interface> *net_vec);
// Get data from /proc/{pid}/stat
void read_pid_stat(const int32_t pid, struct pid_stat *stats);
// Get data from /proc/{pid}/statm
void read_pid_statm(const int32_t pid, struct pid_statm *statm);
// Get data from /proc/{pid}/status
void read_pid_status(const int32_t pid, struct pid_status *status);
// Get data from /proc/uptime
void get_uptime(uint64_t *usage); // in seconds

void get_pid_uptime(const uint64_t pid, uint64_t *pid_uptime);

#endif // PROC_HPP_
