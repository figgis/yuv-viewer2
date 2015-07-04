#include "SDL.h"
#include <stdio.h>

#define P(x) printf("%s: %d\n", #x, x)

enum formats {
    YV12 = 0,
    IYUV,
    YUY2,
    UYVY,
    YVYU,
};

struct config {
    Uint32 format;            /* */
    Uint32 pixel_format;      /* */
    size_t width;             /* frame width - in pixels */
    size_t height;            /* frame height - in pixels */
    size_t wh;                /* width x height */
    size_t frame_size;        /* size of 1 frame - in bytes */
    Sint32 zoom;              /* zoom-factor */
    Uint32 zoom_width;
    Uint32 zoom_height;
    Uint32 grid;              /* grid-mode - on or off */
    Uint32 y_start_pos;       /* start pos for first Y pel */
    Uint32 cb_start_pos;      /* start pos for first Cb pel */
    Uint32 cr_start_pos;      /* start pos for first Cr pel */
    Uint32 y_only;            /* Grayscale, i.e Luma only */
    Uint32 cb_only;           /* Only Cb plane */
    Uint32 cr_only;           /* Only Cr plane */
    Uint32 y_size;            /* sizeof luma-data for 1 frame - in bytes */
    Uint32 cb_size;           /* sizeof croma-data for 1 frame - in bytes */
    Uint32 cr_size;           /* sizeof croma-data for 1 frame - in bytes */
    Uint8* raw;               /* pointer towards complete frame - frame_size bytes */
    char* fname;              /* obvious */
    FILE* fd;                 /* obvious */

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;
};

void usage(char* name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s filename width height format\n", name);
    fprintf(stderr, "./%s foreman.yuv 352 288 IYUV\n", name);
}

Uint32 parse_input(int argc, char **argv, struct config* cfg)
{
    if (argc != 5) {
        usage(argv[0]);
        return 0;
    }

    cfg->fname = argv[1];
    cfg->width = atoi(argv[2]);
    cfg->height = atoi(argv[3]);

    if (!strncmp(argv[4], "YV12", 4)) {
        cfg->pixel_format = SDL_PIXELFORMAT_YV12;
        cfg->format = YV12;
    } else if (!strncmp(argv[4], "IYUV", 4)) {
        cfg->pixel_format = SDL_PIXELFORMAT_IYUV;
        cfg->format = IYUV;
    } else if (!strncmp(argv[4], "YUY2", 4)) {
        cfg->pixel_format = SDL_PIXELFORMAT_YUY2;
        cfg->format = YUY2;
    } else if (!strncmp(argv[4], "UYVY", 4)) {
        cfg->pixel_format = SDL_PIXELFORMAT_UYVY;
        cfg->format = UYVY;
    } else if (!strncmp(argv[4], "YVYU", 4)) {
        cfg->pixel_format = SDL_PIXELFORMAT_YVYU;
        cfg->format = YVYU;
    } else {
        fprintf(stderr, "The format option '%s' is not recognized\n", argv[4]);
        return 0;
    }

    return 1;
}

void setup_param(struct config* cfg)
{
    cfg->wh = cfg->width * cfg->height;

    switch (cfg->format) {
        case YV12:
        case IYUV:
            cfg->frame_size = cfg->wh * 3 / 2;
            cfg->y_size = cfg->wh;
            cfg->cb_size = cfg->wh / 4;
            cfg->cr_size = cfg->wh / 4;
            break;
        case YUY2:
        case UYVY:
        case YVYU:
            cfg->frame_size = cfg->wh * 2;
            cfg->y_size = cfg->wh;
            cfg->cb_size = cfg->wh / 2;
            cfg->cr_size = cfg->wh / 2;
            break;
        default:
            fprintf(stderr, "Unknown format1!\n");
    }
}

Uint32 sdl_init(struct config* cfg)
{
    cfg->window = NULL;
    cfg->renderer = NULL;
    cfg->texture = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr,
                "\nUnable to initialize SDL:  %s\n",
                SDL_GetError()
               );
        return 0;
    }
    cfg->window = SDL_CreateWindow(
            "YCbCr",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            cfg->width,
            cfg->height,
            SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    cfg->renderer = SDL_CreateRenderer(
            cfg->window, -1,
            SDL_RENDERER_ACCELERATED);
    cfg->texture = SDL_CreateTexture(
            cfg->renderer,
            cfg->pixel_format,
            SDL_TEXTUREACCESS_STREAMING,
            cfg->width,
            cfg->height);
    return 1;
}

