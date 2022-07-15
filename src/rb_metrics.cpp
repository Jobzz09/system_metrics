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

uint32_t Rb_metrics::GetGeneralCpuUsage()
{
    return getCpuUsage();
}

uint32_t Rb_metrics::GetCpuUsage()
{
    return getCpuUsage(m_pid);
}

uint32_t Rb_metrics::GetGeneralRamUsage()
{
    return getRamUsage();
}

uint32_t Rb_metrics::GetRamUsage()
{
    return getRamUsage(m_pid);
}

uint32_t Rb_metrics::GetGeneralRamUsage_m()
{
    return getRamUsage_m();
}

uint32_t Rb_metrics::GetRamUsage_m()
{
    return getRamUsage_m(m_pid);
}

std::pair<uint64_t, uint64_t> Rb_metrics::GetGeneralNetUsage()
{
    return getNetUsage();
}

std::pair<uint64_t, uint64_t> Rb_metrics::GetNetUsage()
{
    return getNetUsage(m_pid);
}

std::pair<uint64_t, uint64_t> Rb_metrics::GetGeneralIoStats()
{
    return getIoStats();
}

std::pair<uint64_t, uint64_t> Rb_metrics::GetIoStats()
{
    return getIoStats(m_pid);
}

// Common functions
namespace system_metrics
{
    // Returns the list of system's active network interfaces
    std::vector<std::string> GetActiveNetInterfaces();

    // Returns current total CPU times
    uint32_t GetCpuSnapshot(unsigned int pid = 0);

    // Returns total amount of RAM
    uint32_t GetRamTotal();

    // Returns amount occupied of RAM
    uint32_t GetRamOccupied(unsigned int pid = 0);

    // Returns network using satistics (read, write) in kb
    std::pair<uint64_t, uint64_t> ParseNetData(unsigned int pid = 0);

    // Returns block devices using satistics (read, write) in kb
    std::pair<uint64_t, uint64_t> ParseIoStats(unsigned int pid = 0);

    // Returns block device's sector size
    uint32_t GetBlockDeviceSectorSize(std::string block_device);
}

// Returns parent process id of the provided parent process
uint32_t GetPpid(uint32_t pid)
{
    /*
    /proc/[pid]/stat
        .
        .
        .
        (4) ppid  %d
            The PID of the parent of this process.
        .
        .
        .
    */
    uint32_t result = 0;
    std::ifstream fin("/proc/" + std::to_string(pid) + "/stat");
    if (fin.is_open())
    {
        std::string tmp;
        for (int i = 0; i < 3; i++)
        {
            fin >> tmp;
        }
        fin >> result;
        fin.close();
    }
    return result;
}

// Returns true is <data> is number. False otherwise
bool IsNumber(const std::string &data)
{
    std::string::const_iterator it = data.begin();
    while (it != data.end() && std::isdigit(*it))
        ++it;
    return !data.empty() && it == data.end();
}

// Executes terminal command and returns the result.
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

namespace system_metrics
{
    uint32_t GetCpuSnapshot(uint32_t pid)
    {
        /*
        /proc/stat
              kernel/system statistics.  Varies with architecture.
              Common entries include:

              cpu 10132153 290696 3084719 46828483 16683 0 25195 0
              175628 0
              cpu0 1393280 32966 572056 13343292 6130 0 17875 0 23933 0
                     The amount of time, measured in units of USER_HZ
                     (1/100ths of a second on most architectures, use
                     sysconf(_SC_CLK_TCK) to obtain the right value),
                     that the system ("cpu" line) or the specific CPU
                     ("cpuN" line) spent in various states:

                     user   (1) Time spent in user mode.

                     nice   (2) Time spent in user mode with low
                            priority (nice).

                     system (3) Time spent in system mode.

                     idle   (4) Time spent in the idle task.  This value
                            should be USER_HZ times the second entry in
                            the /proc/uptime pseudo-file.
        */

        std::string path = "/proc";
        if (pid == 0)
        {
            path += "/stat";
        }
        else
        {
            path += "/" + std::to_string(pid) + "/stat";
        }
        uint32_t totalUser = 0, totalUserLow = 0, totalSys = 0, totalIdle = 0, total = 0;
        std::ifstream fin(path);
        if (fin.is_open())
        {
            if (pid == 0)
            {
                std::string tmp;
                fin >> tmp; // cpu

                fin >> totalUser;

                fin >> totalUserLow;

                fin >> totalSys;

                fin >> totalIdle;
            }
            else
            {
                /*
                /proc/[pid]/stat
                    Status information about the process.  This is used by
                    ps(1).  It is defined in the kernel source file
                    fs/proc/array.c.

                    The fields, in order, with their proper scanf(3) format
                    specifiers, are listed below.  Whether or not certain of
                    these fields display valid information is governed by a
                    ptrace access mode PTRACE_MODE_READ_FSCREDS |
                    PTRACE_MODE_NOAUDIT check (refer to ptrace(2)).  If the
                    check denies access, then the field value is displayed as
                    0.  The affected fields are indicated with the marking
                    [PT].
                        .
                        .
                        .
                     (14) utime  %lu
                     Amount of time that this process has been scheduled
                     in user mode, measured in clock ticks (divide by
                     sysconf(_SC_CLK_TCK)).  This includes guest time,
                     guest_time (time spent running a virtual CPU, see
                     below), so that applications that are not aware of
                     the guest time field do not lose that time from
                     their calculations.

                    (15) stime  %lu
                            Amount of time that this process has been scheduled
                            in kernel mode, measured in clock ticks (divide by
                            sysconf(_SC_CLK_TCK)).

                    (16) cutime  %ld
                            Amount of time that this process's waited-for
                            children have been scheduled in user mode, measured
                            in clock ticks (divide by sysconf(_SC_CLK_TCK)).
                            (See also times(2).)  This includes guest time,
                            cguest_time (time spent running a virtual CPU, see
                            below).

                    (17) cstime  %ld
                            Amount of time that this process's waited-for
                            children have been scheduled in kernel mode,
                            measured in clock ticks (divide by
                            sysconf(_SC_CLK_TCK)).
                            .
                            .
                            .
                */
                std::string tmp;
                for (int i = 0; i < 13; i++)
                {
                    fin >> tmp;
                }
                fin >> totalUser; // utime

                fin >> totalUserLow; // stime

                fin >> totalSys; // cutime

                fin >> totalIdle; // cstime
            }
            fin.close();
        }
        total = totalUser + totalUserLow + totalSys + totalIdle;
        return total;
    }

