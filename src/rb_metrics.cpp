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

// Common functions

// Returns the ppid of provided pid
uint32_t GetPpid(uint32_t pid)
{
    std::string ppid;

    ProcStatFile procStatFile(pid);
    if (procStatFile.GetLastError() != ProcStatFile::Result::ERROR)
    {
        if (procStatFile.GetEntryValue(ProcStatFile::Entry::PPID, ppid) == ProcStatFile::Result::ERROR)
        {
            // error getting ppid entry value for pid
            //  log << error occured: error cause << endl;
            // log << "Cannot read /proc/" << pid << "/stat file. Error occured" << std::endl;"

            return 0;
        }
    }
    else
    {
        // Error while reading ProcStat file for pid
        // log << error occured << endl;
        return 0;
    }
    return atol(ppid.c_str());
}

// Returns true if <data> is number. False otherwise
bool IsNumber(const std::string &data)
{
    std::string::const_iterator it = data.begin();
    while (it != data.end() && std::isdigit(*it))
        ++it;
    return !data.empty() && it == data.end();
}

// Executes comman and saves result in string
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

// Get list of all active net interfaces
std::vector<std::string> GetActiveNetworkInterfaces()
{
    std::vector<std::string> result;
    std::string cmd = "ip addr | awk '/state UP/ {print $2}' | sed 's/.$//'";
    std::string data = ExecCommandWithResult(cmd.c_str());
    boost::replace_all(data, "\n", " ");
    boost::trim_all(data);
    boost::algorithm::split(result, data, boost::algorithm::is_any_of(" "), boost::token_compress_on);
    return result;
}

// Get list of all active block devices
std::vector<std::string> GetActiveBlockDevices()
{
    std::vector<std::string> result;
    boost::filesystem::path path("/sys/block/");
    for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {}))
    {
        // Skip every block device that contains 'loop' in its name
        if (entry.path().filename().string().find("loop") != std::string::npos)
        {
            continue;
        }
        result.push_back(entry.path().filename().string());
    }
    return result;
}

// Returns true if the net interface is active
bool IsNetInterfaceActive(std::string net_interface)
{
    for (auto active_net_interface : GetActiveNetworkInterfaces())
    {
        if (net_interface == active_net_interface)
            return true;
    }
    return false;
}
// Returns the sector size for provided block device
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

// Returns true if provided pid exists
bool ProcessExists(uint32_t pid)
{
    return boost::filesystem::exists("/proc/" + std::to_string(pid) + "/");
}

// Сonvert data in kilobytes to kilobytes
uint32_t ToKB(uint64_t bytes)
{
    return bytes >> 10;
}

// Сonvert data in bytes to megabytes
uint32_t ToMB(uint64_t bytes)
{
    return bytes >> 20;
}

// ===== AbstractFile ======

AbstractFile::Result AbstractFile::GetLastError()
{
    return last_error;
}

AbstractFile::Result AbstractFile::GetEntryValue(Entry entry, std::string &dst_string)
{
    if (static_cast<uint32_t>(entry) > fileContent.size())
    {
        // log << "Attempt to step outside file content"
        return Result::ERROR;
    }
    dst_string = fileContent[static_cast<uint32_t>(entry)];
    return Result::SUCCESS;
};

AbstractFile::Result AbstractFile::ReadFile(std::string &&filePath)
{
    std::ifstream fin(filePath);
    if (fin.is_open())
    {
        std::istream_iterator<std::string> stream_start(fin);
        std::istream_iterator<std::string> stream_end;
        std::copy(stream_start, stream_end, std::back_inserter(fileContent));
        fin.close();
        return Result::SUCCESS;
    }
    return Result::ERROR;
}

// ===== ProcStatFile =====

ProcStatFile::ProcStatFile(uint32_t pid)
{
    std::string filePath;
    if (pid != 0)
    {
        filePath = "/proc/" + std::to_string(pid) + "/stat";
    }
    else
    {
        filePath = "/proc/stat";
    }
    if (ReadFile(std::move(filePath)) == Result::ERROR)
    {
        last_error = Result::ERROR;
    }
}

AbstractFile::Result ProcStatFile::GetEntryValue(Entry entry, std::string &dst_string)
{
    if (static_cast<uint32_t>(entry) > fileContent.size())
    {
        // log << "Attempt to step outside file content"
        return Result::ERROR;
    }
    dst_string = fileContent[static_cast<uint32_t>(entry)];
    return Result::SUCCESS;
}

