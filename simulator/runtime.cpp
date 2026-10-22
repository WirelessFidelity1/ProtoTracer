#include "Arduino.h"
#include "Adafruit_APDS9960.h"
#include <vector>
namespace Simulator {
    float audioLevel=0;
    bool deterministic=false;
    uint32_t simulatedMicros=0;
}
uint32_t micros() {
    if(Simulator::deterministic) return Simulator::simulatedMicros;
    static const auto origin=std::chrono::steady_clock::now();
    return uint32_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-origin).count());
}
void delay(unsigned ms) {
    if(Simulator::deterministic) Simulator::simulatedMicros+=ms*1000;
    else std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
int analogRead(int) { return 2048; }
static std::vector<IntervalTimer*>& Timers() { static std::vector<IntervalTimer*> timers; return timers; }
bool IntervalTimer::begin(void(*fn)(),unsigned us) {
    auto& timers=Timers();
    if(std::find(timers.begin(),timers.end(),this)==timers.end()) timers.push_back(this);
    callback=fn; interval=max(1u,us); previous=micros(); return true;
}
void IntervalTimer::Service() {
    for(auto* timer:Timers()) {
        if(timer->callback && micros()-timer->previous>=timer->interval) {
            timer->previous=micros(); timer->callback();
        }
    }
}
