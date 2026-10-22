#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <string>
#include <algorithm>
#include <type_traits>
#include <chrono>
#include <thread>
#define PROGMEM
#define DMAMEM
#define FLASHMEM
#define FASTRUN
#define F(s) reinterpret_cast<const __FlashStringHelper*>(s)
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
#define HEX 16
#define PI 3.14159265358979323846
#define TWO_PI (2*PI)
#define DEG_TO_RAD (PI/180)
#define RAD_TO_DEG (180/PI)
struct __FlashStringHelper;
using byte=uint8_t;
class String : public std::string {
public:
    using std::string::string;
    String(const std::string& s):std::string(s) {}
    String(const __FlashStringHelper* s):std::string(reinterpret_cast<const char*>(s)) {}
    String(int n):std::string(std::to_string(n)) {}
    String(unsigned n):std::string(std::to_string(n)) {}
    String(long n):std::string(std::to_string(n)) {}
    String(unsigned long n):std::string(std::to_string(n)) {}
    String(float n,int digits=2) { char b[80]; std::snprintf(b,sizeof(b),"%.*f",digits,n); assign(b); }
    String(double n,int digits=2):String(float(n),digits) {}
};
template<class A,class B> inline auto min(A a,B b)->std::common_type_t<A,B> { return a<b?a:b; }
template<class A,class B> inline auto max(A a,B b)->std::common_type_t<A,B> { return a>b?a:b; }
template<class T,class A,class B> inline T constrain(T x,A lo,B hi) { return min(max(x,lo),hi); }
template<class T,class A,class B,class C,class D> inline auto map(T x,A a,B b,C c,D d) { return (x-a)*(d-c)/(b-a)+c; }
inline long random(long n) { return n>0 ? std::rand()%n : 0; }
inline long random(long a,long b) { return a+random(b-a); }
inline void randomSeed(unsigned n) { std::srand(n); }
uint32_t micros();
inline uint32_t millis() { return micros()/1000u; }
void delay(unsigned ms);
inline void delayMicroseconds(unsigned us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline int digitalRead(int) { return HIGH; }
inline void analogWrite(int,int) {}
inline void analogWriteFrequency(int,int) {}
inline void analogReadResolution(int) {}
int analogRead(int);
inline void noInterrupts() {}
inline void interrupts() {}
inline int digitalPinToInterrupt(int n) { return n; }
inline void attachInterrupt(int,void(*)(),int) {}
#define FALLING 2
inline uint8_t pgm_read_byte(const void* p) { return *static_cast<const uint8_t*>(p); }
inline uint16_t pgm_read_word(const void* p) { return *static_cast<const uint16_t*>(p); }
inline uint32_t pgm_read_dword(const void* p) { return *static_cast<const uint32_t*>(p); }
inline void* pgm_read_ptr(const void* p) { return *static_cast<void* const*>(p); }
inline size_t strlen_P(const char* s) { return strlen(s); }
struct SimulatorSerial {
    void begin(int) {}
    template<class... T> void print(T&&...) {}
    template<class... T> void println(T&&...) {}
};
inline SimulatorSerial Serial;
class IntervalTimer {
public:
    void (*callback)()=nullptr;
    uint32_t interval=0, previous=0;
    bool begin(void(*fn)(),unsigned us);
    void end() { callback=nullptr; }
    void priority(int) {}
    static void Service();
};
