#include "rb_metrics.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/algorithm.hpp>

std::string ProcStat2String(unsigned int pid);
unsigned int GetPpidFromProcStatString(std::string procStat_string);

namespace system_metrics
{
    std::string GetActiveNetInterface();
    unsigned long long GetSystemUptime();
    unsigned long long GetCpuSnapshot(unsigned int pid = 0);
    unsigned long long GetRamOccupied(unsigned int pid = 0);
    std::pair<unsigned long long, unsigned long long> ParseNetData(unsigned int pid = 0);
    std::pair<unsigned long long, unsigned long long> ParseIoStats(unsigned int pid = 0);
}

int ParseLine(char *line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9')
        p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

bool Is_number(const std::string &data)
{
    std::string::const_iterator it = data.begin();
    while (it != data.end() && std::isdigit(*it))
        ++it;
    return !data.empty() && it == data.end();
}

std::string ExecCommandWithResult(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        return result;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

std::vector<unsigned int> GetAllChildren(unsigned int pid)
{
    boost::filesystem::path proc("/proc");
    std::vector<unsigned int> result;
    unsigned proc_count = 0;
    if (boost::filesystem::is_directory(proc))
    {
        for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(proc), {}))
        {
            boost::filesystem::path tmp(entry);
            if (Is_number(tmp.filename().string()))
            {
                unsigned int curr_pid = atoll(tmp.filename().string().c_str());
                std::string statData = ProcStat2String(curr_pid);
                unsigned int ppid = GetPpidFromProcStatString(statData);
                if (ppid == pid)
                {
                    result.push_back(curr_pid);
                }
            }
        }
    }
    return result;
}

std::string ProcStat2String(unsigned int pid)
{
    std::string result;
    std::string path = "/proc/";
    path += std::to_string(pid);
    path += "/stat";

    std::ifstream fopen(path);
    if (fopen.is_open())
    {
        while (!fopen.eof())
        {
            std::string tmp_str;
            fopen >> tmp_str;
            result += tmp_str + " ";
        }
        fopen.close();
    }
    return result;
}

unsigned int GetPpidFromProcStatString(std::string procStat_string)
{
    if (procStat_string.empty())
    {
        return 0;
    }

    std::stringstream ss(procStat_string);

    unsigned int ppid;
    std::string tmp;
    ss >> tmp;
    ss >> tmp;
    ss >> tmp;
    ss >> tmp;
    ppid = atoll(tmp.c_str());
    return ppid;
}

namespace system_metrics
{
    unsigned long long GetSystemUptime()
    {
        std::ifstream fin("/proc/uptime");
        unsigned long long result;
        if (fin.is_open())
        {
            fin >> result;
        }
        fin.close();
        return result;
    }
    unsigned long long GetCpuSnapshot(unsigned int pid)
    {
        std::string path = "/proc";
        if (pid == 0)
        {
            path += "/stat";
        }
        else
        {
            path += "/" + std::to_string(pid) + "/stat";
        }
        unsigned long long totalUser = 0, totalUserLow = 0, totalSys = 0, totalIdle = 0, total = 0;
        std::ifstream fin(path);
        if (fin.is_open())
        {
            if (pid == 0)
            {
                std::string tmp;
                fin >> tmp; // cpu

                fin >> tmp;
                totalUser = atoll(tmp.c_str());

                fin >> tmp;
                totalUserLow = atoll(tmp.c_str());

                fin >> tmp;
                totalSys = atoll(tmp.c_str());

                fin >> tmp;
                totalIdle = atoll(tmp.c_str());
            }
            else
            {
                std::string tmp;
                for (int i = 0; i < 13; i++)
                {
                    fin >> tmp;
                }
                fin >> tmp; // utime
                totalUser = atoll(tmp.c_str());

                fin >> tmp; // stime
                totalUserLow = atoll(tmp.c_str());

                fin >> tmp; // cutime
                totalSys = atoll(tmp.c_str());

                fin >> tmp; // cstime
                totalIdle = atoll(tmp.c_str());
            }
            fin.close();
        }
        total = totalUser + totalUserLow + totalSys + totalIdle;
        return total;
    }

