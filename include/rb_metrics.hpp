#ifndef RB_METRICS
#define RB_METRICS

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <thread>

#define PROCFS_ROOT "/proc/"
#define SYSFS_ROOT "/sys/"

class AbstractFile
{
public:
    enum class Entry
    {

    };

    enum class Result {
        SUCCESS = 0,
        ERROR = 1
    };

    Result GetLastError();

    // Return entry of the file
    Result GetEntryValue(Entry entry, std::string& result);

protected:
    std::vector<std::string> fileContent;

    virtual Result ReadFile(std::string&& filePath);

    Result last_error = Result::SUCCESS;
};

class ProcStatFile : public AbstractFile
{
public:
    enum class Entry
    {
        USER = 1,
        NICE = 2,
        SYSTEM = 3,
        IDLE = 4,
        UTIME = 13,
        STIME = 14,
        CUTIME = 15,
        CSTIME = 16,
        PPID = 3
    };

    ProcStatFile(uint32_t pid = 0);

    Result GetEntryValue(Entry entry, std::string& dst_string);
};

class ProcStatusFile : public AbstractFile
{
public:
    enum class Entry
    {
        VMRSS = 65,
        PPID = 14
    };

    ProcStatusFile(uint32_t pid);

    Result GetEntryValue(Entry entry, std::string& dst_string);
};

class ProcMemInfoFile : public AbstractFile
{
public:
    enum class Entry
    {
        MEM_TOTAL = 1,
        MEM_AVAILABLE = 7
    };

    ProcMemInfoFile(uint32_t pid = 0);

    Result GetEntryValue(Entry entry, std::string& dst_string);
};

class ProcNetDevFile : public AbstractFile
{
public:
    enum class Entry
    {
        RECV_BYTES = 1,
        WRITE_BYTES = 9
    };

    ProcNetDevFile(uint32_t pid = 0);

    Result GetEntryValue(const std::string& interface_name, Entry entry, std::string& dst_string);

private:
    Result ReadFile(std::string&& filePath) override;
    std::map<std::string, std::vector<std::string>> line_content_for_device;
};

class SysBlockStatFile : public AbstractFile
{
public:
    enum class Entry
    {
        READ_SECTORS = 2,
        WRITE_SECTORS = 6
    };

    Result GetEntryValue(const std::string& block_device, Entry entry, std::string& dst_string);

    SysBlockStatFile(uint32_t pid = 0);

private:
    std::map<std::string, std::vector<std::string>> line_content_for_device;
};

class ProcPidIoFile : public AbstractFile
{
public:
    enum class Entry
    {
        RCHAR = 1,
        WCHAR = 3
    };

    ProcPidIoFile(uint32_t pid);

    Result GetEntryValue(Entry entry, std::string& dst_string);
};

struct cpu_stat
{
    uint32_t user, nice,
        system, idle,
        utime, stime,
        cutime, cstime;
};

struct ram_stat
{
    uint32_t ram_available,
        ram_total;
    uint32_t pid, vmrss;
};

struct net_stat
{
    uint64_t r_bytes, w_bytes;
    uint32_t pid;
};

struct bd_stat
{
    uint64_t r_bytes, w_bytes;
    uint32_t pid;
};

class Collector
{
public:
    Collector() {}

    cpu_stat CpuStat(uint32_t pid = 0);

    ram_stat RamStat(uint32_t pid = 0);

    net_stat NetStat(uint32_t pid = 0);

    bd_stat BdStat(uint32_t pid = 0);
};

Collector Collect();

class Calculator
{
public:
    Calculator() {}
    uint32_t GeneralCpuUsage(cpu_stat early_cpu_data, cpu_stat lately_cpu_data);
    uint32_t CpuUsage(cpu_stat early_cpu_data, cpu_stat lately_cpu_data);

    uint32_t GeneralRamUsage(ram_stat ram_data);
    uint32_t RamUsage(ram_stat ram_data);

    uint32_t GeneralRamUsage_m(ram_stat ram_data);
    uint32_t RamUsage_m(ram_stat ram_data);

    uint32_t NetDownload(net_stat early_net_data, net_stat lately_net_data);
    uint32_t NetUpload(net_stat early_net_data, net_stat lately_net_data);

    uint32_t BlockDeviceWrite(bd_stat early_bd_data, bd_stat lately_bd_data);
    uint32_t BlockDeviceRead(bd_stat early_bd_data, bd_stat lately_bd_data);
};

class Rb_metrics
{
public:
    Rb_metrics(unsigned int pid, unsigned long seconds) : m_pid(pid),
                                                          m_period(seconds),
                                                          m_children_pids(getAllChildren(m_pid)) {}
    // Cpu metrics

    // Returns general cpu usage percentage over the period
    uint32_t GetGeneralCpuUsage();

    // Returns process and its children's cpu usage percentage over the period
    uint32_t GetCpuUsage();

    // RAM metrics

    // Returns general ram usage percentage over the period
    uint32_t GetGeneralRamUsage();

    // Returns process and its children's ram usage percentage over the period
    uint32_t GetRamUsage();

    // Returns ram usage in megabytes over the period
    uint32_t GetGeneralRamUsage_m();

    // Returns process and its children's ram usage in megabytes over the period
    uint32_t GetRamUsage_m();

    // Network

    // Returns general net usage in kilobytes(read, write) over the period
    std::pair<uint64_t, uint64_t> GetGeneralNetUsage();

    // Returns process and its children's net usage in kilobytes(read, write) over the period
    std::pair<uint64_t, uint64_t> GetNetUsage();

    // Block devices

    // Returns general io statistics in kilobytes(read, write) over the period
    std::pair<uint64_t, uint64_t> GetGeneralIoStat();

    // Returns process and its children's general io statistics in kilobytes(read, write) over the period
    std::pair<uint64_t, uint64_t> GetIoStat();

private:
    // Returns cpu usage percentage over the period(m_period)
    uint32_t getCpuUsage(uint32_t pid = 0);

    // Returns RAM usage percentage over the period(m_period)
    uint32_t getRamUsage(uint32_t pid = 0);

    // Returns RAM usage in megabytes over the period(m_period)
    uint32_t getRamUsage_m(uint32_t pid = 0);

    // Returns network usage(pair of (uint32_t readed, uint32_t written)) in kilobytes over the period(m_period)
    std::pair<uint64_t, uint64_t> getNetUsage(uint32_t pid = 0);

    // Returns IO usage (pair of (uint32_t readed, uint32_t written)) in kilobytes over the period(m_period)
    std::pair<uint64_t, uint64_t> getIostat(uint32_t pid = 0);

    // Returns all children of the current pid(m_pid)
    std::vector<uint32_t> getAllChildren(uint32_t pid);

    uint32_t m_pid;                        // Pid of the process under examination
    uint32_t m_period;                     // Time period which data should be measured within
    std::vector<uint32_t> m_children_pids; // Vector of children processes pids
};

#endif