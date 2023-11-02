#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

void getAdstHeader(unsigned char* adtsHeader,AVCodecContext *ctx, int len)

{
    int sampleRate = ctx->sample_rate;
    int freqIdx = 0; //默认为0
    switch (sampleRate)
    {   case 96000: freqIdx = 0; break;
        case 88200: freqIdx = 1; break;
        case 64000: freqIdx = 2; break;
        case 48000: freqIdx = 3; break;
        case 44100: freqIdx = 4; break;
        case 32000: freqIdx = 5; break;
        case 24000: freqIdx = 6; break;
        case 22050: freqIdx = 7; break;
        case 16000: freqIdx = 8; break;
        case 12000: freqIdx = 9; break;
        case 11025: freqIdx = 10; break;
        case 8000: freqIdx = 11; break;
        case 7350: freqIdx = 12; break;
        default: freqIdx = 15; break;//如果都不是默认为15

    }

	
    
	int packetLen=len+7;//ADTS头加AAC数据总长度
    int profile = ctx->profile;
    int chanCfg = ctx->ch_layout.nb_channels;
	adtsHeader[0] = (unsigned char) 0xFF;
    adtsHeader[1] = (unsigned char) 0xF9;
    adtsHeader[2] = (unsigned char) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    adtsHeader[3] = (unsigned char) (((chanCfg & 3) << 6) + (packetLen >> 11));
    adtsHeader[4] = (unsigned char) ((packetLen & 0x7FF) >> 3);
    adtsHeader[5] = (unsigned char) (((packetLen & 7) << 5) + 0x1F);
    adtsHeader[6] = (unsigned char) 0xFC;
	
	

}

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec, AVChannelLayout *dst)
{
    const AVChannelLayout *p, *best_ch_layout;
    int best_nb_channels   = 0;

    if (!codec->ch_layouts)
        return av_channel_layout_copy(dst, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);

    p = codec->ch_layouts;
    while (p->nb_channels) {
        int nb_channels = p->nb_channels;

        if (nb_channels > best_nb_channels) {
            best_ch_layout   = p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return av_channel_layout_copy(dst, best_ch_layout);
}

static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *output)
{
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        exit(1);
    }

    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
        uint8_t adts_header_buf[7];
        getAdstHeader(&adts_header_buf, ctx, pkt->size);
        
        ret = fwrite(adts_header_buf, 1, 7, output);
        if (ret < 7) {
            fprintf(stderr, "Error while writing adts header\n");
            exit(1);
        }


        ret = fwrite(pkt->data, 1, pkt->size, output);
        if (ret < pkt->size) {
            fprintf(stderr, "Error while writing adts header\n");
            exit(1);
        }
        av_packet_unref(pkt);
    }
}

void f32le_to_fltp(float * f32le, float * fltp, int nb_samples)
{
    int i, ch;
    float *r = fltp+nb_samples;
    for (i = 0; i < nb_samples; i++) {
        fltp[i] = f32le[i * 2];
        r[i] = f32le[i * 2 + 1];
    }
}

int main(int argc, char **argv)
{

    const char *in_filename;
    const char *out_filename;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    AVFrame *frame;
    AVPacket *pkt;
    int i, j, k, ret=0;
    FILE *f,*in_f;
    uint16_t *samples;
    float t, tincr;

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
        in_filename = "test.pcm";
        out_filename = "test.aac";
    }else{
        in_filename = argv[1];
        out_filename = argv[2];
        if(strstr(in_filename,".pcm") == NULL){
            fprintf(stderr, "input file must be pcm file\n");
            exit(1);
        }
        if(strstr(out_filename,".aac") == NULL){
            fprintf(stderr, "output file must be aac file\n");
            exit(1);
        }
    }
   

    /* find the aac encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);

    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* put sample parameters */
    c->bit_rate = 64000;

    /* check that the encoder supports fltp pcm input */
    c->sample_fmt = AV_SAMPLE_FMT_FLTP;
    if (!check_sample_fmt(codec, c->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(c->sample_fmt));
        exit(1);
    }

    /* select other audio parameters supported by the encoder */
    c->sample_rate =select_sample_rate(codec);

    c->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
   

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(out_filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", out_filename);
        exit(1);
    }

    /* packet for holding encoded output */
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame->nb_samples     = c->frame_size;
    frame->format         = c->sample_fmt;

    ret = av_channel_layout_copy(&frame->ch_layout, &c->ch_layout);
    if (ret < 0)
        exit(1);

    /* allocate the data buffers */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

    in_f = fopen(in_filename, "rb");
    if(in_f<0){
        fprintf(stderr, "Could not open %s\n", in_filename);
        exit(1);
    }
    size_t frame_size = av_samples_get_buffer_size(&frame->linesize, frame->ch_layout.nb_channels, frame->nb_samples, AV_SAMPLE_FMT_FLTP, 0);
    float* in_buf = (float*)malloc(frame_size);
    float* in_tmp_buf = (float*)malloc(frame_size);
    while(1){
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);
        int read_bytes = fread(in_buf, 1, frame_size, in_f);

        if(read_bytes <= 0){
            break;
        }
        memset(in_tmp_buf, 0, frame_size);
        f32le_to_fltp(in_buf, in_tmp_buf, frame->nb_samples);

        av_samples_fill_arrays(frame->data, frame->linesize, in_tmp_buf, frame->ch_layout.nb_channels, frame->nb_samples, frame->format, 0);
        
        encode(c, frame, pkt, f);
    }
        
    // t = 0;
    // tincr = 2 * M_PI * 440.0 / c->sample_rate;
    // for (i = 0; i < 200; i++) {
    //     /* make sure the frame is writable -- makes a copy if the encoder
    //      * kept a reference internally */
    //     ret = av_frame_make_writable(frame);
    //     if (ret < 0)
    //         exit(1);
    //     samples = (uint16_t*)frame->data[0];

    //     for (j = 0; j < c->frame_size; j++) {
    //         samples[2*j] = (int)(sin(t) * 10000);

    //         for (k = 1; k < c->ch_layout.nb_channels; k++)
    //             samples[2*j + k] = samples[2*j];
    //         t += tincr;
    //     }
    //     encode(c, frame, pkt, f);
    // }

    /* flush the encoder */
    encode(c, NULL, pkt, f);

    fclose(f);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);

    return 0;
}
