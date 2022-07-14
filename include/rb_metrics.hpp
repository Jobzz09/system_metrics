#ifndef RB_METRICS
#define RB_METRICS

#include <vector>
#include <iostream>
#include <thread>

std::vector<unsigned int> GetAllChildren(unsigned int pid);

class Rb_metrics
{
public:
    Rb_metrics(unsigned int pid, unsigned long seconds) : m_pid(pid),
                                                          m_period(seconds),
                                                          m_children_pids(GetAllChildren(m_pid)) {}
    ~Rb_metrics();

    // Cpu metrics

    // Returns general cpu usage percentage
    double GetGeneralCpuUsage();

    // Returns process and its children's cpu usage percentage
    double GetCpuUsage();

    // RAM metrics

    // Returns general ram usage percentage
    double GetGeneralRamUsage();

    // Returns process and its children's ram usage percentage
    double GetRamUsage();

    // Returns ram usage in megabytes
    unsigned long GetGeneralRamUsage_m();

    // Returns process and its children's ram usage in megabytes
    unsigned long GetRamUsage_m();

    // Network

    // Returns general net usage in kilobytes(read, write)
    std::pair<unsigned long long, unsigned long long> GetGeneralNetUsage();

    // Returns process and its children's net usage in kilobytes(read, write)
    std::pair<unsigned long long, unsigned long long> GetNetUsage();

    // Block devices

    // Returns general io statistics in kilobytes(read, write)
    std::pair<unsigned long long, unsigned long long> GetGeneralIoStats();

    // Returns process and its children's general io statistics in kilobytes(read, write)
    std::pair<unsigned long long, unsigned long long> GetIoStats();

private:
    double getCpuUsage(unsigned int pid = 0);

    double getRamUsage(unsigned int pid = 0);

    unsigned long getRamUsage_m(unsigned int pid = 0);

    std::pair<unsigned long long, unsigned long long> getNetUsage(unsigned int pid = 0);

    std::pair<unsigned long long, unsigned long long> getIoStats(unsigned int pid = 0);

    unsigned int m_pid;                        // Pid of the process under examination
    unsigned long m_period;                    // Time period which data should be measured within
    std::vector<unsigned int> m_children_pids; // Vector of children processes pids
};

#endif