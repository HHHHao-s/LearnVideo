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


class Resampler
{
public:
    Resampler(){

    }
    ~Resampler(){
        Deinit();
    }

    int Init(AVChannelLayout in_ch_layout, int in_sample_rate,AVSampleFormat in_sample_fmt, AVChannelLayout out_ch_layout, int out_sample_rate,AVSampleFormat out_sample_fmt,int src_nb_samples){
        if(swr_ctx_){
            printf("swr_ctx_ is not nullptr\n");
            return -1;
        }
        int ret =0;
        swr_ctx_ = swr_alloc();
        if (!swr_ctx_) {
            fprintf(stderr, "Could not allocate resampler context\n");
            return -1;
        }
        
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
        src_nb_samples_ = src_nb_samples;
        max_dst_nb_samples_ = av_rescale_rnd(src_nb_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);

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
    // 不要在消费完dst_data前调用这个函数
    uint8_t ** ReSample(uint8_t** src_data){
        
        int ret = 0;
        int64_t delay = swr_get_delay(swr_ctx_, src_rate_);
        int dst_nb_samples = av_rescale_rnd( delay+
                                        src_nb_samples_, dst_rate_, src_rate_, AV_ROUND_UP);

                                        
        if (dst_nb_samples > max_dst_nb_samples_) {
            av_freep(&dst_data_[0]);
            ret = av_samples_alloc(dst_data_, &dst_linesize_, dst_ch_layout_.nb_channels,
                                   dst_nb_samples, dst_sample_fmt_, 1);
            if (ret < 0){
                AV_ERR(ret, "av_samples_alloc");
                return nullptr;
            }
                
            max_dst_nb_samples_ = dst_nb_samples;
        }

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
        return dst_data_;
    }


private:

    SwrContext* swr_ctx_;
    
    
    uint8_t** dst_data_;
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
