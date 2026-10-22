#include "ExternalDevices/Sensors/Microphone/MicrophoneFourier_MAX9814.h"
namespace Simulator { extern float audioLevel; }
// Sensor substitute: feed the existing viseme/material pipeline with a
// controllable synthetic tone. Hardware FFT implementation remains untouched.
void MicrophoneFourier::Initialize(uint8_t p,uint16_t rate,float lo,float hi,float) {
    pin=p; sampleRate=rate; minDB=lo; maxDB=hi; isInitialized=true;
}
void MicrophoneFourier::Reset() {
    memset(inputStorage,0,sizeof(inputStorage));
    memset(outputData,0,sizeof(outputData));
    memset(outputDataFilt,0,sizeof(outputDataFilt));
}
void MicrophoneFourier::Update() {
    currentValue=threshold=Simulator::audioLevel;
    float time=millis()*0.001f;
    for(int i=0;i<FFTSize;++i) inputStorage[i]=2048+1800*currentValue*sinf(i*0.19f+time*8);
    for(int i=0;i<OutputBins;++i) {
        float peak=18+12*sinf(time*1.7f);
        float d=(i-peak)/8;
        outputData[i]=currentValue*expf(-d*d);
        outputDataFilt[i]+=(outputData[i]-outputDataFilt[i])*0.3f;
    }
}