// ===== ProcStatusFile =====

ProcStatusFile::ProcStatusFile(uint32_t pid)
{
    if (pid == 0)
        return;
    std::string filePath;
    filePath = "/proc/" + std::to_string(pid) + "/status";
    if (ReadFile(std::move(filePath)) == ProcStatusFile::Result::ERROR)
    {
        last_error = Result::ERROR;
        return;
    }
}

AbstractFile::Result ProcStatusFile::GetEntryValue(Entry entry, std::string &dst_string)
{
    if (static_cast<uint32_t>(entry) > fileContent.size())
    {
        // log << "Attempt to step outside file content"
        return Result::ERROR;
    }
    dst_string = fileContent[static_cast<uint32_t>(entry)];
    return Result::SUCCESS;
}

// ===== ProcMemInfoFile ======

ProcMemInfoFile::ProcMemInfoFile(uint32_t pid)
{
    if (pid != 0)
    {
        last_error = Result::ERROR;
        return;
    }

    if (ReadFile("/proc/meminfo") == Result::ERROR)
    {
        last_error = Result::ERROR;
        return;
    }
}

AbstractFile::Result ProcMemInfoFile::GetEntryValue(Entry entry, std::string &dst_string)
{
    if (static_cast<uint32_t>(entry) > fileContent.size())
    {
        // log << "Attempt to step outside file content"
        return Result::ERROR;
    }
    dst_string = fileContent[static_cast<uint32_t>(entry)];
    return Result::SUCCESS;
}

// ===== ProcNetDevFile ======

ProcNetDevFile::ProcNetDevFile(uint32_t pid)
{
    std::string filePath = "/proc";
    if (pid != 0)
    {
        filePath += "/";
        filePath += std::to_string(pid);
    }

    filePath += "/net/dev";
    if (ReadFile(std::move(filePath)) == Result::ERROR)
    {
        last_error = Result::ERROR;
        return;
    }
}

AbstractFile::Result ProcNetDevFile::GetEntryValue(const std::string &interface_name, Entry entry, std::string &dst_string)
{
    if (line_content_for_device.find(interface_name) == line_content_for_device.end())
    {
        // log << "interface " << interface_name << " wasn't found in /proc/net/dev file"
        return Result::ERROR;
    }
    else
    {
        if (static_cast<uint32_t>(entry) > line_content_for_device[interface_name].size())
        {
            // log << "Attempt to step outside file content"
            return Result::ERROR;
        }
        dst_string = line_content_for_device[interface_name][static_cast<uint32_t>(entry)];
        return Result::SUCCESS;
    }
}

AbstractFile::Result ProcNetDevFile::ReadFile(std::string &&filePath)
{
    std::ifstream fin(filePath);
    std::string skip;
    if (fin.is_open())
    {
        while (getline(fin, skip))
        {
            if (skip.find(":") != std::string::npos)
            {
                std::stringstream ss(skip);
                std::string interface_name;

                ss >> interface_name;
                interface_name.erase(interface_name.length() - 1, 1); // remove ':' from the end
                if (!IsNetInterfaceActive(interface_name))
                {
                    continue;
                }

                std::vector<std::string> line_content;

                ss.str(skip); // reload stringstream
                std::istream_iterator<std::string> stream_start(ss);
                std::istream_iterator<std::string> stream_end;
                std::copy(stream_start, stream_end, std::back_inserter(line_content));

                line_content_for_device.emplace(interface_name, line_content);
            }
        }
        fin.close();
        return Result::SUCCESS;
    }
    last_error = Result::ERROR;
    return Result::ERROR;
}

// ===== SysBlockStatFile =====

SysBlockStatFile::SysBlockStatFile(uint32_t pid)
{
    if (pid != 0)
    {
        return;
    }

    std::string filePath = "/sys/block/";
    for (auto active_block_device : GetActiveBlockDevices())
    {
        filePath += active_block_device + "/stat";
        if (ReadFile(std::move(filePath)) == Result::ERROR)
        {
            last_error = Result::ERROR;
            return;
        }
        line_content_for_device.emplace(active_block_device, fileContent);
    }
}