    unsigned long long GetRamTotal()
    {
        std::ifstream meminfo("/proc/meminfo");
        std::string line;

        unsigned long long memTotal = 0;

        if (meminfo.is_open())
        {
            while (std::getline(meminfo, line))
            {
                if (line.find("MemTotal") != std::string::npos)
                {
                    std::stringstream ss(line);
                    std::string value;
                    ss >> value;
                    ss >> memTotal;
                    break;
                }
            }
            meminfo.close();
        }
        return memTotal;
    }

    unsigned long long GetRamAvailable()
    {
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        unsigned long long memAvailable = 0;
        if (meminfo.is_open())
        {
            while (std::getline(meminfo, line))
            {
                if (line.find("MemAvailable") != std::string::npos)
                {
                    std::stringstream ss(line);
                    std::string value;
                    ss >> value;
                    ss >> memAvailable;
                    break;
                }
            }
            meminfo.close();
        }
        return memAvailable;
    }

    // How many ram occupied by process
    unsigned long long GetRamOccupied(unsigned int pid)
    {
        unsigned long long result = 0;
        if (pid == 0)
        {
            auto memTotal = system_metrics::GetRamTotal();
            auto memAvail = system_metrics::GetRamAvailable();
            result = memTotal - memAvail;
        }
        else
        {
            unsigned long long ramOccupied;
            std::ifstream fin("/proc/" + std::to_string(pid) + "/status");
            if (fin.is_open())
            {
                std::string tmp;
                while (!fin.eof())
                {
                    fin >> tmp;
                    if (tmp != "VmRSS:")
                        continue;

                    fin >> tmp;
                    ramOccupied = atoll(tmp.c_str());
                    break;
                }
                fin.close();
            }
            result = ramOccupied;
        }
        return result;
    }

    std::string GetActiveNetInterface()
    {
        std::string cmd = "ip addr | awk '/state UP/ {print $2}' | sed 's/.$//'";
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        boost::trim_all(result);
        return result;
    }

    std::pair<unsigned long long, unsigned long long> ParseNetData(unsigned int pid)
{
    std::pair<unsigned long long, unsigned long long> result{0, 0};
    std::string path = "/proc";
    if (pid == 0)
    {
        path += "/net/dev";
    }
    else
    {
        path += "/" + std::to_string(pid) + "/net/dev";
    }

    std::ifstream fin(path);
    if (fin.is_open())
    {
        std::string tmp;
        while (fin >> tmp)
        {
            // Just skip useless lines
            if (tmp.find_first_of(":") != std::string::npos)
            {
                tmp.erase(tmp.length() - 1, 1);

                if (tmp != system_metrics::GetActiveNetInterface())
                    continue;

                // tmp contains name of the device

                fin >> tmp;
                result.first += static_cast<unsigned long long>(atoll(tmp.c_str()));

                for (int i = 0; i < 7; i++)
                {
                    fin >> tmp;
                }

                fin >> tmp;
                result.second += static_cast<unsigned long long>(atoll(tmp.c_str()));
            }
        }
    }
    return result;
}

    std::pair<unsigned long long, unsigned long long> ParseIoStats(unsigned int pid)
    {
        std::pair<unsigned long long, unsigned long long> result;

        if (pid == 0)
        {
            // Get general io stats
            boost::filesystem::path path("/sys/block/");
            std::vector<std::string> block_devices;
            for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {}))
            {
                if (entry.path().filename().string().find("loop") != std::string::npos)
                {
                    continue;
                }
                block_devices.push_back(entry.path().filename().string());
            }

