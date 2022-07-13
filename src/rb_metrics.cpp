#include "rb_metrics.hpp"

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
}

double Rb_metrics::GetRamUsage()
{
}

unsigned long Rb_metrics::GetGeneralRamUsage_m()
{
}

unsigned long Rb_metrics::GetRamUsage_m()
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::GetGeneralNetUsage()
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::GetNetUsage()
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::GetGeneralIoStats()
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::GetIoStats()
{
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
        auto procTime1 = system_metrics::GetCpuSnapshot(m_pid);
        // plus kid times


        std::this_thread::sleep_for(std::chrono::seconds(m_period));

        auto totalTime2 = system_metrics::GetCpuSnapshot();
        auto procTime2 = system_metrics::GetCpuSnapshot(m_pid);
        //plus kid times

        result += 100 * (procTime2 - procTime1);
        result /= (totalTime2 - totalTime1);
    }
    return result;
}

double Rb_metrics::getRamUsage(unsigned int pid) // = 0
{
}

double Rb_metrics::getRamUsage_m(unsigned int pid) // = 0
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::getNetUsage(unsigned int pid) // = 0
{
}

std::pair<unsigned long, unsigned long> Rb_metrics::getIoStats(unsigned int pid) // = 0
{
}