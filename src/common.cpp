#include "common.hpp"

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
                fin >> tmp; //utime
                totalUser = atoll(tmp.c_str());

                fin >> tmp; //stime
                totalUserLow = atoll(tmp.c_str());

                fin >> tmp; //cutime
                totalSys = atoll(tmp.c_str());

                fin >> tmp; //cstime
                totalIdle = atoll(tmp.c_str());
            }
            fin.close();
        }
        total = totalUser + totalUserLow + totalSys + totalIdle;
        return total;
    }
}