#include "window_utils.h"

void toggle_fullscreen(SDL_Window* win) {
    const Uint32 window_flags = SDL_GetWindowFlags(win);
    if (window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(win, 0);
        SDL_SetWindowResizable(win, SDL_TRUE);
        SDL_SetWindowPosition(win, 10, 40);
        SDL_SetWindowSize(win, 1280, 720);
    } else {
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowResizable(win, SDL_FALSE);
    }
}
