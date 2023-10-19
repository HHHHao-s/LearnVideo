# `<center>`音视频学习笔记 `</center>`

# 1. ffmpeg

## 1.1 ffmpeg九宫格

```powershell
ffmpeg -i 1.mp4 -i 1.mp4 -i 1.mp4 -i 1.mp4 -filter_complex "nullsrc=size=1280x720[base];[0:v]setpts=PTS-STARTPTS,scale=640x360[lu];[1:v]setpts=PTS-STARTPTS,scale=640x360[ru];[2:v]setpts=PTS-STARTPTS,scale=640x360[ld];[3:v]setpts=PTS-STARTPTS,scale=640x360[rd];[base][lu]overlay=shortest=1[tmp1];[tmp1][ru]overlay=shortest=1:x=640[tmp2];[tmp2][ld]overlay=shortest=1:y=360[tmp3];[tmp3][rd]overlay=shortest=1:x=640:y=360" -y out.mp4
```

# 2. SDL

直接下载[SDL2](https://github.com/libsdl-org/SDL/releases)，然后用cmake引入就可以使用

![1697698638002](image/README/1697698638002.png)
