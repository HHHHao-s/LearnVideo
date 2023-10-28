#include <SDL.h>
#include <random>
#include <iostream>


#undef main
int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("SDL2 Video Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, 640, 480);

   
    SDL_Rect rect;
    rect.w = 50;
    rect.h = 50;
    int run = 1;
    while (run) {

        rect.x = rand()% 640;
        rect.y = rand()% 480;

        

        SDL_SetRenderTarget(renderer, texture); // Set the target to our texture
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear the texture with black color
        SDL_RenderClear(renderer); // Update the screen with cleared texture

        SDL_RenderDrawRect(renderer, &rect); // Draw the rect to the texture
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Set color of rect to red
        SDL_RenderFillRect(renderer, &rect); // Fill the texture with red color

        SDL_SetRenderTarget(renderer, NULL); // Reset the target to the default
        SDL_RenderCopy(renderer, texture, NULL, NULL); // Copy the texture to the renderer

        SDL_RenderPresent(renderer); // Update the screen
        SDL_Delay(300);
        

    }



    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}