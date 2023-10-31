

#include <stdio.h>

#include <string.h>



#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavutil/mem.h>
#define AUDIO_INBUF_SIZE    100000
#define AUDIO_REFILL_THRESH 4096


static void decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, FILE *outfile)
{
    int i, ch;
    int ret, sample_size;

    int send_ret = 1;
    do
    {
        //传入要解码的packet
        ret = avcodec_send_packet(dec_ctx, pkt);
        //AVERROR(EAGAIN) 传入失败，表示先要receive frame再重新send packet
        if(ret == AVERROR(EAGAIN))
        {
            send_ret = 0;
            fprintf(stderr, "avcodec_send_packet = AVERROR(EAGAIN)\n");
        }
        else if(ret < 0)
        {
            fprintf(stderr, "avcodec_send_packet = ret < 0 : %s\n", av_err2str(ret));
            return;
        }

        while (ret >= 0)
        {
            //调用avcodec_receive_frame会在内部首先调用av_frame_unref来释放frame本来的数据
            //就是这次调用会将上次调用返回的frame数据释放
            ret = avcodec_receive_frame(dec_ctx, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            else if(ret < 0)
            {
                fprintf(stderr, "avcodec_receive_frame = ret < 0\n");
                exit(1);
            }

            //获取采样点占用的字节
            sample_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
            if(sample_size < 0)
            {
                fprintf(stderr, "av_get_bytes_per_sample = sample_size < 0\n");
                exit(1);
            }

            //在第一帧的时候输出一下音频信息
            static int print_info = 1;
            if(print_info)
            {
                print_info = 0;
                printf("ar-samplerate: %uHz\n", frame->sample_rate);
                printf("ac-channel: %u\n", frame->ch_layout.nb_channels);
                printf("f-format: %u\n", frame->format);
            }
            //平面方式：
            //LLLLRRRRLLLLRRRRLLLLRRRR      (LLLLRRRR这样为一个音频帧)
            //交错方式：
            //LRLRLRLRLRLRLRLRLR           (LR这样为一个音频样本)
            //按交错方式写入(nb_samples这一帧多少样本)
            for(i = 0; i < frame->nb_samples; i++)
            {   //dec_ctx->channels 多少个通道的数据
                for(int channle = 0; channle < dec_ctx->ch_layout.nb_channels; channle++)
                {
                    //frame->data[0] = L 通道
                    //frame->data[1] = R 通道
                    fwrite(frame->data[channle] + sample_size * i, 1, sample_size, outfile);

                }
            }
        }

    }while(!send_ret);


}

static int read_file(void *user, uint8_t *buf, int buf_size)
{
    if(!user)
        return AVERROR_EOF;

    FILE *fp = (FILE*)user;
    int true_size = fread(buf, 1, buf_size, fp);
    if(true_size < buf_size)
    {
        if(feof(fp))
            return AVERROR_EOF;
        else if(ferror(fp))
            return AVERROR(EIO);
    }
    printf("read_file true_size = %d\n", true_size);
    return true_size;
}

static int parse_aac_to_pcm(const char* input_filename, const char* output_filename){

    const AVCodec *codec;
    AVCodecContext *codec_ctx= NULL;
    AVCodecParserContext *parser = NULL;
    int len = 0;
    int ret = 0;

   
    
    uint8_t *data = NULL;
    size_t   data_size = 0;
    AVPacket *pkt = NULL;
    AVFrame *de_frame = NULL;


    //申请AVPacket本身的内存
    FILE* in_file = fopen(input_filename, "rb");
    
    if(!in_file)
    {
        fprintf(stderr, "Could not open %s\n", input_filename);
        exit(1);
    }

    FILE *outfile = fopen(output_filename, "wb");
    if(!outfile)
    {
        fprintf(stderr, "Could not open %s\n", output_filename);
        exit(1);
    }

    char *in_buf = av_malloc(AUDIO_INBUF_SIZE);
    AVIOContext *avio_ctx = NULL;
    //创建AVIOContext
    // avio_ctx 会把av_malloc来的buf在free的时候释放掉
    avio_ctx = avio_alloc_context(in_buf, AUDIO_INBUF_SIZE, 0, (void *)in_file, (void *)&read_file, NULL, NULL);
    
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    fmt_ctx->pb = avio_ctx;
    
    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    // printf("avformat_open_input error%s\n", av_err2str(ret));
    if(ret<-1)
    {   

        printf("avformat_open_input error%s\n", av_err2str(ret));
        return -1;
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if(!codec)
    {
        printf("avcodec_find_decoder error\n");
        return -1;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx)
    {
        printf("avcodec_alloc_context3 error\n");
        return -1;
    }

    ret = avcodec_open2(codec_ctx, codec, NULL);
    if(ret < 0)
    {
        printf("avcodec_open2 error%s\n", av_err2str(ret));
        return -1;
    }

    pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    while(1){
        ret = av_read_frame(fmt_ctx, pkt);

        if(ret < 0)
        {
            printf("av_read_frame error%s\n", av_err2str(ret));
            av_packet_unref(pkt);
            break;
        }
        decode(codec_ctx, pkt, frame, outfile);

        av_packet_unref(pkt);
    }

    printf("decode end\n");
    decode(codec_ctx, NULL, frame, outfile);

 
    
    // avcodec_free_context(&codec_ctx);

    
    
    // 自定义avio不会把里面的pb释放掉
    // fmt_ctx会free掉
    avformat_close_input(&fmt_ctx);
 
    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx)
        av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
    
    av_frame_free(&frame);
    av_packet_free(&pkt);
    
    
    fclose(in_file);

    fclose(outfile);
    
    printf("niubi");
    return 0;

}

int main(int argc, char *argv[])
{
    
    char* filename = NULL;
    char* outfilename = NULL;
    if(argc != 3)
    {
        printf("Usage: %s <input file> <output file>\n", argv[0]);


        filename = "test.aac";
        outfilename = "test.pcm";
    }else{
        filename = argv[1];
        outfilename = argv[2];
    }
    
    parse_aac_to_pcm(filename, outfilename);
    
    printf("niubi");

    return 0;
}

