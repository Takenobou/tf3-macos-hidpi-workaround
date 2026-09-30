#include <stdint.h>
#include <stdio.h>

typedef struct SDL_Window SDL_Window;
extern SDL_Window *SDL_CreateWindow(const char *, int, int, int, int, uint32_t);
extern void SDL_GetWindowSize(SDL_Window *, int *, int *);
extern void SDL_Vulkan_GetDrawableSize(SDL_Window *, int *, int *);
extern const char *SDL_GetError(void);
extern SDL_Window *SDL_GetMouseFocus(void);
extern SDL_Window *SDL_GetWindowFromID(uint32_t);
extern uint32_t SDL_GetMouseState(int *, int *);
extern void SDL_WarpMouseInWindow(SDL_Window *, int, int);

/* Relevant fields of the stable SDL2 event ABI; SDL_PollEvent writes into
 * the caller's complete SDL_Event, which this shim never allocates. */
typedef union {
    uint32_t type;
    struct { uint32_t type, timestamp, window_id; uint8_t event, padding[3];
             int32_t data1, data2; } window;
    struct { uint32_t type, timestamp, window_id, which, state;
             int32_t x, y, xrel, yrel; } motion;
    struct { uint32_t type, timestamp, window_id, which;
             uint8_t button, state, clicks, padding; int32_t x, y; } button;
    struct { uint32_t type, timestamp, window_id, which;
             int32_t x, y; uint32_t direction; float precise_x, precise_y;
             int32_t mouse_x, mouse_y; } wheel;
} Event;
extern int SDL_PollEvent(Event *);

static void get_scale(SDL_Window *window, double *sx, double *sy) {
    *sx = *sy = 1;
    if (!window) return;
    int w, h, pw, ph;
    SDL_GetWindowSize(window, &w, &h);
    SDL_Vulkan_GetDrawableSize(window, &pw, &ph);
    if (w > 0 && h > 0 && pw > 0 && ph > 0) {
        *sx = (double)pw / w;
        *sy = (double)ph / h;
    }
}

static uint32_t get_mouse_state(int *x, int *y) {
    uint32_t buttons = SDL_GetMouseState(x, y);
    double sx, sy;
    get_scale(SDL_GetMouseFocus(), &sx, &sy);
    if (x) *x = (int)(*x * sx);
    if (y) *y = (int)(*y * sy);
    return buttons;
}

static void warp_mouse(SDL_Window *window, int x, int y) {
    double sx, sy;
    get_scale(window ? window : SDL_GetMouseFocus(), &sx, &sy);
    SDL_WarpMouseInWindow(window, (int)(x / sx), (int)(y / sy));
}

static int poll_event(Event *event) {
    int result = SDL_PollEvent(event);
    if (!result || !event) return result;
    if (event->type != 0x200 && (event->type < 0x400 || event->type > 0x403))
        return result;
    SDL_Window *window = SDL_GetWindowFromID(event->window.window_id);
    double sx, sy;
    get_scale(window, &sx, &sy);
    switch (event->type) {
        case 0x200: /* RESIZED and SIZE_CHANGED carry logical dimensions. */
            if (event->window.event == 5 || event->window.event == 6) {
                event->window.data1 = (int)(event->window.data1 * sx);
                event->window.data2 = (int)(event->window.data2 * sy);
            }
            break;
        case 0x400:
            event->motion.x = (int)(event->motion.x * sx);
            event->motion.y = (int)(event->motion.y * sy);
            event->motion.xrel = (int)(event->motion.xrel * sx);
            event->motion.yrel = (int)(event->motion.yrel * sy);
            break;
        case 0x401:
        case 0x402:
            event->button.x = (int)(event->button.x * sx);
            event->button.y = (int)(event->button.y * sy);
            break;
        case 0x403:
            event->wheel.mouse_x = (int)(event->wheel.mouse_x * sx);
            event->wheel.mouse_y = (int)(event->wheel.mouse_y * sy);
            break;
    }
    return result;
}

/* TF3 uses window coordinates as render pixels. Give its size query the
 * drawable dimensions, then translate mouse input into that same space. */
static void get_pixel_size(SDL_Window *window, int *width, int *height) {
    SDL_Vulkan_GetDrawableSize(window, width, height);
}

static SDL_Window *create_window(const char *title, int x, int y,
                                 int width, int height, uint32_t flags) {
    fprintf(stderr, "[hidpi] SDL_CreateWindow flags=0x%x size=%dx%d; adding ALLOW_HIGHDPI\n",
            flags, width, height);
    SDL_Window *window = SDL_CreateWindow(title, x, y, width, height, flags | 0x2000u);
    if (window) {
        int w, h, pw, ph;
        SDL_GetWindowSize(window, &w, &h);
        SDL_Vulkan_GetDrawableSize(window, &pw, &ph);
        fprintf(stderr, "[hidpi] window=%dx%d drawable=%dx%d\n", w, h, pw, ph);
    } else {
        fprintf(stderr, "[hidpi] creation failed: %s\n", SDL_GetError());
    }
    return window;
}

__attribute__((used)) static const struct {
    const void *replacement;
    const void *original;
} interposers[] __attribute__((section("__DATA,__interpose"))) = {
    {(const void *)create_window, (const void *)SDL_CreateWindow},
    {(const void *)get_pixel_size, (const void *)SDL_GetWindowSize},
    {(const void *)get_mouse_state, (const void *)SDL_GetMouseState},
    {(const void *)warp_mouse, (const void *)SDL_WarpMouseInWindow},
    {(const void *)poll_event, (const void *)SDL_PollEvent},
};
