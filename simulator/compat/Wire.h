#pragma once
struct SimulatorWire {
    void begin() {} void end() {} void setClock(int) {}
    void setSDA(int) {} void setSCL(int) {} void beginTransmission(int) {}
    int endTransmission() { return 0; }
};
inline SimulatorWire Wire;
