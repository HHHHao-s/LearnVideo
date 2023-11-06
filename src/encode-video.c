/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file libavcodec encoding video API usage example
 * @example encode_video.c
 *
 * Generate synthetic video data and encode it to an output file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char **argv)
{
    const char *in_filename, *out_filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i=0, ret, x, y;
    FILE *in_f, *out_f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file> <codec name>\n", argv[0]);
        in_filename = "test_yuv420p_1280x720.yuv";
        out_filename = "test.h264";
        codec_name = "libx264";
    }else{
        in_filename = argv[1];
        out_filename = argv[2];
        codec_name = argv[3];
    }


    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 1280;
    c->height = 720;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 25;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    // c->thread_count = 4;

    // c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;// 会失去sps，pps

    if (codec->id == AV_CODEC_ID_H264){
        av_opt_set(c->priv_data, "preset", "slow", 0);// 越慢质量越高，压缩速度越慢，最后大小接近 
        // av_opt_set(c->priv_data, "tune", "zerolatency", 0);// 直播时设置
        av_opt_set(c->priv_data, "tune", "film",0);
        av_opt_set(c->priv_data, "profile", "main",0);// 画质选项
    }
        

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }

    in_f = fopen(in_filename, "rb");
    if (!in_f) {
        fprintf(stderr, "Could not open %s\n", in_filename);
        exit(1);
    }

    out_f = fopen(out_filename, "wb");
    if (!out_f) {
        fprintf(stderr, "Could not open %s\n", out_filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }
    int align = 1;// 按1字节对齐
    size_t size = av_image_get_buffer_size(c->pix_fmt, c->width, c->height, align);
    /* encode video */
    char* buf = av_malloc(size);
    
    while(1){
        fflush(stdout);

        /* Make sure the frame data is writable.
           On the first round, the frame is fresh from av_frame_get_buffer()
           and therefore we know it is writable.
           But on the next rounds, encode() will have called
           avcodec_send_frame(), and the codec may have kept a reference to
           the frame in its internal structures, that makes the frame
           unwritable.
           av_frame_make_writable() checks that and allocates a new buffer
           for the frame only if necessary.
         */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        /* read raw video frame */
        
        ret = fread(buf, 1, size, in_f);
        if (ret <= 0){
            perror("fread ");
            break;
        } 

        ret = av_image_fill_arrays(frame->data, frame->linesize, buf, c->pix_fmt, c->width, c->height, 1);
        if(ret < 0){
            fprintf(stderr, "%s\n", av_err2str(ret));
            exit(1);
        }
        

        frame->pts = i+=40;

        /* encode the image */
        encode(c, frame, pkt, out_f);
    }

    /* flush the encoder */
    encode(c, NULL, pkt, out_f);

    /* Add sequence end code to have a real MPEG file.
       It makes only sense because this tiny examples writes packets
       directly. This is called "elementary stream" and only works for some
       codecs. To create a valid file, you usually need to write packets
       into a proper file format or protocol; see mux.c.
     */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), out_f);
    fclose(in_f);
    fclose(out_f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    // getchar();

    return 0;
}
