#include <queue>
#define AV_ERR(ret, msg) \
    {\
        char errStr[64] = { 0 };\
        av_strerror(ret, errStr, 64);\
        printf("%s %d %s:%s\n",__FILE__, __LINE__, msg, errStr);\
    }

#define AV_MSG(msg) \
    {\
        printf("%s %d %s\n",__FILE__, __LINE__, msg);\
    }

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}


class VideoEncoder
{
public:
    VideoEncoder();
    ~VideoEncoder();
    int InitH264(int width, int height, int fps, int bitrate);
    void Deinit();
    std::queue<AVPacket*> Encode(uint8_t* frame, int yuv_size, int stream_index, int64_t pts, int64_t time_base);
    AVCodecContext *GetCtx(){
        return codec_ctx_;
    }
private:
    int width_;
    int height_;
    int fps_;
    int bitrate_;
    const AVCodec *codec_;
    AVCodecContext *codec_ctx_;
    int64_t pts_;
    bool deinited_;
    AVFrame* frame_;


};