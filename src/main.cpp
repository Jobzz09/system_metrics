#include "rb_metrics.hpp"

int main()
{
    while (1)
    {
        Rb_metrics meter(6097, 1);
        double cpuGen = 0, cpu = 0, ramGen = 0, ram = 0;
        unsigned long long ramMbGen = 0, ramMb = 0, netGenr = 0, netGenw = 0, netr = 0, netw = 0,
                           ioGenw = 0, ioGenr = 0, ior = 0, iow = 0;

        auto th1([&cpuGen, &meter]()
                 { cpuGen = meter.GetGeneralCpuUsage(); });

        auto th2([&cpu, &meter]()
                 { cpu = meter.GetCpuUsage(); });

        auto th3([&ramGen, &meter]()
                 { ramGen = meter.GetGeneralRamUsage(); });

        auto th4([&ram, &meter]()
                 { ram = meter.GetRamUsage(); });

        auto th5([&ramMbGen, &meter]
                 { ramMbGen = meter.GetGeneralRamUsage_m(); });

        auto th6([&ramMb, &meter]()
                 { ramMb = meter.GetRamUsage_m(); });

        auto th7([&netGenr, &netGenw, &meter]()
                 {
            auto data = meter.GetGeneralNetUsage();
            netGenr = data.first;
            netGenw = data.second; });

        auto th8([&netr, &netw, &meter]
                 {
            auto data = meter.GetNetUsage();
            netr = data.first;
            netw = data.second; });

        auto th9([&meter, &ioGenr, &ioGenw]()
                 {
            auto data = meter.GetGeneralIoStats();
            ioGenr = data.first;
            ioGenw = data.second; });

        auto th10([&meter, &ior, &iow]
                  {
          auto data = meter.GetIoStats();
          ior = data.first;
          iow = data.second; });

        // th1();
        // th2();
        // th3();
        // th4();
        // th5();
        // th6();
        // std::thread t1(th9);
        //        std::thread t2(th10);

        // t1.detach();
        // t2.detach();

        th9();
        th10();

        std::this_thread::sleep_for(std::chrono::seconds(1));

        // std::cout << "CPU: " << cpuGen << "% " << cpu << "%" << std::endl;
        // std::cout << "RAM: " << ramGen << "% " << ramMbGen << "mb " << ram << "% " << ramMb << "mb" << std::endl;
        std::cout << "IO read: " << ioGenr << "kb/s pid:" << ior << "kb/s" << std::endl;
        std::cout << "IO write: " << ioGenw << "kb/s pid:" << iow << "kb/s" << std::endl;
    }
}

// hpp - rb_include
// cpp - common