AbstractFile::Result SysBlockStatFile::GetEntryValue(const std::string &block_device, Entry entry, std::string &dst_string)
{
    if (line_content_for_device.find(block_device) == line_content_for_device.end())
    {
        // log << "block_device " << block_device << " wasn't found in /sys/block file"
        return Result::ERROR;
    }
    else
    {
        if (static_cast<uint32_t>(entry) > line_content_for_device[block_device].size())
        {
            // log << "Attempt to step outside file content"
            return Result::ERROR;
        }
        dst_string = line_content_for_device[block_device][static_cast<uint32_t>(entry)];
        return Result::SUCCESS;
    }
}

// ====== ProcPidIoFile ======

ProcPidIoFile::ProcPidIoFile(uint32_t pid)
{
    if (ReadFile("/proc/" + std::to_string(pid) + "/io") == Result::ERROR)
    {
        last_error = Result::ERROR;
        return;
    }
}

AbstractFile::Result ProcPidIoFile::GetEntryValue(Entry entry, std::string &dst_string)
{
    if (static_cast<uint32_t>(entry) > fileContent.size())
    {
        // log << "block_device " << block_device << " wasn't found in /sys/block file"
        return Result::ERROR;
    }
    dst_string = fileContent[static_cast<uint32_t>(entry)];
    return Result::SUCCESS;
}

// ===== Collector =====

cpu_stat Collector::CpuStat(uint32_t pid)
{
    cpu_stat cpu_data{0};
    ProcStatFile procStatFile;
    if (procStatFile.GetLastError() != AbstractFile::Result::ERROR)
    {
        std::string data;
        AbstractFile::Result res = procStatFile.GetEntryValue(ProcStatFile::Entry::USER, data);
        if (res == AbstractFile::Result::ERROR)
        {
            // log << Error while getting entry for pid << std::endl;
            return cpu_stat{0};
        }
        cpu_data.user = atol(data.c_str());

        res = procStatFile.GetEntryValue(ProcStatFile::Entry::NICE, data);
        if (res == AbstractFile::Result::ERROR)
        {
            // log << Error while getting entry for pid << std::endl;
            return cpu_stat{0};
        }
        cpu_data.nice = atol(data.c_str());

        res = procStatFile.GetEntryValue(ProcStatFile::Entry::SYSTEM, data);
        if (res == AbstractFile::Result::ERROR)
        {
            // log << Error while getting entry for pid << std::endl;
            return cpu_stat{0};
        }
        cpu_data.system = atol(data.c_str());

        res = procStatFile.GetEntryValue(ProcStatFile::Entry::IDLE, data);
        if (res == AbstractFile::Result::ERROR)
        {
            // log << Error while getting entry for pid << std::endl;
            return cpu_stat{0};
        }
        cpu_data.idle = atol(data.c_str());

        if (pid != 0)
        {
            ProcStatFile pidProcStatFile(pid);
            if (pidProcStatFile.GetLastError() != AbstractFile::Result::ERROR)
            {
                // file has been readed succesfully

                res = pidProcStatFile.GetEntryValue(ProcStatFile::Entry::UTIME, data);
                if (res == AbstractFile::Result::ERROR)
                {
                    // log << Error while getting entry for pid << std::endl;
                    return cpu_stat{0};
                }
                cpu_data.utime = atol(data.c_str());

                res = pidProcStatFile.GetEntryValue(ProcStatFile::Entry::STIME, data);
                if (res == AbstractFile::Result::ERROR)
                {
                    // log << Error while getting entry for pid << std::endl;
                    return cpu_stat{0};
                }
                cpu_data.stime = atol(data.c_str());

                res = pidProcStatFile.GetEntryValue(ProcStatFile::Entry::CUTIME, data);
                if (res == AbstractFile::Result::ERROR)
                {
                    // log << Error while getting entry for pid << std::endl;
                    return cpu_stat{0};
                }
                cpu_data.cutime = atol(data.c_str());

                res = pidProcStatFile.GetEntryValue(ProcStatFile::Entry::CSTIME, data);
                if (res == AbstractFile::Result::ERROR)
                {
                    // log << Error while getting entry for pid << std::endl;
                    return cpu_stat{0};
                }
                cpu_data.cstime = atol(data.c_str());
            }
            else
            {
                // file hasn't been readed

                //  log << File was not readed << endl;
                return cpu_stat{0};
            }
        }
    }
    else
    {
        // file hasn't been readed

        // log << file was not readed << endl;
        return cpu_stat{0};
    }

    return cpu_data;
}

