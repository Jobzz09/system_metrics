#include "rb_metrics.hpp"

int main()
{
    while (1)
    {
        Rb_metrics meter(25528, 1);
        double cpuGen = 0, cpu = 0, ramGen = 0, ram = 0;
        uint64_t ramMbGen = 0, ramMb = 0, netGenr = 0, netGenw = 0, netr = 0, netw = 0,
                           ioGenw = 0, ioGenr = 0, ior = 0, iow = 0;

        std::thread th1([&cpuGen, &meter]()
                        { cpuGen = meter.GetGeneralCpuUsage(); });

        std::thread th2([&cpu, &meter]()
                        { cpu = meter.GetCpuUsage(); });

        std::thread th3([&ramGen, &meter]()
                        { ramGen = meter.GetGeneralRamUsage(); });

        std::thread th4([&ram, &meter]()
                        { ram = meter.GetRamUsage(); });

        std::thread th5([&ramMbGen, &meter]
                        { ramMbGen = meter.GetGeneralRamUsage_m(); });

        std::thread th6([&ramMb, &meter]()
                        { ramMb = meter.GetRamUsage_m(); });

        std::thread th7([&netGenr, &netGenw, &meter]()
                        {
            auto data = meter.GetGeneralNetUsage();
            netGenr = data.first;
            netGenw = data.second; });

        std::thread th8([&netr, &netw, &meter]
                        {
            auto data = meter.GetNetUsage();
            netr = data.first;
            netw = data.second; });

        std::thread th9([&meter, &ioGenr, &ioGenw]()
                        {
            auto data = meter.GetGeneralIoStats();
            ioGenr = data.first;
            ioGenw = data.second; });

        std::thread th10([&meter, &ior, &iow]
                         {
          auto data = meter.GetIoStats();
          ior = data.first;
          iow = data.second; });

            th1.detach();
            th2.detach();
            th3.detach();
            th4.detach();
            th5.detach();
            th6.detach();
            th7.detach();
            th8.detach();
            th9.detach();
            th10.detach();


            std::this_thread::sleep_for(std::chrono::seconds(2));
            system("clear");
            std::cout << "Current pid: 25528\n"
                      << "\t\r"
                      << "Cpu: " << cpuGen << "% " << cpu << "%\n"
                      << "\t\r"
                      << "Ram: " << ramGen << "% " << ram << "%\n"
                      << "\t\r"
                      << "Ram(mb): " << ramMbGen << " " << ramMb << "\n\t\r"
                      << "Net: " << netGenr << " kb/s " << netGenw << "kb/s"
                      << "\n\t\r" << netr << "kb/s " << netw << "kb/s"
                      << "\n\t\r"
                      << "IO: " << ioGenr << "kb/s " << ioGenw << "kb/s"
                      << "\n\t\r" << ior << "kb/s " << iow << "kb/s "
                      << "\n\t\r";
    }
}

// hpp - rb_include
// cpp - common