            for (auto &block_device : block_devices)
            {
                boost::filesystem::path tmp_path(path.string() + "/" + block_device + "/stat");
                if (boost::filesystem::is_regular_file(tmp_path))
                {
                    std::ifstream fin(tmp_path.string());
                    if (fin.is_open())
                    {
                        // field 3 - read sectors
                        // field 7 - write sectors
                        std::string tmp;
                        for (int i = 0; i < 2; i++)
                        {
                            fin >> tmp;
                        }
                        fin >> tmp;
                        result.first += atoll(tmp.c_str());

                        for (int i = 0; i < 3; i++)
                        {
                            fin >> tmp;
                        }
                        fin >> tmp;
                        result.second += atoll(tmp.c_str());
                        fin.close();
                    }
                }
            }

            result.second *= 512;
            result.first *= 512;

            result.second /= 1024;
            result.first /= 1024;
        }

        else
        {
            boost::filesystem::path tmp_path("/proc/" + std::to_string(pid) + "/io");
            if (boost::filesystem::is_regular_file(tmp_path))
            {
                std::ifstream fin(tmp_path.string().c_str());
                if (fin.is_open())
                {
                    std::string tmp;
                    fin >> tmp; // rchar:
                    fin >> tmp;
                    result.first += atoll(tmp.c_str());

                    fin >> tmp; // wchar:
                    fin >> tmp;
                    result.second += atoll(tmp.c_str());
                }
            }
            // add children branch
            // sum their values
            result.first /= 1024;
            result.second /= 1024;
        }
        return result;
    }

}

Rb_metrics::~Rb_metrics()
{
}

double Rb_metrics::GetGeneralCpuUsage()
{
    double result = getCpuUsage();
    return result;
}

double Rb_metrics::GetCpuUsage()
{
    double result = getCpuUsage(m_pid);
    return result;
}

double Rb_metrics::GetGeneralRamUsage()
{
    double result = getRamUsage();
    return result;
}

double Rb_metrics::GetRamUsage()
{
    double result = getRamUsage(m_pid);
    return result;
}

unsigned long Rb_metrics::GetGeneralRamUsage_m()
{
    unsigned long result = getRamUsage_m();
    return result;
}

