#include "config.h"
#include <SDL2/SDL_mixer.h>


std::vector<Mix_Chunk *> sounds;
Config config; // Instancia de Config

int loadSound(const char *filename) {
    Mix_Chunk *s = Mix_LoadWAV(filename);
    if (s == NULL) {
        printf("Failed to load wav! SDL_mixer Error: %s\n", Mix_GetError());
        return -1;
    }
    sounds.push_back(s);
    return sounds.size() - 1;
}

int volume;
void setVolume(int v) {
    volume = (MIX_MAX_VOLUME * v) / 100;
}

int playSound(int s) {
    Mix_Volume(-1, volume);
    Mix_PlayChannel(-1, sounds[s], 0);
    return 0;
}

int initMixer() {
    Mix_Init(MIX_INIT_MP3);
    SDL_Init(SDL_INIT_AUDIO);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return -1;
    }
    setVolume(80);
    return 0;
}

void quitMixer() {
    for (int s = 0; s < sounds.size(); s++) {
        Mix_FreeChunk(sounds[s]);
        sounds[s] = NULL;
    }
    Mix_Quit();
}

int main() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        std::cout << "Failed at SDL_Init()" << std::endl;

    initMixer();

    // Cargar 5 sonidos
    int sound1 = loadSound("sound1.wav");
    int sound2 = loadSound("sound2.wav");
    int sound3 = loadSound("sound3.wav");
    int sound4 = loadSound("sound4.wav");
    int sound5 = loadSound("sound5.wav");

    // Configurar teclas para los sonidos
    for (int i = 0; i < 5; i++) {
        char key = config.getKeyForSound(i);
        if (key == '\0') { // Si no hay tecla asignada
            std::cout << "Presiona una tecla para configurar el sonido " << (i + 1) << ": ";
            std::cin >> key;
            config.setKeyForSound(i, key);
        }
    }

    sdltemplate::init();
    while (sdltemplate::running) {
        sdltemplate::loop();
        sdltemplate::begin_render();
        sdltemplate::end_render();

        // Verificar si la tecla correspondiente a un sonido ha sido presionada
        if (sdltemplate::keystates[SDL_GetScancodeFromKey(config.getKeyForSound(0))]) {
            playSound(sound1);
        }
        if (sdltemplate::keystates[SDL_GetScancodeFromKey(config.getKeyForSound(1))]) {
            playSound(sound2);
        }
        if (sdltemplate::keystates[SDL_GetScancodeFromKey(config.getKeyForSound(2))]) {
            playSound(sound3);
        }
        if (sdltemplate::keystates[SDL_GetScancodeFromKey(config.getKeyForSound(3))]) {
            playSound(sound4);
        }
        if (sdltemplate::keystates[SDL_GetScancodeFromKey(config.getKeyForSound(4))]) {
            playSound(sound5);
        }
    }

    quitMixer();
    sdltemplate::quit();
    return 0;
}

