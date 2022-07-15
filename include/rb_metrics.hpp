#ifndef RB_METRICS
#define RB_METRICS

#include <vector>
#include <iostream>
#include <cstdint>
#include <thread>

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
    std::pair<uint64_t, uint64_t> GetGeneralIoStats();

    // Returns process and its children's general io statistics in kilobytes(read, write) over the period
    std::pair<uint64_t, uint64_t> GetIoStats();

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
    std::pair<uint64_t, uint64_t> getIoStats(uint32_t pid = 0);

    // Returns all children of the current pid(m_pid)
    std::vector<uint32_t> getAllChildren(uint32_t pid);

    uint32_t m_pid;                        // Pid of the process under examination
    uint32_t m_period;                     // Time period which data should be measured within
    std::vector<uint32_t> m_children_pids; // Vector of children processes pids
};

#endif