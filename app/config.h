#include "template.h"
#include <map>


class Config {
public:
    std::map<int, char> soundKeyMap; // Mapeo de sonidos a teclas

    Config() {}
    
    // Configurar una tecla para un sonido específico
    void setKeyForSound(int soundIndex, char key) {
        soundKeyMap[soundIndex] = key;
        std::cout << "Sonido " << soundIndex << " asignado a la tecla " << key << std::endl;
    }
    
    // Obtener la tecla asignada a un sonido
    char getKeyForSound(int soundIndex) {
        if (soundKeyMap.find(soundIndex) != soundKeyMap.end()) {
            return soundKeyMap[soundIndex];
        }
        return '\0'; // Devuelve un carácter nulo si no hay configuración
    }
    
    // Eliminar configuración de una tecla
    void removeKeyForSound(int soundIndex) {
        soundKeyMap.erase(soundIndex);
    }
    
    // Limpiar toda la configuración de teclas
    void resetConfig() {
        soundKeyMap.clear();
    }
};
