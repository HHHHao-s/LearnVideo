#include "muxer.h"
#include "audioEncoder.h"

int main(){
    Muxer muxer;
    muxer.Init("test.mp4");

    AudioEncoder audioEncoder;
    audioEncoder.InitAAC(2, 44100, 128000);

    return 0;

}