ram_stat Collector::RamStat(uint32_t pid)
{
    ram_stat ram_data{0};
    ProcMemInfoFile procMemInfoFile;

    if (procMemInfoFile.GetLastError() != AbstractFile::Result::ERROR)
    {
        // file has been readed succesfully
        std::string data;
        AbstractFile::Result result;

        result = procMemInfoFile.GetEntryValue(ProcMemInfoFile::Entry::MEM_AVAILABLE, data);
        if (result == AbstractFile::Result::ERROR)
        {
            // Error getting entry value
            return ram_stat{0};
        }
        ram_data.ram_available = atol(data.c_str());

        result = procMemInfoFile.GetEntryValue(ProcMemInfoFile::Entry::MEM_TOTAL, data);
        if (result == AbstractFile::Result::ERROR)
        {
            // Error getting entry value
            return ram_stat{0};
        }
        ram_data.ram_total = atol(data.c_str());

        if (pid != 0)
        {
            ProcStatusFile procStatusFile(pid);

            if (procStatusFile.GetLastError() != AbstractFile::Result::ERROR)
            {
                // File has been readed
                result = procStatusFile.GetEntryValue(ProcStatusFile::Entry::VMRSS, data);
                if (result == AbstractFile::Result::ERROR)
                {
                    // Error getting entry value
                    return ram_stat{0};
                }
                ram_data.vmrss = atol(data.c_str());

                ram_data.pid = pid;
            }
            else
            {
                // file hasn't been readed
                // Log << "Error while reading procStatusFile for pid" << pid << endl;
                return ram_stat{0};
            }
        }
    }
    else
    {
        // file hasn't been readed
        // Log << "Error while reading procStatusFile for pid" << pid << endl;
        return ram_stat{0};
    }

    return ram_data;
}

net_stat Collector::NetStat(uint32_t pid)
{
    net_stat net_data{0};
    for (auto active_net_interface : GetActiveNetworkInterfaces())
    {
        ProcNetDevFile procNetDevFile(pid);
        if (procNetDevFile.GetLastError() != AbstractFile::Result::ERROR)
        {
            // File has been readed successfully
            AbstractFile::Result result;
            std::string data;

            result = procNetDevFile.GetEntryValue(active_net_interface, ProcNetDevFile::Entry::RECV_BYTES, data);
            if (result == AbstractFile::Result::ERROR)
            {
                // Error getting entry value
                return net_stat{0};
            }
            net_data.r_bytes += atoll(data.c_str());

            result = procNetDevFile.GetEntryValue(active_net_interface, ProcNetDevFile::Entry::WRITE_BYTES, data);
            if (result == AbstractFile::Result::ERROR)
            {
                // Error getting entry value
                return net_stat{0};
            }
            net_data.w_bytes += atoll(data.c_str());
        }
    }
    if (pid != 0)
    {
        net_data.pid = pid;
    }
    return net_data;
}

