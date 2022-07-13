#include "rb_metrics.hpp"

int main() {
    while (1) {
        Rb_metrics meter(143841, 1);
        std::cout << "CPU:\nGeneral: " << meter.GetGeneralCpuUsage() << " Pid's: " << meter.GetCpuUsage() << std::endl;
    }
}