#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <SDL.h>
import SDL.Window;
import SDL.Renderer;
import ffplay;


constexpr static auto program_name = "ffplay-cpp";
int main(int argc, char* argv[]) 
{
    Options options;

    SDL::Window window{ program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, options.screen_width, options.screen_height, 0 };
    SDL::Renderer renderer{ window.Get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC };


    Player is{ std::move(options) };
    if (is.Open("https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/1080/Big_Buck_Bunny_1080_10s_1MB.mp4") < 0)
        return 1;

    is.EventLoop(window.Get(), renderer.Get());

    std::cout << "FFPlay C++ Rewrite Initialized!" << std::endl;
    
    // Print FFmpeg and SDL versions to verify linking
    std::cout << "FFmpeg avcodec version: " << avcodec_version() << std::endl;
    
    SDL_version compiled;
    SDL_VERSION(&compiled);
    std::cout << "SDL version: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch << std::endl;

    return 0;
}
