module;
extern "C" {
#include <libavutil/frame.h>
}

export module utils.Frame;

export struct Frame
{
    AVFrame* frame{};
    //AVSubtitle sub;
    int serial{};
    bool uploaded{};
    double pts{};           /* presentation timestamp for the frame */
    double duration{};      /* estimated duration of the frame */
    int64_t pos{};          /* byte position of the frame in the input file */
    int width{};
    int height{};
    int format{};
    AVRational sample_aspect_ratio;

    constexpr bool flip_v() const
    {
        return frame->linesize[0] < 0;
    }

    constexpr void fill(AVFrame* frame)
    {
        sample_aspect_ratio = frame->sample_aspect_ratio;
        width = frame->width;
        height = frame->height;
        format = frame->format;
    }
};