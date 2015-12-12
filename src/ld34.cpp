
#include <SDL2/SDL.h>
#include <SDL2/SDL_Image.h>
#include <SDL2/SDL_Mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720
#define WINDOW_TITLE    "LD 34\0"
#define BYTES_PER_PIXEL 4
#define TIME_PER_FRAME  0.03333

#define internal        static
#define global          static

typedef int8_t          i8;
typedef int16_t         i16;
typedef int32_t         i32;
typedef int64_t         i64;

typedef uint8_t         u8;
typedef uint16_t        u16;
typedef uint32_t        u32;
typedef uint64_t        u64;

typedef float           f32;
typedef double          f64;

union V2f32 {
    struct {
        f32 x;
        f32 y;
    };

    struct {
        f32 w;
        f32 h;
    };
};

union V2i32 {
    struct {
        i32 x;
        i32 y;
    };

    struct {
        i32 w;
        i32 h;
    };
};

struct V3f32 {
    f32 x;
    f32 y;
    f32 z;
};

union V4f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };

    struct {
        f32 a;
        f32 r;
        f32 g;
        f32 b;
    };
};

struct Display {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* render_texture;
    SDL_PixelFormat pixel_format;
    void* pixels_buffer;
    i32 buffer_pitch;
    V2i32 size;
    char* title;
};

struct Texture {
    void* pixels;
    V2i32 size;
};

void WritePixelsToDisplay(void* pixels, Display* display, i32 w, i32 h, i32 x, i32 y) {
    for(i32 ya = y, py = 0; ya < y + h; ya++, py++) {
        for(i32 xa = x, px = 0; xa < x + w; xa++, px++) {
            u32 pixel = *((u32*)pixels + (py * w) + px);

            if(pixel != 0x00FF00FF) {
                *((u32*)display->pixels_buffer + (ya * display->size.w) + xa) = pixel;
            }
        }
    }
}

void LoadTextureFromFile(Texture* texture, char* path, SDL_PixelFormat* format) {
    SDL_Surface* image = NULL;

    image = IMG_Load(path);
    if(!image) {
        printf("Warning - could not load image '%s'.\n", path);
        return;
    }

    SDL_Surface* proper_image = SDL_ConvertSurface(image, format, 0);
    if(!proper_image) {
        printf("Warning - could not convert image. %s.\n", SDL_GetError());
        return;
    }

    texture->pixels = proper_image->pixels;
    texture->size = {proper_image->w, proper_image->h};
}

// TODO (Mathew): Maybe make the x and y the grid location rather than pixel location?
void SpliceTexture(Texture* src, Texture* dst, i32 x, i32 y, i32 w, i32 h, bool flip_x, bool flip_y) {
    dst->size = {w, h};
    dst->pixels = malloc(dst->size.w * dst->size.h * BYTES_PER_PIXEL);

    if(flip_x && !flip_y) {
        for(i32 ya = y, py = 0; ya < y + h; ya++, py++) {
            for(i32 xa = x + w - 1, px = 0; xa >= x; xa--, px++) {
                *((u32*)dst->pixels + (py * w) + px) = *((u32*)src->pixels + (ya * src->size.w) + xa);
            }
        }
    }
    else if(flip_y && !flip_x) {
        for(i32 ya = y + h - 1, py = 0; ya >= y; ya--, py++) {
            for(i32 xa = x, px = 0; xa < x + w; xa++, px++) {
                *((u32*)dst->pixels + (py * w) + px) = *((u32*)src->pixels + (ya * src->size.w) + xa);
            }
        }
    }
    else if(flip_x && flip_y) {
        for(i32 ya = y + h - 1, py = 0; ya >= y; ya--, py++) {
            for(i32 xa = x + w - 1, px = 0; xa >= x; xa--, px++) {
                *((u32*)dst->pixels + (py * w) + px) = *((u32*)src->pixels + (ya * src->size.w) + xa);
            }
        }
    }
    else {
        for(i32 ya = y, py = 0; ya < y + h; ya++, py++) {
            for(i32 xa = x, px = 0; xa < x + w; xa++, px++) {
                *((u32*)dst->pixels + (py * w) + px) = *((u32*)src->pixels + (ya * src->size.w) + xa);
            }
        }
    }
}

