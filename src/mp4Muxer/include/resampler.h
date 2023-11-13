extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/mem.h>
}

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

struct SampleData{
    uint8_t** data_arr;
    int line_size;
    int nb_samples;
};


class Resampler
{
public:
    Resampler(){
        // ffplay -ar 48000 -channels 2 -f f32le test_fltp_48000_2.pcm
        // ffplay -ar 48000 -channels 2 -f f32le test_flt_48000_2.pcm
        // test_ = fopen("test_fltp_48000_2.pcm", "wb");

    }
    ~Resampler(){
        Deinit();
    }

    int Init(AVChannelLayout in_ch_layout, int in_sample_rate,AVSampleFormat in_sample_fmt, AVChannelLayout out_ch_layout, int out_sample_rate,AVSampleFormat out_sample_fmt,int src_nb_samples){
        // test_buf = (float*)av_malloc(8192);
        if(swr_ctx_){
            printf("swr_ctx_ is not nullptr\n");
            return -1;
        }
        int ret = 0;
        
        ret = swr_alloc_set_opts2(&swr_ctx_, &out_ch_layout, out_sample_fmt, out_sample_rate,
                &in_ch_layout, in_sample_fmt, in_sample_rate,
                0, NULL);
        if (ret < 0) {
            AV_ERR(ret, "swr_alloc_set_opts2 failed");
            return -1;
        }
        src_rate_ = in_sample_rate;
        src_ch_layout_ = in_ch_layout;
        src_sample_fmt_ = in_sample_fmt;

        dst_rate_ = out_sample_rate;
        dst_ch_layout_ = out_ch_layout;
        dst_sample_fmt_ = out_sample_fmt;

        
        ret = swr_init(swr_ctx_);
        if (ret < 0) {
            AV_ERR(ret, "swr_init failed");
            return -1;
        }
        max_dst_nb_samples_= src_nb_samples_ = src_nb_samples;
        


        ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_linesize_, out_ch_layout.nb_channels,
                                             max_dst_nb_samples_, dst_sample_fmt_, 0);
        if (ret < 0) {
            AV_ERR(ret, "Could not allocate destination samples");

        }

        return 0;
    }

    void Deinit(){
        
        if(dst_data_){
            av_freep(&dst_data_[0]);
        }
        if(swr_ctx_){
            swr_free(&swr_ctx_);
            swr_ctx_ = nullptr;
        }
        deinited_ = true;
    }
    
    // 返回指向一个AVFrame指针，不要在消费完这个AVFrame前调用这个函数
    // 这个frame的格式对应dst_sample_fmt_，通道数对应dst_ch_layout_，采样率对应dst_rate_
    AVFrame* ReSample(uint8_t** src_data){
        
        int ret = 0;
        // int64_t delay = swr_get_delay(swr_ctx_, src_rate_);
        int dst_nb_samples = src_nb_samples_; // 我们要转成和源音频相同的采样数，编码器要求

        /* convert to destination format */
        ret = swr_convert(swr_ctx_, dst_data_, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples_);
        if (ret < 0) {
            AV_ERR(ret, "Error while converting");
            return nullptr;
        }
        int dst_bufsize = av_samples_get_buffer_size(&dst_linesize_, dst_ch_layout_.nb_channels,
                                                 ret, dst_sample_fmt_, 1);
        if (dst_bufsize < 0) {
            AV_ERR(ret, "Could not get sample buffer size");

            return nullptr;
        }
        printf("in:%d out:%d\n", src_nb_samples_, ret);

        float * L = (float*)dst_data_[0];
        float * R = (float*)dst_data_[1];

        // for(int i=0, p = 0;i<dst_bufsize/8;i++,p+=2){
        //     test_buf[p] = L[i];
        //     test_buf[p+1] = R[i];
            
        // }

        // dst_data_ 是一个指针数组，指向的是一个数组，数组的每个元素是一个通道的数据
        // 两个通道是空间上相邻的，所以我们要返回第一个通道的地址，即dst_data_[0]
        // 而不是这个指针数组的地址，即dst_data_


        // fwrite(dst_data_, 1, dst_bufsize, test_);

        // return {dst_data_[0],dst_nb_samples };

        // 可以返回这个dst_data_，在encode中进行处理就可以

        AVFrame* frame = av_frame_alloc();
        frame->data[0] = dst_data_[0];
        frame->data[1] = dst_data_[1];
        // audio 只用设置linesize[0]
        frame->linesize[0] = dst_linesize_;
        frame->nb_samples = dst_nb_samples;
        frame->format = dst_sample_fmt_;
        frame->ch_layout = dst_ch_layout_;

        return frame;
    }


private:
    FILE *test_;
    SwrContext* swr_ctx_{nullptr};
    
    float * test_buf;
    uint8_t** dst_data_{nullptr};
    int src_rate_;
    int dst_rate_;
    AVSampleFormat src_sample_fmt_;
    AVSampleFormat dst_sample_fmt_;
    AVChannelLayout src_ch_layout_;
    AVChannelLayout dst_ch_layout_;

    int src_linesize_;
    int dst_linesize_;
    int src_nb_samples_;

    int max_dst_nb_samples_;

    bool deinited_;
};
