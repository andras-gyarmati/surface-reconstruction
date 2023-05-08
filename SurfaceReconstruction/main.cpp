#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <iostream>
#include <sstream>
#include "Includes/GLDebugMessageCallback.h"
#include "application.h"

int main(int argc, char* args[]) {
    // Set up the system to call exitProgram() before exiting
    atexit([]() {
        SDL_Quit();

        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
    });

    // 1. Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
        std::cout << "[SDL init] Error while initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 2. Set up OpenGL requirements, create the window, and start OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    const char* title = "LiDAR Point Cloud Visualizer";
    SDL_Window* win = SDL_CreateWindow(title, 10, 100, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (win == nullptr) {
        std::cout << "[Window creation] Error while initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 3. Create the OpenGL context
    const SDL_GLContext context = SDL_GL_CreateContext(win);
    if (context == 0) {
        std::cout << "[OGL context creation] Error while initializing SDL" << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    // Initialize GLEW
    const GLenum error = glewInit();
    if (error != GLEW_OK) {
        std::cout << "[GLEW] Error while init!" << std::endl;
        return 1;
    }

    // Get OpenGL version
    int glVersion[2] = {-1, -1};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    std::cout << "Running OpenGL " << glVersion[0] << "." << glVersion[1] << std::endl;

    if (glVersion[0] == -1 && glVersion[1] == -1) {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(win);

        std::cout << "[OGL context creation] Could not create OGL context! SDL_GL_SetAttribute(...) calls may have wrong settings." << std::endl;

        return 1;
    }

    std::stringstream window_title;
    window_title << "OpenGL " << glVersion[0] << "." << glVersion[1];
    SDL_SetWindowTitle(win, window_title.str().c_str());

    // Set up the debug callback function if in debug context
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageCallback(GLDebugMessageCallback, nullptr);
    }

    // Initialize ImGui
    ImGui_ImplSdlGL3_Init(win);

    // 4. Start the main event processing loop
    {
        bool quit = false;
        SDL_Event ev;

        application app;
        if (!app.init(win)) {
            SDL_GL_DeleteContext(context);
            SDL_DestroyWindow(win);
            std::cout << "[app.Init] Error while app init!" << std::endl;
            return 1;
        }
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        app.resize(w, h);

        while (!quit) {
            while (SDL_PollEvent(&ev)) {
                ImGui_ImplSdlGL3_ProcessEvent(&ev);
                const bool is_mouse_captured = ImGui::GetIO().WantCaptureMouse;
                const bool is_keyboard_captured = ImGui::GetIO().WantCaptureKeyboard;
                switch (ev.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    }
                    if (!is_keyboard_captured)
                        app.keyboard_down(ev.key);
                    break;
                case SDL_KEYUP:
                    if (!is_keyboard_captured)
                        app.keyboard_up(ev.key);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (!is_mouse_captured)
                        app.mouse_down(ev.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (!is_mouse_captured)
                        app.mouse_up(ev.button);
                    break;
                case SDL_MOUSEWHEEL:
                    if (!is_mouse_captured)
                        app.mouse_wheel(ev.wheel);
                    break;
                case SDL_MOUSEMOTION:
                    if (!is_mouse_captured)
                        app.mouse_move(ev.motion);
                    break;
                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        app.resize(ev.window.data1, ev.window.data2);
                    }
                    break;
                default: break;
                }
            }
            ImGui_ImplSdlGL3_NewFrame(win);

            app.update();
            app.render();
            ImGui::Render();

            SDL_GL_SwapWindow(win);
        }

        app.clean();
    }

    // 5. Exit
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(win);

    return 0;
}