void ScaleTexture(Texture* texture, i32 sx, i32 sy) {
    Texture new_tex;
    new_tex.size = {texture->size.w * sx, texture->size.h * sy};
    new_tex.pixels = malloc(new_tex.size.w * new_tex.size.h * BYTES_PER_PIXEL);

    for(i32 y = 0; y < texture->size.h; y++) {
        for(i32 x = 0; x < texture->size.w; x++) {
            for(i32 yy = 0; yy < sy; yy++) {
                for(i32 xx = 0; xx < sx; xx++) {
                    i32 dst_offset = (((y * sy) + yy) * new_tex.size.w) + (x * sx) + xx;
                    i32 src_offset = (y * texture->size.w) + x;

                    *((u32*)new_tex.pixels + dst_offset) = *((u32*)texture->pixels + src_offset);
                }
            }
        }
    }

    *texture = new_tex;
}

int main(int argc, char** argv) {
    Display display = {};
    display.size = {1280, 720};
    display.buffer_pitch = display.size.w * BYTES_PER_PIXEL;
    display.title = "LD 34\0";

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error - Could not init SDL.\n");
        return -1;
    }

    int flags = IMG_INIT_PNG;
    if(IMG_Init(flags) & flags != flags) {
        printf("Error - Could not init SDL_Image.\n");
        return -1;
    }

    flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    if(Mix_Init(flags) & flags != flags) {
        printf("Error - Could not init SDL_Mixer.\n");
        return -1;
    }

    display.window = SDL_CreateWindow(display.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display.size.w, display.size.h, SDL_WINDOW_SHOWN);
    if(!display.window) {
        printf("Error - Could not create window. %s.\n", SDL_GetError());
        return -1;
    }

    display.pixel_format = *SDL_GetWindowSurface(display.window)->format;

    display.renderer = SDL_CreateRenderer(display.window, -1, SDL_RENDERER_ACCELERATED);
    if(!display.renderer) {
        printf("Error - Could not create renderer.\n");
        return -1;
    }

    display.render_texture = SDL_CreateTexture(display.renderer, display.pixel_format.format, SDL_TEXTUREACCESS_STREAMING, display.size.w, display.size.h);
    if(!display.render_texture) {
        printf("Error - Could not create render_texture. %s.\n", SDL_GetError());
        return -1;
    }

    display.pixels_buffer = malloc(display.size.w * display.size.h * BYTES_PER_PIXEL);
    if(!display.pixels_buffer) {
        printf("Error - Could not create pixel_buffer. %s.\n", SDL_GetError());
        return -1;
    }

    Texture texture_sheet;
    LoadTextureFromFile(&texture_sheet, "../res/graphics/textures.png", &display.pixel_format);

    Texture face;
    SpliceTexture(&texture_sheet, &face, 96, 32, 32, 32, false, false);
    ScaleTexture(&face, 4, 4);

    f32 last_time = SDL_GetTicks() / 1000.0f;
    f32 avg_time_timer = 0;
    i32 frames = 0;
    f32 avg_time = 0.0f;
    bool running = true;
    SDL_Event event;
    while(running) {
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT: {
                    running = false;
                } break;
            }
        }

        f32 current_time = SDL_GetTicks() / 1000.0f;
        f32 time_this_frame = current_time - last_time;
        last_time = current_time;

        avg_time_timer += time_this_frame;
        avg_time += time_this_frame;
        frames++;

        if(avg_time_timer >= 1.0f) {
            printf("%.3f avg ms/frame\n", (avg_time / frames) * 1000);
            frames = 0;
            avg_time = 0;
            avg_time_timer = 0;
        }

        if(time_this_frame < TIME_PER_FRAME) {
            i32 ms_to_sleep = (TIME_PER_FRAME - time_this_frame) * 1000;
            SDL_Delay(ms_to_sleep);
        }
        else {
            printf("Warning - this frame took %.3f ms. Targeted ms/frame is %.3f.\n", time_this_frame * 1000.0f, TIME_PER_FRAME * 1000.0f);
        }

        SDL_RenderClear(display.renderer);
        for(i32 i = 0; i < display.size.w * display.size.h; i++) {
            *((u32*)display.pixels_buffer + i) = 0xFFFFFFFF;
        }

        WritePixelsToDisplay(face.pixels, &display, face.size.w, face.size.h, 100, 100);

        SDL_LockTexture(display.render_texture, NULL, &display.pixels_buffer, &display.buffer_pitch);
        SDL_UnlockTexture(display.render_texture);
        SDL_RenderCopy(display.renderer, display.render_texture, NULL, NULL);
        SDL_RenderPresent(display.renderer);
    }

    return 0;
}