bd_stat Collector::BdStat(uint32_t pid)
{
    bd_stat bd_data{0};
    if (pid == 0)
    {
        SysBlockStatFile sysBlockStatFile;
        if (sysBlockStatFile.GetLastError() != AbstractFile::Result::ERROR)
        {
            AbstractFile::Result result;
            std::string data;
            for (auto active_bd : GetActiveBlockDevices())
            {
                result = sysBlockStatFile.GetEntryValue(active_bd, SysBlockStatFile::Entry::READ_SECTORS, data);
                if (result == AbstractFile::Result::ERROR)
                {
                    // Error getting entry value
                    return bd_stat{0};
                }
                bd_data.r_bytes += atoll(data.c_str());

                result = sysBlockStatFile.GetEntryValue(active_bd, SysBlockStatFile::Entry::WRITE_SECTORS, data);
                if (result == AbstractFile::Result::ERROR)
                {
                    // Error getting entry value
                    // log << error cause << endl;
                    return bd_stat{0};
                }
                bd_data.w_bytes += atoll(data.c_str());

                bd_data.r_bytes *= GetBlockDeviceSectorSize(active_bd);
                bd_data.w_bytes *= GetBlockDeviceSectorSize(active_bd);
            }
        }
        else
        {
            // Error while reading file
            // log << error cause << endl;
            return bd_stat{0};
        }
    }

    else
    {
        ProcPidIoFile procPidIoFile(pid);
        if (procPidIoFile.GetLastError() != AbstractFile::Result::ERROR)
        {
            AbstractFile::Result result;
            std::string data;

            result = procPidIoFile.GetEntryValue(ProcPidIoFile::Entry::RCHAR, data);
            if (result == AbstractFile::Result::ERROR)
            {
                // Error getting entry value
                // log << error cause << endl;
                return bd_stat{0};
            }
            bd_data.r_bytes += atoll(data.c_str());

            result = procPidIoFile.GetEntryValue(ProcPidIoFile::Entry::WCHAR, data);
            if (result == AbstractFile::Result::ERROR)
            {
                // Error getting entry value
                // log << error cause << endl;
                return bd_stat{0};
            }
            bd_data.w_bytes += atoll(data.c_str());
            bd_data.pid = pid;
        }
        else
        {
            // Error while reading file
            // log << error cause << endl;
            return bd_stat{0};
        }
    }
    return bd_data;
}

// ===== Calculator =====

uint32_t Calculator::GeneralCpuUsage(cpu_stat early_cpu_data, cpu_stat lately_cpu_data)
{
    uint32_t result = 0;
    if (lately_cpu_data.user < early_cpu_data.user ||
        lately_cpu_data.nice < early_cpu_data.nice ||
        lately_cpu_data.system < early_cpu_data.system ||
        lately_cpu_data.idle < early_cpu_data.idle)
    {
        // Overflow detection. Just skip this value.
        return result;
    }

    uint64_t total = (lately_cpu_data.user - early_cpu_data.user) +
                     (lately_cpu_data.nice - early_cpu_data.nice) +
                     (lately_cpu_data.system - early_cpu_data.system);
    uint64_t total_without_idle = total;
    total += (lately_cpu_data.idle - early_cpu_data.idle);
    total_without_idle *= 100;
    total_without_idle /= total;
    result = total_without_idle;
    return result;
}

uint32_t Calculator::CpuUsage(cpu_stat early_cpu_data, cpu_stat lately_cpu_data)
{
    uint32_t result = 0;
    uint64_t total_cpu_time_before = early_cpu_data.user + early_cpu_data.nice +
                                     early_cpu_data.system + early_cpu_data.idle;

    uint64_t total_pid_time_before = early_cpu_data.utime + early_cpu_data.stime +
                                     early_cpu_data.cutime + early_cpu_data.cstime;

    uint64_t total_cpu_time_after = lately_cpu_data.user + lately_cpu_data.nice +
                                    lately_cpu_data.system + lately_cpu_data.idle;

    uint64_t total_pid_time_after = lately_cpu_data.utime + lately_cpu_data.stime +
                                    lately_cpu_data.cutime + lately_cpu_data.cstime;

    result = (total_pid_time_after - total_pid_time_before);
    result *= 100;
    result /= (total_cpu_time_after - total_cpu_time_before);
    return result;
}

uint32_t Calculator::RamUsage(ram_stat ram_data)
{
    uint32_t occupied_ram;
    if (ram_data.pid != 0)
    {
        occupied_ram = ram_data.vmrss;
    }
    else
    {
        occupied_ram = ram_data.ram_total - ram_data.ram_available;
    }
    occupied_ram *= 100;
    occupied_ram /= ram_data.ram_total;
    return occupied_ram;
}

uint32_t Calculator::RamUsage_m(ram_stat ram_data)
{
    uint64_t occupied_ram;
    if (ram_data.pid != 0)
    {
        occupied_ram = ram_data.vmrss;
    }
    else
    {
        occupied_ram = ram_data.ram_total - ram_data.ram_available;
    }
    occupied_ram *= 1024; // kbytes -> bytes
    return ToMB(occupied_ram);
}

uint32_t Calculator::NetDownload(net_stat early_net_data, net_stat lately_net_data)
{
    return lately_net_data.r_bytes - early_net_data.r_bytes;
}

uint32_t Calculator::NetUpload(net_stat early_net_data, net_stat lately_net_data)
{
    return lately_net_data.w_bytes - early_net_data.w_bytes;
}