Uint32 open_input(struct config* cfg)
{
    cfg->fd = fopen(cfg->fname, "rb");
    if (cfg->fd == NULL) {
        fprintf(stderr, "Error opening %s\n", cfg->fname);
        return 0;
    }
    return 1;
}

Uint32 allocate_memory(struct config* cfg)
{
    cfg->raw = malloc(sizeof(Uint8) * cfg->frame_size);
    if (!cfg->raw) {
        fprintf(stderr, "Error allocating memory...\n");
        return 0;
    }
    return 1;
}

Uint32 free_memory(struct config* cfg)
{
    free(cfg->raw);
    return 1;
}

Uint32 sdl_free(struct config* cfg)
{
    SDL_DestroyWindow(cfg->window);
    SDL_DestroyRenderer(cfg->renderer);
    SDL_DestroyTexture(cfg->texture);
    return 1;
}

Uint32 display_frame(struct config* cfg)
{
    SDL_UpdateTexture(cfg->texture,
            NULL,
            cfg->raw,
            cfg->width * SDL_BYTESPERPIXEL(cfg->pixel_format));
    SDL_RenderClear(cfg->renderer);
    SDL_RenderCopy(cfg->renderer,
            cfg->texture, NULL, NULL);
    SDL_RenderPresent(cfg->renderer);
    return 1;
}

Uint32 read_frame(struct config* cfg)
{
    size_t ret = fread(cfg->raw, sizeof(Uint8), cfg->frame_size, cfg->fd);
    if (ret != cfg->frame_size) {
        return 0;
    }
    return 1;
}

void set_caption(struct config* cfg, char *array, Uint32 frame, Uint32 bytes)
{
    snprintf(array, bytes, "%s - frame %d, size %zux%zu",
            cfg->fname,
            frame,
            cfg->width,
            cfg->height);
}

Uint32 event_loop(struct config* cfg)
{
    char caption[256];
    int quit = 0;
    Uint32 frame = 0;

    while (!quit) {

        set_caption(cfg, caption, frame, 256);
        SDL_SetWindowTitle(cfg->window, caption);

        /* wait for SDL event */
        SDL_WaitEvent(&cfg->event);

        switch (cfg->event.type)
        {
            case SDL_KEYDOWN:
                switch (cfg->event.key.keysym.sym)
                {
                    case SDLK_RIGHT: /* next frame */
                        /* check for next frame existing */
                        if (read_frame(cfg)) {
                            display_frame(cfg);
                            frame++;
                        }
                        break;
                    case SDLK_LEFT: /* previous frame */
                        if (frame > 1) {
                            frame--;
                            fseek(cfg->fd, ((frame-1) * cfg->frame_size), SEEK_SET);
                            if (read_frame(cfg)) {
                                display_frame(cfg);
                            }
                        }
                        break;
                    case SDLK_r: /* rewind */
                        if (frame > 1) {
                            frame = 1;
                            fseek(cfg->fd, 0, SEEK_SET);
                            if (read_frame(cfg)) {
                                display_frame(cfg);
                            }
                        }
                        break;
                    case SDLK_q: /* quit */
                        quit = 1;
                        break;
                    default:
                        break;
                }
            default:
               break;
        }
    }

    return 1;
}

int main(int argc, char** argv) {

    int ret = EXIT_SUCCESS;
    struct config cfg = {0};

    if (!parse_input(argc, argv, &cfg)) {
        return EXIT_FAILURE;
    }

    /* Initialize parameters corresponding to YUV-format */
    setup_param(&cfg);

    if (!sdl_init(&cfg)) {
        return EXIT_FAILURE;
    }

    if (!open_input(&cfg)) {
        return EXIT_FAILURE;
    }

    if (!allocate_memory(&cfg)) {
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* send event to display first frame */
    cfg.event.type = SDL_KEYDOWN;
    cfg.event.key.keysym.sym = SDLK_RIGHT;
    SDL_PushEvent(&cfg.event);
    /* while true */
    event_loop(&cfg);

cleanup:
    free_memory(&cfg);
    SDL_DestroyWindow(cfg.window);
    SDL_DestroyRenderer(cfg.renderer);
    SDL_DestroyTexture(cfg.texture);

    SDL_Quit();

    return ret;
}
