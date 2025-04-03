#define MINIAUDIO_IMPLEMENTATION
#include "config.h"
#include "miniaudio.h"
#include <conio.h> // Para detección de teclas en Windows

#define NUM_SOUNDS 5

const char* soundFiles[NUM_SOUNDS] = {
    "audio/sound1.wav",
    "audio/sound2.wav",
    "audio/sound3.wav",
    "audio/sound4.wav",
    "audio/sound5.wav",
};

ma_engine engine;
ma_sound sounds[NUM_SOUNDS];
bool isPlaying[NUM_SOUNDS] = {false}; // Control de reproducción
Config config; // Configuración de teclas

void initAudio() {
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        std::cerr << "Error inicializando Miniaudio" << std::endl;
        exit(1);
    }
    for (int i = 0; i < NUM_SOUNDS; i++) {
        if (ma_sound_init_from_file(&engine, soundFiles[i], 0, NULL, NULL, &sounds[i]) != MA_SUCCESS) {
            std::cerr << "Error cargando: " << soundFiles[i] << std::endl;
        }
    }
}

void cleanupAudio() {
    for (int i = 0; i < NUM_SOUNDS; i++) {
        ma_sound_uninit(&sounds[i]);
    }
    ma_engine_uninit(&engine);
}

void toggleSound(int soundIndex) {
    if (isPlaying[soundIndex]) {
        ma_sound_stop(&sounds[soundIndex]);
    } else {
        ma_sound_start(&sounds[soundIndex]);
    }
    isPlaying[soundIndex] = !isPlaying[soundIndex];
}

int main() {
    initAudio();
    std::cout << "Presiona una tecla para asignarla a un sonido.\n";
    std::cout << "Luego usa esa tecla para reproducir/detener el sonido.\n";
    std::cout << "Presiona ESC para salir.\n";

    int nextSoundIndex = 0;

    while (true) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 27) break; // 27 = ESC

            int assignedSound = config.getSoundForKey(key);

            if (assignedSound != -1) {
                // Si la tecla ya está asignada a un sonido, alternar reproducción
                toggleSound(assignedSound);
            } else if (nextSoundIndex < NUM_SOUNDS) {
                // Si la tecla no está asignada y hay sonidos disponibles, asignar
                config.setKeyForSound(key, nextSoundIndex);
                toggleSound(nextSoundIndex);
                nextSoundIndex++;
            } else {
                std::cout << "No quedan sonidos disponibles para asignar.\n";
            }
        }
    }

    cleanupAudio();
    return 0;
}
