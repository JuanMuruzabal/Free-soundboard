
#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <iostream>

class Config {
public:
    std::map<char, int> keySoundMap; // Mapeo de teclas a sonidos

    Config() {}

    // Asignar una tecla a un sonido
    void setKeyForSound(char key, int soundIndex) {
        keySoundMap[key] = soundIndex;
        std::cout << "Tecla '" << key << "' asignada al sonido " << soundIndex + 1 << std::endl;
    }

    // Obtener el sonido asignado a una tecla
    int getSoundForKey(char key) {
        if (keySoundMap.find(key) != keySoundMap.end()) {
            return keySoundMap[key];
        }
        return -1; // -1 indica que no está asignado
    }

    // Mostrar configuraciones actuales
    void showConfig() {
        std::cout << "Configuración de teclas:\n";
        for (const auto& pair : keySoundMap) {
            std::cout << "Tecla '" << pair.first << "' -> Sonido " << pair.second + 1 << std::endl;
        }
    }
};

#endif