unsigned long Rb_metrics::GetRamUsage_m()
{
    unsigned long result = getRamUsage_m(m_pid);
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::GetGeneralNetUsage()
{
    auto result = getNetUsage();
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::GetNetUsage()
{
    auto result = getNetUsage(m_pid);
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::GetGeneralIoStats()
{
    auto result = getIoStats();
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::GetIoStats()
{
    auto result = getIoStats(m_pid);
    return result;
}

double Rb_metrics::getCpuUsage(unsigned int pid) // = 0
{
    double result = 0.00;
    if (pid == 0)
    {
        unsigned long long last_totalUser, last_totalUserLow, last_totalSys, last_totalIdle, last_total;
        std::ifstream fin("/proc/stat");
        if (fin.is_open())
        {
            std::string tmp;
            fin >> tmp; // cpu
            fin >> last_totalUser;
            fin >> last_totalUserLow;
            fin >> last_totalSys;
            fin >> last_totalIdle;
            fin.close();
        }

        std::this_thread::sleep_for(std::chrono::seconds(m_period));

        unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
        fin.open("/proc/stat");
        if (fin.is_open())
        {
            std::string tmp;
            fin >> tmp;
            fin >> totalUser;
            fin >> totalUserLow;
            fin >> totalSys;
            fin >> totalIdle;
            fin.close();
        }

        if (totalUser < last_totalUser || totalUserLow < last_totalUserLow ||
            totalSys < last_totalSys || totalIdle < last_totalIdle)
        {
            // Overflow detection. Just skip this value.
            result = -1.0;
            return result;
        }
        total = (totalUser - last_totalUser) + (totalUserLow - last_totalUserLow) +
                (totalSys - last_totalSys);
        result = total;
        total += (totalIdle - last_totalIdle);
        result *= 100;
        result /= total;
    }

    else
    {
        auto totalTime1 = system_metrics::GetCpuSnapshot();
        auto procTime1 = system_metrics::GetCpuSnapshot(pid);
        // plus kid times

        std::this_thread::sleep_for(std::chrono::seconds(m_period));

        auto totalTime2 = system_metrics::GetCpuSnapshot();
        auto procTime2 = system_metrics::GetCpuSnapshot(pid);
        // plus kid times

        result += 100 * (procTime2 - procTime1);
        result /= (totalTime2 - totalTime1);
    }
    return result;
}

double Rb_metrics::getRamUsage(unsigned int pid) // = 0
{
    double result = 0;
    std::this_thread::sleep_for(std::chrono::seconds(m_period));
    auto totalRam = system_metrics::GetRamTotal();
    unsigned long long occRam = 0;
    if (pid == 0)
    {
        occRam = system_metrics::GetRamOccupied();
    }
    else
    {
        occRam = system_metrics::GetRamOccupied(pid);
    }
    result += (occRam * 100);
    result /= totalRam;
    return result;
}

unsigned long Rb_metrics::getRamUsage_m(unsigned int pid) // = 0
{
    unsigned long result = 0;
    std::this_thread::sleep_for(std::chrono::seconds(m_period));
    if (pid == 0)
    {
        result = system_metrics::GetRamOccupied();
    }
    else
    {
        result = system_metrics::GetRamOccupied(pid);
    }
    result /= 1024;
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::getNetUsage(unsigned int pid) // = 0
{
    std::pair<unsigned long, unsigned long> result{0, 0};
    if (pid == 0)
    {
        auto tmp = system_metrics::ParseNetData();
        std::this_thread::sleep_for(std::chrono::seconds(m_period));
        result = system_metrics::ParseNetData();
        result.first -= tmp.first;
        result.second -= tmp.second;
    }
    else
    {
        auto tmp = system_metrics::ParseNetData(pid);
        // Collect children data
        for (auto kid : m_children_pids)
        {
            auto inner_tmp = system_metrics::ParseNetData(kid);
            tmp.first += inner_tmp.first;
            tmp.second += inner_tmp.second;
        }

        std::this_thread::sleep_for(std::chrono::seconds(m_period));

        result = system_metrics::ParseNetData(pid);
        for (auto kid : m_children_pids)
        {
            auto inner_tmp = system_metrics::ParseNetData(kid);
            result.first += inner_tmp.first;
            result.second += inner_tmp.second;
        }

        result.first -= tmp.first;
        result.second -= tmp.second;
    }
    result.first /= 1024;
    result.second /= 1024;
    return result;
}

std::pair<unsigned long long, unsigned long long> Rb_metrics::getIoStats(unsigned int pid) // = 0
{
    std::pair<unsigned long, unsigned long> result{0, 0};
    if (pid == 0)
    {
        auto tmp = system_metrics::ParseIoStats();

        std::this_thread::sleep_for(std::chrono::seconds(m_period));

        result = system_metrics::ParseIoStats();
        result.first -= tmp.first;
        result.second -= tmp.second;
    }
    else
    {
        auto tmp = system_metrics::ParseIoStats(pid);
        for (auto kid : m_children_pids)
        {
            auto inner_tmp = system_metrics::ParseIoStats(kid);
            tmp.first += inner_tmp.first;
            tmp.second += inner_tmp.second;
        }

        std::this_thread::sleep_for(std::chrono::seconds(m_period));
        result = system_metrics::ParseIoStats(pid);
        for (auto kid : m_children_pids)
        {
            auto inner_tmp = system_metrics::ParseIoStats(kid);
            result.first += inner_tmp.first;
            result.second += inner_tmp.second;
        }

        result.first -= tmp.first;
        result.second -= tmp.second;
    }

    return result;
}