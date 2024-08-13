#include <time.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <switch.h>
#include "sdl_starter.h"
#include "sdl_assets_loader.h"

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_GameController *controller = nullptr;

bool isGamePaused = false;
int shouldCloseTheGame = 0;
int trail = 0;
int wait = 15;

Sprite playerSprite;
Sprite switchlogoSprite;

const int PLAYER_SPEED = 600;

int logoVelocityX = 250;
int logoVelocityY = 250;

int colorIndex = 0;
int soundIndex = 0;

Mix_Music *music = nullptr;
Mix_Chunk *sounds[4] = {nullptr};

SDL_Texture *pauseGameTexture = nullptr;
SDL_Rect pauseGameBounds;

SDL_Color colors[] = {
    {128, 128, 128, 0}, // gray
    {255, 255, 255, 0}, // white
    {255, 0, 0, 0},     // red
    {0, 255, 0, 0},     // green
    {0, 0, 255, 0},     // blue
    {255, 255, 0, 0},   // brown
    {0, 255, 255, 0},   // cyan
    {255, 0, 255, 0},   // purple
};

void quitGame()
{
    // clean up your textures when you are done with them
    SDL_DestroyTexture(switchlogoSprite.texture);
    SDL_DestroyTexture(pauseGameTexture);

    // stop sounds and free loaded data
    Mix_HaltChannel(-1);
    Mix_FreeMusic(music);

    for (soundIndex = 0; soundIndex < 4; soundIndex++)
    {
        if (sounds[soundIndex])
        {
            Mix_FreeChunk(sounds[soundIndex]);
        }
    }

    IMG_Quit();
    Mix_CloseAudio();
    TTF_Quit();
    Mix_Quit();
    SDL_Quit();
    romfsExit();
}

void handleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            shouldCloseTheGame = 1;
        }

        if (event.type == SDL_JOYBUTTONDOWN)
        {
            if (event.jbutton.button == JOY_MINUS)
            {
                shouldCloseTheGame = 1;
            }

            if (event.jbutton.button == JOY_PLUS)
            {
                isGamePaused = !isGamePaused;
                Mix_PlayChannel(-1, sounds[0], 0);
            }

            if (event.jbutton.button == JOY_A)
            {
                if (wait > 0)
                    wait--;
            }

            if (event.jbutton.button == JOY_X)
            {
                if (wait < 100)
                    wait++;
            }

            if (event.jbutton.button == JOY_B)
            {
                trail = !trail;
            }
        }
    }
}

int rand_range(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void update(float deltaTime)
{
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) && playerSprite.textureBounds.y > 0)
    {
        playerSprite.textureBounds.y -= PLAYER_SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) && playerSprite.textureBounds.y < SCREEN_HEIGHT - playerSprite.textureBounds.h)
    {
        playerSprite.textureBounds.y += PLAYER_SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) && playerSprite.textureBounds.x > 0)
    {
        playerSprite.textureBounds.x -= PLAYER_SPEED * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) && playerSprite.textureBounds.x < SCREEN_WIDTH - playerSprite.textureBounds.w)
    {
        playerSprite.textureBounds.x += PLAYER_SPEED * deltaTime;
    }

    if (SDL_HasIntersection(&playerSprite.textureBounds, &switchlogoSprite.textureBounds))
    {
        logoVelocityX *= -1;
        logoVelocityY *= -1;

        colorIndex = rand_range(0, 4);
        soundIndex = rand_range(0, 3);

        Mix_PlayChannel(-1, sounds[soundIndex], 0);
    }

    if (switchlogoSprite.textureBounds.x + switchlogoSprite.textureBounds.w > SCREEN_WIDTH || switchlogoSprite.textureBounds.x < 0)
    {
        logoVelocityX *= -1;
        colorIndex = rand_range(0, 4);
        soundIndex = rand_range(0, 3);

        Mix_PlayChannel(-1, sounds[soundIndex], 0);
    }

    if (switchlogoSprite.textureBounds.y + switchlogoSprite.textureBounds.h > SCREEN_HEIGHT || switchlogoSprite.textureBounds.y < 0)
    {
        logoVelocityY *= -1;
        colorIndex = rand_range(0, 4);
        soundIndex = rand_range(0, 3);

        Mix_PlayChannel(-1, sounds[soundIndex], 0);
    }

    // set position and bounce on the walls
    switchlogoSprite.textureBounds.x += logoVelocityX * deltaTime;
    switchlogoSprite.textureBounds.y += logoVelocityY * deltaTime;
}

void renderSprite(Sprite sprite)
{
    SDL_RenderCopy(renderer, sprite.texture, NULL, &sprite.textureBounds);
}

void render()
{
    if (!trail)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
        SDL_RenderClear(renderer);
    }

    SDL_SetTextureColorMod(switchlogoSprite.texture, colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b);
    renderSprite(switchlogoSprite);

    renderSprite(playerSprite);

    if (isGamePaused)
    {
        // put text on screen
        SDL_RenderCopy(renderer, pauseGameTexture, NULL, &pauseGameBounds);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    window = SDL_CreateWindow("sdl2 switch starter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (startSDL(window, renderer) > 0)
    {
        return 1;
    }

    if (SDL_NumJoysticks() < 1)
    {
        printf("No game controllers connected!\n");
        return -1;
    }
    else
    {
        controller = SDL_GameControllerOpen(0);
        if (controller == NULL)
        {
            printf("Unable to open game controller! SDL Error: %s\n", SDL_GetError());
            return -1;
        }
    }

    playerSprite = loadSprite(renderer, "data/alien_1.png", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2);
    switchlogoSprite = loadSprite(renderer, "data/switch.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    // load font from romfs
    TTF_Font *font = TTF_OpenFont("data/LeroyLetteringLightBeta01.ttf", 36);

    // render text as texture
    updateTextureText(pauseGameTexture, "Game Paused", font, renderer);

    SDL_QueryTexture(pauseGameTexture, NULL, NULL, &pauseGameBounds.w, &pauseGameBounds.h);
    pauseGameBounds.x = SCREEN_WIDTH / 2 - pauseGameBounds.w / 2;
    pauseGameBounds.y = 200;

    // no need to keep the font loaded
    TTF_CloseFont(font);

    // load music and sounds from files
    sounds[0] = loadSound("data/pop1.wav");
    sounds[1] = loadSound("data/pop2.wav");
    sounds[2] = loadSound("data/pop3.wav");
    sounds[3] = loadSound("data/pop4.wav");

    music = loadMusic("data/background.ogg");

    Mix_PlayMusic(music, -1);

    srand(time(NULL));

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime = previousFrameTime;
    float deltaTime = 0.0f;

    while (!shouldCloseTheGame && appletMainLoop())
    {
        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        SDL_GameControllerUpdate();

        handleEvents();

        if (!isGamePaused)
        {
            update(deltaTime);
        }

        render();

        SDL_Delay(wait);
    }

    quitGame();
    return 0;
}