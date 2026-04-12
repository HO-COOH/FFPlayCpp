#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <print>
#include <SDL.h>
import ffplay;


int main(int argc, char* argv[]) 
{
    std::println("FFmpeg avcodec version: {}", avcodec_version());

    SDL_version compiled;
    SDL_VERSION(&compiled);
    std::println("SDL version: {}.{}.{}", (int)compiled.major, (int)compiled.minor, (int)compiled.patch);

    Options options;

    Player is{ std::move(options) };
    if (is.Open("https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4") < 0)
    {
        std::cerr << "Open video failed\n";
        return 1;
    }

    is.EventLoop();

    return 0;
}
