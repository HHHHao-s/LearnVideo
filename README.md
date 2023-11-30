# <center>音视频学习笔记 </center>

## 零、 vscode

### 配置cmake
在c_cpp_properties.json中加入这一行

`"configurationProvider": "ms-vscode.cmake-tools"`

### WORKING_DIRECTORY
`set_target_properties(${EXECUTABLES} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
`

## 一、 FFmpeg

### FFmpeg examples

[FFmpeg examples](https://ffmpeg.org/doxygen/trunk/examples.html)

### ffmpeg九宫格

```powershell
ffmpeg -i 1.mp4 -i 1.mp4 -i 1.mp4 -i 1.mp4 -filter_complex "nullsrc=size=1280x720[base];[0:v]setpts=PTS-STARTPTS,scale=640x360[lu];[1:v]setpts=PTS-STARTPTS,scale=640x360[ru];[2:v]setpts=PTS-STARTPTS,scale=640x360[ld];[3:v]setpts=PTS-STARTPTS,scale=640x360[rd];[base][lu]overlay=shortest=1[tmp1];[tmp1][ru]overlay=shortest=1:x=640[tmp2];[tmp2][ld]overlay=shortest=1:y=360[tmp3];[tmp3][rd]overlay=shortest=1:x=640:y=360" -y out.mp4
```

### FFmpeg内存模型

![](README/16170208_616a95107eb9f94451.webp)

### AVPacket 相关

```c
AVPacket *av_packet_alloc(void);
void av_packet_free(AVPacket **pkt);
AVPacket *av_packet_clone(const AVPacket *src);
int av_new_packet(AVPacket *pkt, int size);
int av_packet_ref(AVPacket *dst, const AVPacket *src);
void av_packet_unref(AVPacket *pkt);
void av_packet_move_ref(AVPacket *dst, AVPacket *src);
int av_packet_make_refcounted(AVPacket *pkt);
```
流程在[test1](src/ffmpegTester.cpp)中

### AVFormat 相关

```c
int avformat_open_input(AVFormatContext **ps, const char *url,
                        const AVInputFormat *fmt, AVDictionary **options);
void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output);
AVPacket *av_packet_alloc(void);
int av_read_frame(AVFormatContext *s, AVPacket *pkt);
int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp, int flags);
```

### AVBitStreamFilter 相关

```c
// Query
const AVBitStreamFilter *av_bsf_next(void **opaque);
const AVBitStreamFilter *av_bsf_get_by_name(const char *name);

// Setup
int av_bsf_alloc(const AVBitStreamFilter *filter, AVBSFContext **ctx);
int av_bsf_init(AVBSFContext *ctx);

// Usage
// 会移动pkt到bsf内部
int av_bsf_send_packet(AVBSFContext *ctx, AVPacket *pkt);
// 会将数据从bsf内部移动到pkt
int av_bsf_receive_packet(AVBSFContext *ctx, AVPacket *pkt);

// Cleanup
void av_bsf_free(AVBSFContext **ctx);
```
在[test3](src/ffmpegTester.cpp)中

### AAC提取PCM

[AAC提取PCM](https://blog.csdn.net/m0_37599645/article/details/112286537)

在[parse_pcm.cpp](src/parse_pcm.cpp)中

播放：`ffplay -ar 48000 -ac 2 -f f32le test.pcm`

### H264提取YUV420P

[H264提取YUV420P](https://blog.csdn.net/asdasfdgdhh/article/details/109777820)

在[parse_yuv420p.cpp](src/parse_yuv.cpp)中

播放: `ffplay -pixel_format yuv420p -video_size 1280x720  -i test.yuv`

### 使用AVIO

[AVIO](https://zhuanlan.zhihu.com/p/501294281)

在[07-09-avio.cpp](src/07-09-avio.cpp)中

```c
AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));


//创建AVIOContext
AVIOContext *avio_ctx = avio_alloc_context(inbuf, AUDIO_INBUF_SIZE, 0, (void *)in_file, (void *)&read_file, NULL, NULL);


AVFormatContext *fmt_ctx = avformat_alloc_context();
fmt_ctx->pb = avio_ctx;
// 不需要手动设置，avformat_open_input会自动设置
// fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;



// 调用av_read_frame和avformat_open_input时，会自动调用avio_ctx->read_packet
// 可以在read_packet函数中从内存读取数据
ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
ret = av_read_frame(fmt_ctx, pkt);

// 必须这样释放，否则会内存泄漏
// 自定义avio不会把里面的pb释放掉
// fmt_ctx会free掉
avformat_close_input(&fmt_ctx);

/* note: the internal buffer could have changed, and be != avio_ctx_buffer */
if (avio_ctx)
    av_freep(&avio_ctx->buffer);
avio_context_free(&avio_ctx);

```

### 音频重采样

[resample_audio.c](src/resample_audio.c)

```c
/**
 * Gets the delay the next input sample will experience relative to the next output sample.
 *
 * Swresample can buffer data if more input has been provided than available
 * output space, also converting between sample rates needs a delay.
 * This function returns the sum of all such delays.
 * The exact delay is not necessarily an integer value in either input or
 * output sample rate. Especially when downsampling by a large value, the
 * output sample rate may be a poor choice to represent the delay, similarly
 * for upsampling and the input sample rate.
 *
 * @param s     swr context
 * @param base  timebase in which the returned delay will be:
 *              @li if it's set to 1 the returned delay is in seconds
 *              @li if it's set to 1000 the returned delay is in milliseconds
 *              @li if it's set to the input sample rate then the returned
 *                  delay is in input samples
 *              @li if it's set to the output sample rate then the returned
 *                  delay is in output samples
 *              @li if it's the least common multiple of in_sample_rate and
 *                  out_sample_rate then an exact rounding-free delay will be
 *                  returned
 * @returns     the delay in 1 / @c base units.
 */
int64_t swr_get_delay(struct SwrContext *s, int64_t base);

/**
 * @}
 *
 * @name Core conversion functions
 * @{
 */

/** Convert audio.
 *
 * in and in_count can be set to 0 to flush the last few samples out at the
 * end.
 *
 * If more input is provided than output space, then the input will be buffered.
 * You can avoid this buffering by using swr_get_out_samples() to retrieve an
 * upper bound on the required number of output samples for the given number of
 * input samples. Conversion will run directly without copying whenever possible.
 *
 * @param s         allocated Swr context, with parameters set
 * @param out       output buffers, only the first one need be set in case of packed audio
 * @param out_count amount of space available for output in samples per channel
 * @param in        input buffers, only the first one need to be set in case of packed audio
 * @param in_count  number of input samples available in one channel
 *
 * @return number of samples output per channel, negative value on error
 */
int swr_convert(struct SwrContext *s, uint8_t **dst_data, int dst_nb_samples,
                                const uint8_t **src_data , int src_nb_samples);

// 每次调用swr_convert时，都会返回一个size，这个size<=dst_nb_samples，于是，需要调用swr_get_delay加到src_nb_samples上以获取更多的dst_nb_samples
// 这样下次一调用swr_convert时，dst_nb_samples就会增加
int64_t delay = swr_get_delay(swr_ctx, src_rate);
        dst_nb_samples = av_rescale_rnd( delay+
                                        src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);

swr_convert(swr_ctx, dst_data, dst_nb_samples, NULL, 0);可以flush剩余的数据

```



### 编码PCM

[encode_pcm_aac.c](src/encode_pcm_aac.c)

```c
// 这项要和源文件的采样率一致，不然编码后的aac文件会出现变速的情况
c->sample_rate =select_sample_rate(codec);
//例如使用 ffmpeg -i video.mp4 -vn -ar 44100 -ac 2 -f f32le test.pcm 编码时，需要设置 c->sample_rate = 44100;

// 此函数会正确的设置frame->data指向in_tmp_buf中对应的位置
av_samples_fill_arrays(frame->data, frame->linesize, in_tmp_buf, frame->ch_layout.nb_channels, frame->nb_samples, frame->format, 0);

// 使用ffplay test.aac就可以播放了，不需要指定参数

```


### 编码yuv420p

提取yuv:
`ffmpeg -i video.mp4 -c:v rawvideo -pix_fmt yuv420p test_yuv420p_1280x720.yuv`

播放yuv:
`ffplay -f rawvideo -pixel_format yuv420p -video_size 1280x720 .\test_yuv420p_1280x720.yuv`

[encode_video](src/encode_video.c)


### 合成音频和视频到flv

[mux](src/mux.c)

I find a problem in the example file mux.c, I need some help about modifing the codec of a muxer

```c
// The filename is xxx.flv
// The default codec for flv is AV_CODEC_ID_ADPCM_SWF and AV_CODEC_ID_FLV1
// If I want to modify the codec I need to copy the entire value of return pointer of av_guess_format() like this, since it returns a const pointer
// If I modify the codec like this, it will cause a segmentation fault when I call avformat_write_header(), because it didn't copy the write_header function pointer in private fileds
AVOutputFormat av_flv_format;
av_flv_format = *(AVOutputFormat*)av_guess_format(NULL, filename, NULL);
av_flv_format.audio_codec = AV_CODEC_ID_AAC;
av_flv_format.video_codec = AV_CODEC_ID_H264;

// Then I try to copy the whole FFOutputFormat to modify the codec
// Because I can't include the mux.h file so I need to copy the definition of FFOutputFormat
FFOutputFormat ff_flv_format;
ff_flv_format = *(FFOutputFormat*)av_guess_format(NULL, filename, NULL);
ff_flv_format.p.audio_codec = AV_CODEC_ID_AAC;
ff_flv_format.p.video_codec = AV_CODEC_ID_H264;
// At this time, it works
avformat_alloc_output_context2(&oc,&ff_flv_format, NULL, filename);
// what should I do if I want to modify the codec?
```


### 合成MP4

[mux_mp4](src/mux_mp4.cpp)

## 二、 SDL

直接下载[SDL2](https://github.com/libsdl-org/SDL/releases)，然后用cmake引入就可以使用

![1697698638002](README/1697698638002.png)

## 三、 格式

### FLV

![flv](README/flv.webp)

[FlvParser](https://blog.csdn.net/weixin_41643938/article/details/124537035)


### MP4

[MP4格式分析](https://zhuanlan.zhihu.com/p/355803589)

#### 获取mp4文件的帧率fps

[hexinator文件](README/video_mp4_hex.txt)

[mediainfo文件](README/video_mp4_mediainfo.txt)

1.帧率(每帧播放的时间) = 电影的总时间duration/(电影的总帧数*时间刻度timescale)

2.电影的总时间duration获取和时间刻度timescale的获取：

mvhd Box

00 00 00 6C:  size,mvhd Box的大小为108个字节

6D 76 68 64:  mvhd

00 00 00 00:  version,flags

BE EE 87 C5:  creation-time创建文件的时间,是距离1904年1月1日0点的秒数

BE EE 87 C5:  modifiation-time修改文件的时间，

00 01 5F 90:  timescale时标，时间刻度的单位，为1/90 000秒

01 A8 61 B8:  duration持续的timescale

00 01 00 00:  rate播放描述的等级

01 00:    volume播放时的音量


3.电影的总帧数获取

00 00 00 14:  size of stsz,20

73 74 73 7A:  stsz

00 00 00 00:  version

00 00 00 12:  sample-size

00 00 00 01:  总帧数

4.电影的一帧播放时长

DAFBF        Name:                            stts

DAFC3       Version:                          0 (0x00)

DAFC4       Flags:                            0 (0x000000)

DAFC7       Number of entries:                1 (0x00000001)

DAFCB       Sample Count:                     250 (0x000000FA)

DAFCF       Sample Duration:                  512 (0x00000200)

**Sample Duration/Time Scale = 512/128000 = 0.04秒**

## 四、 协议

### HLS(Http Live Streaming) 协议、TS流、M3U8文件

[HLS specification](https://datatracker.ietf.org/doc/html/draft-pantos-http-live-streaming-23)

[HLS 简介](https://zhuanlan.zhihu.com/p/608628539)

[TS 文件格式](https://blog.csdn.net/andylao62/article/details/120019483)

## 五、 SRS

### 播放HLS

使用http的端口播放（8080）m3u8文件，例如：`http://localhost:8080/live/livestream.m3u8`

使用listen端口通过rtmp传输文件，例如：`ffmpeg -i test.mp4 -c copy -f flv rtmp://localhost:1935/live/livestream`

### 推流rtmp 
`ffmpeg -re -stream_loop -1 -i sync-aac.flv -f flv -c copy -flvflags no_duration_filesize rtmp://127.0.0.1:19353/live/stream`