    uint32_t GetRamTotal()
    {
        /*
        /proc/meminfo
              This file reports statistics about memory usage on the
              system.  It is used by free(1) to report the amount of
              free and used memory (both physical and swap) on the
              system as well as the shared memory and buffers used by
              the kernel.  Each line of the file consists of a parameter
              name, followed by a colon, the value of the parameter, and
              an option unit of measurement (e.g., "kB").  The list
              below describes the parameter names and the format
              specifier required to read the field value.  Except as
              noted below, all of the fields have been present since at
              least Linux 2.6.0.  Some fields are displayed only if the
              kernel was configured with various options; those
              dependencies are noted in the list.

              MemTotal %lu
                     Total usable RAM (i.e., physical RAM minus a few
                     reserved bits and the kernel binary code).
        */
        std::ifstream meminfo("/proc/meminfo");
        std::string line;

        uint32_t memTotal = 0;

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
        /*
        /proc/meminfo
              This file reports statistics about memory usage on the
              system.  It is used by free(1) to report the amount of
              free and used memory (both physical and swap) on the
              system as well as the shared memory and buffers used by
              the kernel.  Each line of the file consists of a parameter
              name, followed by a colon, the value of the parameter, and
              an option unit of measurement (e.g., "kB").  The list
              below describes the parameter names and the format
              specifier required to read the field value.  Except as
              noted below, all of the fields have been present since at
              least Linux 2.6.0.  Some fields are displayed only if the
              kernel was configured with various options; those
              dependencies are noted in the list.

              MemAvailable %lu (since Linux 3.14)
                     An estimate of how much memory is available for
                     starting new applications, without swapping.
        */
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
    uint32_t GetRamOccupied(unsigned int pid)
    {
        uint32_t result = 0;
        if (pid == 0)
        {
            auto memTotal = system_metrics::GetRamTotal();
            auto memAvail = system_metrics::GetRamAvailable();
            result = memTotal - memAvail;
        }
        else
        {
            /*
            /proc/[pid]/status
              Provides much of the information in /proc/[pid]/stat and
              /proc/[pid]/statm in a format that's easier for humans to
              parse.

                VmRSS  Resident set size.  Note that the value here is the
                     sum of RssAnon, RssFile, and RssShmem.

                RssAnon
                     Size of resident anonymous memory.

                RssFile
                     Size of resident file mappings.

                RssShmem
                     Size of resident shared memory (includes System V
                     shared memory, mappings from tmpfs(5), and shared
                     anonymous mappings)
            */
            uint32_t ramOccupied;
            std::ifstream fin("/proc/" + std::to_string(pid) + "/status");
            if (fin.is_open())
            {
                std::string tmp;
                while (!fin.eof())
                {
                    fin >> tmp;
                    if (tmp != "VmRSS:")
                        continue;

                    fin >> ramOccupied;
                    break;
                }
                fin.close();
            }
            result = ramOccupied;
        }
        return result;
    }

    std::vector<std::string> GetActiveNetInterfaces()
    {
        std::vector<std::string> result;
        std::string cmd = "ip addr | awk '/state UP/ {print $2}' | sed 's/.$//'";
        std::string data = ExecCommandWithResult(cmd.c_str());
        boost::replace_all(data, "\n", " ");
        boost::trim_all(data);
        boost::algorithm::split(result, data, boost::algorithm::is_any_of(" "), boost::token_compress_on);
        return result;
    }

    std::pair<uint64_t, uint64_t> ParseNetData(uint32_t pid)
    {
        /*
        /proc/net/dev
        /proc/[pid]/net/dev

              The dev pseudo-file contains network device status
              information.  This gives the number of received and sent
              packets, the number of errors and collisions and other
              basic statistics.  These are used by the ifconfig(8)
              program to report device status.  The format is:

              Inter-|   Receive                                                |  Transmit
               face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
                  lo: 2776770   11307    0    0    0     0          0         0  2776770   11307    0    0    0     0       0          0
                eth0: 1215645    2751    0    0    0     0          0         0  1782404    4324    0    0    0   427       0          0
                ppp0: 1622270    5552    1    0    0     0          0         0   354130    5669    0    0    0     0       0          0
                tap0:    7714      81    0    0    0     0          0         0     7714      81    0    0    0     0       0          0

        */
        std::pair<uint64_t, uint64_t> result{0, 0};
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

                    bool is_found = false;
                    for (auto net_if : system_metrics::GetActiveNetInterfaces())
                    {
                        if (tmp == net_if)
                        {
                            is_found = true;
                        }
                    }
                    if (!is_found)
                        continue;

                    // tmp contains name of the device

                    fin >> tmp;
                    result.first += atol(tmp.c_str());

                    for (int i = 0; i < 7; i++)
                    {
                        fin >> tmp;
                    }

                    fin >> tmp;
                    result.second += atol(tmp.c_str());
                }
            }
        }
        return result;
    }

    std::pair<uint64_t, uint64_t> ParseIoStats(uint32_t pid)
    {
        std::pair<uint64_t, uint64_t> result;

        // Get general stats
        if (pid == 0)
        {
            /* In order to get info about general number of IO operations,
               we have to parse this data in /sys/block/[block_device_name]/stat
               for each active block device in the system

            This file documents the contents of the /sys/block/<dev>/stat file.

            The stat file provides several statistics about the state of block
            device <dev>.
            Name            units         description
            ----            -----         -----------
            read I/Os       requests      number of read I/Os processed
            read merges     requests      number of read I/Os merged with in-queue I/O
        *---read sectors    sectors       number of sectors read
            read ticks      milliseconds  total wait time for read requests
            write I/Os      requests      number of write I/Os processed
            write merges    requests      number of write I/Os merged with in-queue I/O
        *---write sectors   sectors       number of sectors written
            write ticks     milliseconds  total wait time for write requests
            in_flight       requests      number of I/Os currently in flight
            io_ticks        milliseconds  total time this block device has been active
            time_in_queue   milliseconds  total wait time for all requests
            discard I/Os    requests      number of discard I/Os processed
            discard merges  requests      number of discard I/Os merged with in-queue I/O
            discard sectors sectors       number of sectors discarded
            discard ticks   milliseconds  total wait time for discard requests
            */

            boost::filesystem::path path("/sys/block/");
            std::vector<std::string> block_devices;
            for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {}))
            {
                // Skip every block device that contains 'loop' in its name
                if (entry.path().filename().string().find("loop") != std::string::npos)
                {
                    continue;
                }
                block_devices.push_back(entry.path().filename().string());
            }

            // Iterate over each block device and collect its data
            for (auto &block_device : block_devices)
            {
                boost::filesystem::path tmp_path(path.string() + "/" + block_device + "/stat");
                if (boost::filesystem::is_regular_file(tmp_path))
                {
                    std::ifstream fin(tmp_path.string());
                    if (fin.is_open())
                    {

                        std::string tmp;
                        uint64_t readed = 0, written = 0;
                        for (int i = 0; i < 2; i++)
                        {
                            fin >> tmp;
                        }

                        // field 3 - read sectors
                        fin >> readed;

                        for (int i = 0; i < 3; i++)
                        {
                            fin >> tmp;
                        }

                        // field 7 - write sectors
                        fin >> written;

                        fin.close();

                        // Sector may be differ for each block device, so we collect only byte amount in result
                        auto sector_size = system_metrics::GetBlockDeviceSectorSize(block_device);
                        result.first += (readed * sector_size);
                        result.second += (written * sector_size);
                    }
                }
            }

            // Result now is in bytes. We need it in kilobytes
            result.second /= 1024;
            result.first /= 1024;
        }

        else
        {
            /*
            /proc/[pid]/io (since kernel 2.6.20)
              This file contains I/O statistics for the process, for
              example:

                  # cat /proc/3828/io
                  rchar: 323934931
                  wchar: 323929600
                  syscr: 632687
                  syscw: 632675
                  read_bytes: 0
                  write_bytes: 323932160
                  cancelled_write_bytes: 0

              The fields are as follows:

              rchar: characters read
                     The number of bytes which this task has caused to
                     be read from storage.  This is simply the sum of
                     bytes which this process passed to read(2) and
                     similar system calls.  It includes things such as
                     terminal I/O and is unaffected by whether or not
                     actual physical disk I/O was required (the read
                     might have been satisfied from pagecache).

              wchar: characters written
                     The number of bytes which this task has caused, or
                     shall cause to be written to disk.  Similar caveats
                     apply here as with rchar.

                    ...
            */
            boost::filesystem::path tmp_path("/proc/" + std::to_string(pid) + "/io");
            if (boost::filesystem::is_regular_file(tmp_path))
            {
                std::ifstream fin(tmp_path.string().c_str());
                if (fin.is_open())
                {
                    std::string tmp;
                    fin >> tmp;

                    // rchar:
                    fin >> tmp;
                    result.first += atoll(tmp.c_str());

                    fin >> tmp;

                    // wchar:
                    fin >> tmp;
                    result.second += atoll(tmp.c_str());
                }
            }
            // Result is in bytes. We need kilobytes
            result.first /= 1024;
            result.second /= 1024;
        }
        return result;
    }

    uint32_t GetBlockDeviceSectorSize(std::string block_device)
    {
        /*
                =================
                Queue sysfs files
                =================
                This text file will detail the queue files that are located in the sysfs tree
                for each block device. Note that stacked devices typically do not export
                any settings, since their queue merely functions as a remapping target.
                These files are the ones found in the /sys/block/xxx/queue/ directory.
                ...

                hw_sector_size (RO)
                -------------------
                This is the hardware sector size of the device,
                in bytes.

                ...
        */
        uint32_t size;
        std::ifstream fin("/sys/block/" + block_device + "/queue/hw_sector_size");
        if (fin.is_open())
        {
            fin >> size;
            fin.close();
        }
        return size;
    }
}