uint32_t Calculator::BlockDeviceWrite(bd_stat early_bd_data, bd_stat lately_bd_data)
{
    return lately_bd_data.w_bytes - early_bd_data.w_bytes;
}

uint32_t Calculator::BlockDeviceRead(bd_stat early_bd_data, bd_stat lately_bd_data)
{
    return lately_bd_data.r_bytes - early_bd_data.r_bytes;
}

// ===== Rb_metrics =====

uint32_t Rb_metrics::GetGeneralCpuUsage()
{
    return getCpuUsage(0);
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

std::pair<uint64_t, uint64_t> Rb_metrics::GetGeneralIoStat()
{
    return getIostat();
}

std::pair<uint64_t, uint64_t> Rb_metrics::GetIoStat()
{
    return getIostat(m_pid);
}

uint32_t Rb_metrics::getCpuUsage(unsigned int pid) // = 0
{
    if (pid != 0 && !ProcessExists(pid))
    {
        // Log >> <pid> does not exist
        return 0;
    }
    auto early_cpu_data = Collector().CpuStat(pid);
    std::this_thread::sleep_for(std::chrono::seconds(m_period));
    auto lately_cpu_data = Collector().CpuStat(pid);
    if (pid == 0)
    {
        return Calculator().GeneralCpuUsage(early_cpu_data, lately_cpu_data);
    }
    return Calculator().CpuUsage(early_cpu_data, lately_cpu_data);
}

uint32_t Rb_metrics::getRamUsage(unsigned int pid) // = 0
{
    if (pid != 0 && !ProcessExists(pid))
    {
        // Log >> <pid> does not exist
        return 0;
    }
    auto ram_data = Collector().RamStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_ram_data = Collector().RamStat(child);
            ram_data.vmrss += child_ram_data.vmrss;
        }
    }
    return Calculator().RamUsage(ram_data);
}

uint32_t Rb_metrics::getRamUsage_m(uint32_t pid) // = 0
{
    if (pid != 0 && !ProcessExists(pid))
    {
        // Log >> <pid> does not exist
        return 0;
    }
    auto ram_data = Collector().RamStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_ram_data = Collector().RamStat(child);
            ram_data.vmrss += child_ram_data.vmrss;
        }
    }
    return Calculator().RamUsage_m(ram_data);
}

std::pair<uint64_t, uint64_t> Rb_metrics::getNetUsage(uint32_t pid) // = 0
{
    std::pair<uint64_t, uint64_t> result;
    auto early_network_usage_data = Collector().NetStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_network_data = Collector().NetStat(child);
            early_network_usage_data.r_bytes += child_network_data.r_bytes;
            early_network_usage_data.w_bytes += child_network_data.w_bytes;
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(m_period));
    auto lately_network_usage_data = Collector().NetStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_network_data = Collector().NetStat(child);
            lately_network_usage_data.r_bytes += child_network_data.r_bytes;
            lately_network_usage_data.w_bytes += child_network_data.w_bytes;
        }
    }
    result.first = ToKB(Calculator().NetDownload(early_network_usage_data, lately_network_usage_data));
    result.second = ToKB(Calculator().NetUpload(early_network_usage_data, lately_network_usage_data));
    return result;
}

std::pair<uint64_t, uint64_t> Rb_metrics::getIostat(uint32_t pid) // = 0
{
    std::pair<uint64_t, uint64_t> result;
    auto early_bd_usage_data = Collector().BdStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_bd_data = Collector().BdStat(child);
            early_bd_usage_data.r_bytes += child_bd_data.r_bytes;
            early_bd_usage_data.w_bytes += child_bd_data.w_bytes;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(m_period));
    auto lately_bd_usage_data = Collector().BdStat(pid);
    if (pid != 0)
    {
        for (auto child : getAllChildren(pid))
        {
            auto child_bd_data = Collector().BdStat(child);
            lately_bd_usage_data.r_bytes += child_bd_data.r_bytes;
            lately_bd_usage_data.w_bytes += child_bd_data.w_bytes;
        }
    }
    result.first = ToKB(Calculator().BlockDeviceRead(early_bd_usage_data, lately_bd_usage_data));
    result.second = ToKB(Calculator().BlockDeviceWrite(early_bd_usage_data, lately_bd_usage_data));
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