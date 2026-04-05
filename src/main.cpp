#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <SDL.h>

import ffplay;

int main(int argc, char* argv[]) {

    //VideoState is;
    Read

    std::cout << "FFPlay C++ Rewrite Initialized!" << std::endl;
    
    // Print FFmpeg and SDL versions to verify linking
    std::cout << "FFmpeg avcodec version: " << avcodec_version() << std::endl;
    
    SDL_version compiled;
    SDL_VERSION(&compiled);
    std::cout << "SDL version: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch << std::endl;

    return 0;
}
