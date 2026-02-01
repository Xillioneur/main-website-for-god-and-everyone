#include <SDL.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
bool running = true;

int rectX = 50;
int rectY = 50;
const int RECT_SPEED = 5;

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:  rectX -= RECT_SPEED; break;
                case SDLK_RIGHT: rectX += RECT_SPEED; break;
                case SDLK_UP:    rectY -= RECT_SPEED; break;
                case SDLK_DOWN:  rectY += RECT_SPEED; break;
            }
        }
    }

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black background
    SDL_RenderClear(renderer);

    SDL_Rect fillRect = { rectX, rectY, 100, 100 };
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); // Red rectangle
    SDL_RenderFillRect(renderer, &fillRect);

    SDL_RenderPresent(renderer);

#ifdef __EMSCRIPTEN__
    if (!running) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char* args[]) {
#ifdef __EMSCRIPTEN__
    SDL_SetMainReady(); // Inform SDL that the main thread is ready
#endif
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Hint SDL to disable vsync, letting Emscripten manage timing
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    window = SDL_CreateWindow("SDL2 Emscripten Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 60, 1);
#else
    while (running) {
        main_loop();
    }
#endif

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "SDL application exited." << std::endl; // This will print to browser console
    return 0;
}
