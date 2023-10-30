#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"{

#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
}
#define AUDIO_INBUF_SIZE    10240
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
            char err[128] = {0};
            av_strerror(ret, err, 128);
            fprintf(stderr, "avcodec_send_packet = ret < 0 : %s\n", err);
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

static int parse_aac_to_pcm(const char* input_filename, const char* output_filename){

    const AVCodec *codec;
    AVCodecContext *codec_ctx= NULL;
    AVCodecParserContext *parser = NULL;
    int len = 0;
    int ret = 0;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data = NULL;
    size_t   data_size = 0;
    AVPacket *pkt = NULL;
    AVFrame *de_frame = NULL;


    //申请AVPacket本身的内存
    pkt = av_packet_alloc();
    //要使用的解码器ID
    enum AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
    if(strstr(input_filename, "aac") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_AAC;
    }
    else if(strstr(input_filename, ".mp3") != NULL)
    {
        audio_codec_id = AV_CODEC_ID_MP3;
    }
    else
    {
        printf("audio_codec_id = AV_CODEC_ID_NONE\n");
        return 0;
    }

    //查找相应的解码器
    codec = avcodec_find_decoder(audio_codec_id);
    if(!codec)
    {
        printf("Codec not find!\n");
        return 0;
    }

    //根据解码器ID获取裸流的解析器
    parser = av_parser_init(codec->id);
    if(!parser)
    {
        printf("Parser not find!\n");
        return 0;
    }

    //分配codec使用的上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx)
    {
        printf("avcodec_alloc_context3 failed!\n");
        return 0;
    }

    //将解码器和解码使用的上下文关联
    if(avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
        printf("avcodec_open2 failed!\n");
        return 0;
    }

    //打开输入文件
    infile = fopen(input_filename, "rb");
    if(!infile)
    {
        printf("infile fopen failed!\n");
        return 0;
    }

    //输出文件
    outfile = fopen(output_filename, "wb");
    if(!outfile)
    {
        printf("outfilie fopen failed!\n");
        return 0;
    }


    int file_end = 0;
    //读取文件开始解码
    data = inbuf;
    data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, infile);
    de_frame = av_frame_alloc();
    while (data_size > 0)
    {
        //获取传入到avcodec_send_packet一个packet的数据量
        //（一帧的量可能也会多帧的量，这里测试是一帧的量）
        ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size,
                               data, data_size,
                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if(ret < 0)
        {
            printf("av_parser_parser2 Error!\n");
            return 0;
        }
        //使用了多少数据做一个偏移
        data += ret;
        data_size -= ret;

        if(pkt->size)
            decode(codec_ctx, pkt, de_frame, outfile);


        //如果当前缓冲区中数据少于AUDIO_REFILL_THRESH就再读
        //避免多次读文件
        if((data_size < AUDIO_REFILL_THRESH) && !file_end )
        {
            //剩余数据移动缓冲区前
            memmove(inbuf, data, data_size);
            data = inbuf;
            //跨过已有数据存储
            len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, infile);
            if(len > 0)
                data_size += len;
            else if(len == 0)
            {
                file_end = 1;
                printf("file end!\n");
            }
        }

    }



    //冲刷解码器
    pkt->data = NULL;
    pkt->size = 0;
    decode(codec_ctx, pkt, de_frame, outfile);

    fclose(infile);
    fclose(outfile);

    avcodec_free_context(&codec_ctx);
    av_parser_close(parser);
    av_frame_free(&de_frame);
    av_packet_free(&pkt);

    printf("audio decoder end!\n");

}

int main(int argc, char *argv[])
{
    
    char* filename = NULL;
    char* outfilename = NULL;
    if(argc != 3)
    {
        printf("Usage: %s <input file> <output file>\n", argv[0]);

        filename = "test.mp3";
        outfilename = "test.pcm";
    }else{
        filename = argv[1];
        outfilename = argv[2];
    }
    
    parse_aac_to_pcm(filename, outfilename);
    
    return 0;
}

