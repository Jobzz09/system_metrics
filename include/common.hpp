#ifndef RB_METRICS_COMMON
#define RB_METRICS_COMMON

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

int ParseLine(char *line);

bool Is_number(const std::string &data);

std::string ExecCommandWithResult(const char *cmd);

std::vector<unsigned int> GetAllChildren(unsigned int pid);

std::string ProcStat2String(unsigned int pid);

unsigned int GetPpidFromProcStatString(std::string procStat_string);

namespace system_metrics
{
    unsigned long long GetSystemUptime();

    unsigned long long GetCpuSnapshot(unsigned int pid = 0);

    void initProcMetrics();
}

#endif