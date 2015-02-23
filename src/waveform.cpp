#include <stdlib.h>
#include <math.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFRunLoop.h>

#define pi 3.14159265359
#define NUM_CHANNELS 2
#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096
#define SAMPLE_TYPE short
#define MAX_NUMBER 32767
#define SAMPLE_RATE 44100

typedef struct AQCallbackStruct {
    double freq;
    double amp;
    int tableType;
} AQCallbackStruct;

void callback(void *custom_data, AudioQueueRef queue, AudioQueueBufferRef buffer);

class Waveform {

    AudioStreamBasicDescription format;
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
public:

    Waveform() {
        format.mSampleRate       = SAMPLE_RATE;
        format.mFormatID         = kAudioFormatLinearPCM;
        format.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        format.mBitsPerChannel   = 8 * sizeof(SAMPLE_TYPE);
        format.mChannelsPerFrame = NUM_CHANNELS;
        format.mBytesPerFrame    = sizeof(SAMPLE_TYPE) * NUM_CHANNELS;
        format.mFramesPerPacket  = 1;
        format.mBytesPerPacket   = format.mBytesPerFrame * format.mFramesPerPacket;
        format.mReserved         = 0;
    }

    ~Waveform() {
        AudioQueueStop(queue, false);
        AudioQueueDispose(queue, false);
        CFRunLoopStop(CFRunLoopGetCurrent());
    }
    void play(float freq, float amp, int tableType);
private:
    void generateTone(AudioQueueBufferRef buffer, double freq, double amp, int outputWaveform);
};

void callback(void *custom_data, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    AQCallbackStruct *aqc = (AQCallbackStruct *)custom_data;
    double freq = aqc->freq;
    double amp = aqc->amp;

    if(freq == 0.0) {
        memset(buffer->mAudioData, 0, buffer->mAudioDataBytesCapacity);
        buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
    } else {
        // Make the buffer length a multiple of the wavelength for the output frequency.
        int sampleCount = buffer->mAudioDataBytesCapacity / sizeof(SInt16);
        double bufferLength = sampleCount;
        double wavelength = SAMPLE_RATE / freq;
        double repetitions = floor(bufferLength / wavelength);
        if(repetitions > 0.0) {
            sampleCount = round(wavelength * repetitions);
        }
        double x, y;
        double sd = 1.0 / SAMPLE_RATE;
        double max16bit = SHRT_MAX;
        int i;

        short *p = (short *)buffer->mAudioData;
        int outputWaveform = aqc->tableType;

        for(i = 0; i < sampleCount; i++) {
            x = i * sd * freq;
            switch(outputWaveform) {
                    // sine
                case 1:
                    y = sin(x * 2.0 * M_PI);
                    break;
                    // triangle
                case 2:
                    x = fmod(x, 1.0);
                    if(x < 0.25)
                        y = x * 4.0; // up 0.0 to 1.0
                    else if(x < 0.75)
                        y = (1.0 - x) * 4.0 - 2.0; // down 1.0 to -1.0
                    else
                        y = (x - 1.0) * 4.0; // up -1.0 to 0.0
                    break;
                    // sawtooth
                case 3:
                    y  = 0.8 - fmod(x, 1.0) * 1.8;
                    break;
                    // sqaure
                case 4:
                    y = (fmod(x, 1.0) < 0.5)? 0.7: -0.7;
                    break;
                default: y = 0; break;
            }
            p[i] = y * max16bit * amp;
        }
        buffer->mAudioDataByteSize = sampleCount * sizeof(SInt16);
    }

    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}


void Waveform::play(float freq, float amp, int tableType)
{
    AQCallbackStruct out;
    out.freq = freq;
    out.amp = amp;
	out.tableType = tableType;

    AudioQueueNewOutput(&format, callback, &out, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);

    for (unsigned int i = 0; i < NUM_BUFFERS; i++)
    {
        AudioQueueAllocateBuffer(queue, BUFFER_SIZE, &buffers[i]);
        buffers[i]->mAudioDataByteSize = BUFFER_SIZE;
        callback(&out, queue, buffers[i]);
    }
    AudioQueueStart(queue, NULL);
    CFRunLoopRun();
};