uint32_t Rb_metrics::getCpuUsage(unsigned int pid) // = 0
{
    uint32_t result = 0.00;
    if (pid == 0)
    {
        uint32_t last_totalUser, last_totalUserLow, last_totalSys, last_totalIdle, last_total;
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

        uint32_t totalUser, totalUserLow, totalSys, totalIdle, total;
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
            result = 0;
            return result;
        }
        total = (totalUser - last_totalUser) + (totalUserLow - last_totalUserLow) +
                (totalSys - last_totalSys);
        result = total;

        // We cannot use GetCpuSnapshot() because of this line
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

uint32_t Rb_metrics::getRamUsage(unsigned int pid) // = 0
{
    uint32_t result = 0;
    auto totalRam = system_metrics::GetRamTotal();
    unsigned long long occRam = 0;
    occRam = system_metrics::GetRamOccupied(pid);
    if (pid != 0)
    {
        for (auto child : m_children_pids)
        {
            occRam += system_metrics::GetRamOccupied(child);
        }
    }
    result += (occRam * 100);
    result /= totalRam;
    return result;
}

uint32_t Rb_metrics::getRamUsage_m(uint32_t pid) // = 0
{
    uint32_t result = 0;
    result = system_metrics::GetRamOccupied(pid);
    if (pid != 0)
    {
        for (auto child : m_children_pids)
        {
            result += system_metrics::GetRamOccupied(child);
        }
    }
    // Result is in kilobytes. Return in megabytes.
    result /= 1024;
    return result;
}

std::pair<uint64_t, uint64_t> Rb_metrics::getNetUsage(uint32_t pid) // = 0
{
    std::pair<uint64_t, uint64_t> result{0, 0};
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
    // Parsed data is in bytes/sec. We need kb/sec.
    result.first /= 1024;
    result.second /= 1024;
    return result;
}

std::pair<uint64_t, uint64_t> Rb_metrics::getIoStats(uint32_t pid) // = 0
{
    std::pair<uint64_t, uint64_t> result{0, 0};
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

std::vector<uint32_t> Rb_metrics::getAllChildren(uint32_t pid)
{
    std::vector<unsigned int> result;
    unsigned proc_count = 0;
    if (boost::filesystem::is_directory("/proc"))
    {
        for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator("/proc"), {}))
        {
            boost::filesystem::path tmp(entry);
            if (IsNumber(tmp.filename().string()))
            {
                uint32_t curr_pid = atoi(tmp.filename().string().c_str());
                uint32_t ppid = GetPpid(curr_pid);
                if (ppid == pid)
                {
                    result.push_back(curr_pid);
                }
            }
        }
    }
    return result;
}