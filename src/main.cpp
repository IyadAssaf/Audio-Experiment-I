#include <node.h>
#include <v8.h>
#include <stdlib.h>

#include "waveform.cpp"

using namespace v8;

Handle<Value> tones(const Arguments& args) {
    HandleScope scope;
    return scope.Close(String::New("Initialized"));
}

Handle<Value> start(const Arguments& args) {
    HandleScope scope;
    return scope.Close(String::New("started"));
}

Handle<Value> play(const Arguments& args) {
    HandleScope scope;

    double freq = (double)args[0]->NumberValue();
    double amp = (double)args[1]->NumberValue();
    int waveform = args[2]->NumberValue();

    Waveform *form = new Waveform();
    form->play(freq, amp, waveform);

    return scope.Close(String::New("started"));
}

void init(Handle<Object> target) {
    target->Set(String::NewSymbol("start"),
        FunctionTemplate::New(start)->GetFunction());

    target->Set(String::NewSymbol("play"),
        FunctionTemplate::New(play)->GetFunction());
}
NODE_MODULE(tones, init);
