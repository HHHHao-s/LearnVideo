#include "muxer.h"
#include "audioEncoder.h"
#include "videoEncoder.h"
#include "resampler.h"


#define YUV_WIDTH 1920
#define YUV_HEIGHT 1080
#define YUV_FPS 60
#define YUV_BIT_RATE 500000

#define PCM_SAMPLE_RATE 48000
#define PCM_CHANNEL 2
#define PCM_BIT_RATE 128000
#define PCM_SAMPLE_FORMAT AV_SAMPLE_FMT_S16
#define PCM_CH_LAYOUT AV_CH_LAYOUT_STEREO
#define TIME_BASE 1000000
#define DST_PCM_SAMPLE_RATE 48000
// ffmpeg -i sync-h264.mp4 -an -video_size 1920x1080 -pixel_format yuv420p -t 15 sync_1920x1080_yuv420p.yuv
// ffmpeg -i sync-h264.mp4 -vn -ar 48000 -f s16le -s 2 -t 15 sync_48000_2_s16le.pcm
int main(int argc, char **argv){

    const char* yuv_name = NULL;
    const char* pcm_name = NULL;
    const char* output_file = NULL;
    if(argc!=4){
        printf("Usage: %s <*.yuv> <*.pcm> <output file>\n", argv[0]);
        yuv_name = "sync_1920x1080_yuv420p.yuv";
        pcm_name = "sync_48000_2_s16le.pcm";
        output_file = "sync-my.mp4";
    }else{
        printf("input file: %s\n", argv[1]);
        printf("input file: %s\n", argv[2]);
        printf("output file: %s\n", argv[3]);
        yuv_name = argv[1];
        pcm_name = argv[2];
        output_file = argv[3];
    }
    int ret = 0;


    FILE *yuv_fp = fopen(yuv_name, "rb");
    if(!yuv_fp){
        printf("open %s failed\n", yuv_name);
        return -1;
    }
    FILE *pcm_fp = fopen(pcm_name, "rb");
    if(!pcm_fp){
        printf("open %s failed\n", pcm_name);
        return -1;
    }

    
    
    VideoEncoder videoEncoder;
    videoEncoder.InitH264(YUV_WIDTH, YUV_HEIGHT, YUV_FPS, YUV_BIT_RATE);

    int yuv_size =av_image_get_buffer_size(AV_PIX_FMT_YUV420P, YUV_WIDTH, YUV_HEIGHT, 1);
    uint8_t* yuv_buffer = (uint8_t*)av_malloc(yuv_size);

    AudioEncoder audioEncoder;
    audioEncoder.InitAAC(PCM_CHANNEL, PCM_SAMPLE_RATE, PCM_BIT_RATE);

    uint8_t **pcm_data=NULL;
    int line_size=0;
    AVChannelLayout src_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16;
    AVSampleFormat dst_sample_fmt = (AVSampleFormat)audioEncoder.GetSampleFormat();
    // AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_FLT;
    int src_rate = PCM_SAMPLE_RATE;
    int src_nb_samples = audioEncoder.GetFrameSize();
    int dst_rate = DST_PCM_SAMPLE_RATE;
    ret = av_samples_alloc_array_and_samples(&pcm_data, &line_size, src_ch_layout.nb_channels, src_rate, src_sample_fmt, 1 );
    int read_pcm_size = src_nb_samples * src_ch_layout.nb_channels * av_get_bytes_per_sample(src_sample_fmt);
    if(ret<0){
        AV_ERR(ret, "av_samples_alloc_array_and_samples");
        return -1;
    }
    Resampler resampler;
    resampler.Init(src_ch_layout, src_rate,src_sample_fmt, src_ch_layout, dst_rate, dst_sample_fmt, src_nb_samples);

    Muxer muxer;
    muxer.Init(output_file);
    
    ret = muxer.AddStream(videoEncoder.GetCtx());
    if(ret<0){
        printf("AddStream video failed\n");
        return -1;
    }
    ret = muxer.AddStream(audioEncoder.GetCtx());
    if(ret<0){
        printf("AddStream audio failed\n");
        return -1;
    }

    ret = muxer.Open();
    if(ret<0){
        printf("muxer.Open failed\n");
        return -1;
    }
    ret = muxer.WriteHeader();
    if(ret<0){
        printf("muxer.WriteHeader failed\n");
        return -1;
    }
    int64_t audio_time_base = TIME_BASE;
    int64_t video_time_base = TIME_BASE;

    double audio_pts = 0;
    double video_pts = 0;
    double audio_frame_duration = 1.0 *audioEncoder.GetFrameSize()/ audioEncoder.GetSampleRate()*audio_time_base;
    double video_frame_duration = 1.0 / YUV_FPS * video_time_base;

    bool audio_eof = false;// 先测试视频
    bool video_eof = false;

    int audio_index = muxer.GetAudioIndex();
    int video_index = muxer.GetVideoIndex();
    

    while(!audio_eof || !video_eof){
        if(!audio_eof){
            ret = fread(pcm_data[0], 1, read_pcm_size, pcm_fp);
            if(ret!=read_pcm_size){
                printf("fread audio finish\n");
                audio_eof = true;
            }

            auto [dst_data,size] = resampler.ReSample(pcm_data);
            std::queue<AVPacket*> q = audioEncoder.Encode((uint8_t*)dst_data, size, audio_index, audio_pts, audio_time_base);
            while(!q.empty()){
                AVPacket* pkt = q.front(); // 可以为pkt实现RAII
                q.pop();
                pkt->stream_index = audio_index;
                ret = muxer.WritePacket(pkt);
                if(ret<0){
                    printf("WritePacket audio failed\n");
                    return -1;
                }
                
            }
            audio_pts += audio_frame_duration;
        }
        if(!video_eof){
            ret = fread(yuv_buffer, 1, yuv_size, yuv_fp);
            if(ret!=yuv_size){
                printf("fread video finish\n");
                video_eof = true;
            }
            std::queue<AVPacket*> q = videoEncoder.Encode(yuv_buffer, yuv_size, video_index, video_pts, video_time_base);
            while(!q.empty()){
                AVPacket* pkt = q.front();
                q.pop();
                pkt->stream_index = video_index;
                ret = muxer.WritePacket(pkt);
                
                if(ret<0){
                    printf("WritePacket video failed\n");
                    return -1;
                }
                
            }
            video_pts += video_frame_duration;
        }
    }
    // flush
    videoEncoder.Encode(nullptr, 0, video_index, video_pts, video_time_base);
    audioEncoder.Encode(nullptr,0,  audio_index, audio_pts, audio_time_base);
    
    ret = muxer.WriteTrailer();
    if(ret<0){
        printf("WriteTrailer failed\n");
        return -1;

    }

    if(yuv_fp)
        fclose(yuv_fp);

    if(pcm_fp)
        fclose(pcm_fp);

    if(yuv_buffer)
        av_freep(&yuv_buffer);

    if(pcm_data)
        av_freep(&pcm_data);

    return 0;

}