#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

#define PI 3.14159

double frequency = 880.0;

typedef struct MyAUGraphPlayer
{
    AudioStreamBasicDescription streamFormat;

    AUGraph graph;
    AUNode output;
    AUNode mixer;
    AUNode sine;
    AudioUnit audioUnits[3];
    AudioBufferList *inputBuffer;

    Float64 firstInputSampleTime;
    Float64 firstOutputSampleTime;
    Float64 inToOutSampleTimeOffset;
} MyAUGraphPlayer;

OSStatus SineWaveRenderCallback(void * inRefCon,
                                AudioUnitRenderActionFlags * ioActionFlags,
                                const AudioTimeStamp * inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList * ioData)
{
    // inRefCon is the context pointer we passed in earlier when setting the render callback
    double currentPhase = *((double *)inRefCon);
    // ioData is where we're supposed to put the audio samples we've created
    Float32 * outputBuffer = (Float32 *)ioData->mBuffers[0].mData;
    // const double phaseStep = (frequency / 44100.) * (M_PI * 2.);
	const double phaseStep = (frequency / 44100.) * (M_PI * 2.);

	for(int i = 0; i < inNumberFrames; i++) {
		// printf("Current phase: %d , phase step: %d \n", sin(currentPhase), phaseStep);
        outputBuffer[i] = sin(currentPhase);
		printf("Output buffer: %d\n", sin(currentPhase));
        currentPhase += phaseStep;
    }

    // If we were doing stereo (or more), this would copy our sine wave samples
    // to all of the remaining channels
    for(int i = 1; i < ioData->mNumberBuffers; i++) {
        memcpy(ioData->mBuffers[i].mData, outputBuffer, ioData->mBuffers[i].mDataByteSize);
    }

    // writing the current phase back to inRefCon so we can use it on the next call
    *((double *)inRefCon) = currentPhase;
    return noErr;
}


int main(int argc, const char * argv[])
{
    printf("args: %s", argv[1]);
    if(argv[1]) {
        frequency = (double)atoi(argv[1]);
    }

    MyAUGraphPlayer *player = {0};
    MyAUGraphPlayer p = {0};
    player=&p;

    NewAUGraph(&player->graph);

    OSStatus result = 0;

    AudioStreamBasicDescription ASBD = {
        .mSampleRate       = 44100,
        .mFormatID         = kAudioFormatLinearPCM,
        .mFormatFlags      = kAudioFormatFlagsNativeFloatPacked,
        .mChannelsPerFrame = 2,
        .mFramesPerPacket  = 1,
        .mBitsPerChannel   = sizeof(Float32) * 8,
        .mBytesPerPacket   = sizeof(Float32),
        .mBytesPerFrame    = sizeof(Float32)
    };

    //Output
    {
        AudioComponentDescription description = {
            .componentType = kAudioUnitType_Output,
            .componentSubType = kAudioUnitSubType_DefaultOutput,
            .componentManufacturer = kAudioUnitManufacturer_Apple
        };
        result = AUGraphAddNode(player->graph, &description, &player->output);
        printf("err: %d\n", result);
        AudioComponent comp = AudioComponentFindNext(NULL, &description);
        result = AudioComponentInstanceNew(comp, &player->audioUnits[0]);
        printf("err: %d\n", result);
        result = AudioUnitInitialize(player->audioUnits[0]);
        printf("err: %d\n", result);
    }

    //Mixer
    {
        AudioComponentDescription description = {
            .componentType = kAudioUnitType_Mixer,
            .componentSubType = kAudioUnitSubType_StereoMixer,
            .componentManufacturer = kAudioUnitManufacturer_Apple
        };
        result = AUGraphAddNode(player->graph, &description, &player->mixer);
        printf("err: %d\n", result);
        AudioComponent comp = AudioComponentFindNext(NULL, &description);
        result = AudioComponentInstanceNew(comp, &player->audioUnits[1]);
        printf("err: %d\n", result);
    }


    //Sine
    {
        AudioComponentDescription description = {
            .componentType = kAudioUnitType_Generator,
            .componentSubType = kAudioUnitSubType_ScheduledSoundPlayer,
            .componentManufacturer = kAudioUnitManufacturer_Apple
        };
        result = AUGraphAddNode(player->graph, &description, &player->sine);
        printf("err: %d\n", result);
        AudioComponent comp = AudioComponentFindNext(NULL, &description);
        result = AudioComponentInstanceNew(comp, &player->audioUnits[2]);
        printf("err: %d\n", result);
        result = AudioUnitInitialize(player->audioUnits[2]);
        printf("err: %d\n", result);

    }

    result = AUGraphConnectNodeInput(player->graph,
                                     player->sine,
                                     0,
                                     player->mixer,
                                     0);
    printf("err: %d\n", result);

    result = AUGraphConnectNodeInput(player->graph,
                                     player->mixer,
                                     0,
                                     player->output,
                                     0);
    printf("err: %d\n", result);

    result = AUGraphOpen(player->graph);
    printf("err: %d\n", result);


    UInt32 numbuses = 1;
    result = AudioUnitSetProperty(player->audioUnits[1], kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &numbuses, sizeof(numbuses));
    printf("err: %d\n", result);

    for (UInt32 i = 0; i <= numbuses; ++i) {
        // setup render callback struct
        AURenderCallbackStruct rcbs;
        rcbs.inputProc = &SineWaveRenderCallback;
        rcbs.inputProcRefCon = &player;

        printf("set AUGraphSetNodeInputCallback\n");

        // set a callback for the specified node's specified input
        result = AUGraphSetNodeInputCallback(player->graph, player->mixer, i, &rcbs);
        printf("AUGraphSetNodeInputCallback err: %d\n", result);

        printf("set input bus %d, client kAudioUnitProperty_StreamFormat\n", (unsigned int)i);

        // set the input stream format, this is the format of the audio for mixer input
        result = AudioUnitSetProperty(player->audioUnits[1], kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, i, &ASBD, sizeof(ASBD));
        printf("err: %d\n", result);
    }

    result = AudioUnitSetProperty(player->audioUnits[1], kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &ASBD, sizeof(ASBD));
    printf("err: %d\n", result);

    OSStatus status = AUGraphInitialize(player->graph);
    printf("err: %d\n", status);

    player->firstOutputSampleTime = -1;
    AudioOutputUnitStart(player->audioUnits[0]);
    AUGraphStart(player->graph);

    getchar();

    return 0;
}
