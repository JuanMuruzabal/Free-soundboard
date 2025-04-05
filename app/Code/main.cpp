#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "config.h"

#include <conio.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstring>

#define NUM_SOUNDS 5

const char *soundFiles[NUM_SOUNDS] = {
    "audio/sound1.wav",
    "audio/sound2.wav",
    "audio/sound3.wav",
    "audio/sound4.wav",
    "audio/sound5.wav"};

ma_context context;
ma_device playbackDevice;
ma_device captureDevice;
ma_decoder decoders[NUM_SOUNDS];
Config config;

std::string configFilePath = "audio_config.txt";
int selectedPlaybackDeviceIndex = -1;
int selectedCaptureDeviceIndex = -1;

std::vector<std::vector<float>> soundBuffers(NUM_SOUNDS);
std::vector<size_t> bufferPositions(NUM_SOUNDS, 0);
std::mutex bufferMutex;
std::atomic<bool> isPlaying[NUM_SOUNDS];
std::atomic<bool> gRunning = true;

void saveDeviceConfig()
{
    std::ofstream file(configFilePath);
    if (file.is_open())
    {
        file << selectedPlaybackDeviceIndex << std::endl;
        file << selectedCaptureDeviceIndex << std::endl;
        file.close();
    }
}

void loadDeviceConfig()
{
    std::ifstream file(configFilePath);
    if (file.is_open())
    {
        file >> selectedPlaybackDeviceIndex;
        file >> selectedCaptureDeviceIndex;
        file.close();
        std::cout << "Dispositivo de salida seleccionado: " << selectedPlaybackDeviceIndex << std::endl;
        std::cout << "Dispositivo de entrada seleccionado: " << selectedCaptureDeviceIndex << std::endl;
    }
}

void listDevices(ma_context *ctx, std::vector<ma_device_info> &playbacks, std::vector<ma_device_info> &captures)
{
    ma_device_info *pPlaybackInfos;
    ma_device_info *pCaptureInfos;
    ma_uint32 playbackCount, captureCount;

    if (ma_context_get_devices(ctx, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        std::cerr << "No se pudieron obtener los dispositivos de audio." << std::endl;
        return;
    }

    std::cout << "\nDispositivos de salida disponibles:\n";
    for (ma_uint32 i = 0; i < playbackCount; ++i)
    {
        std::cout << i << ": " << pPlaybackInfos[i].name << std::endl;
        playbacks.push_back(pPlaybackInfos[i]);
    }

    std::cout << "\nDispositivos de entrada disponibles:\n";
    for (ma_uint32 i = 0; i < captureCount; ++i)
    {
        std::cout << i << ": " << pCaptureInfos[i].name << std::endl;
        captures.push_back(pCaptureInfos[i]);
    }
}

void selectDevices(ma_context *ctx)
{
    std::vector<ma_device_info> playbacks, captures;
    listDevices(ctx, playbacks, captures);

    std::string input;

    std::cout << "Selecciona el dispositivo de salida (enter para mantener actual): ";
    std::getline(std::cin, input);
    if (!input.empty())
    {
        int idx = std::stoi(input);
        if (idx >= 0 && idx < (int)playbacks.size())
            selectedPlaybackDeviceIndex = idx;
        else
            std::cerr << "Índice inválido de salida.\n";
    }

    std::cout << "Selecciona el dispositivo de entrada (enter para mantener actual): ";
    std::getline(std::cin, input);
    if (!input.empty())
    {
        int idx = std::stoi(input);
        if (idx >= 0 && idx < (int)captures.size())
            selectedCaptureDeviceIndex = idx;
        else
            std::cerr << "Índice inválido de entrada.\n";
    }

    saveDeviceConfig();
}

void playbackCallback(ma_device *device, void *output, const void *input, ma_uint32 frameCount)
{
    float *out = (float *)output;
    std::memset(out, 0, sizeof(float) * frameCount * 2);

    std::lock_guard<std::mutex> lock(bufferMutex);
    for (int i = 0; i < NUM_SOUNDS; ++i)
    {
        if (isPlaying[i])
        {
            auto &buffer = soundBuffers[i];
            size_t &pos = bufferPositions[i];
            for (ma_uint32 f = 0; f < frameCount && pos + 1 < buffer.size(); ++f)
            {
                out[f * 2] += buffer[pos++];
                out[f * 2 + 1] += buffer[pos++];
            }
            if (pos >= buffer.size())
            {
                isPlaying[i] = false;
                pos = 0;
            }
        }
    }

    (void)input;
}

void captureCallback(ma_device *device, void *output, const void *input, ma_uint32 frameCount)
{
    const float *in = (const float *)input;
    float *out = (float *)output;

    if (!in || !out)
        return;

    for (ma_uint32 i = 0; i < frameCount * 2; i++)
    {
        out[i] += in[i];
    }
}

void initAudio()
{
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS)
    {
        std::cerr << "Error inicializando el contexto de audio.\n";
        exit(1);
    }

    if (!std::filesystem::exists(configFilePath))
    {
        std::cout << "Configuración de audio no encontrada. Seleccionando dispositivos.\n";
        selectDevices(&context);
    }
    else
    {
        loadDeviceConfig();
    }

    // Cargar sonidos
    for (int i = 0; i < NUM_SOUNDS; ++i)
    {
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);
        if (ma_decoder_init_file(soundFiles[i], &config, &decoders[i]) == MA_SUCCESS)
        {
            ma_uint64 frames;
            ma_decoder_get_length_in_pcm_frames(&decoders[i], &frames);
            soundBuffers[i].resize(frames * 2);
            ma_decoder_read_pcm_frames(&decoders[i], soundBuffers[i].data(), frames, NULL);
        }
        else
        {
            std::cerr << "Error cargando: " << soundFiles[i] << std::endl;
        }
    }

    // Configuración del dispositivo de salida
    ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
    playbackConfig.playback.format = ma_format_f32;
    playbackConfig.playback.channels = 2;
    playbackConfig.sampleRate = 48000;
    playbackConfig.dataCallback = playbackCallback;

    if (selectedPlaybackDeviceIndex >= 0)
    {
        ma_device_info *pPlaybackInfos;
        ma_uint32 count;
        if (ma_context_get_devices(&context, &pPlaybackInfos, &count, NULL, NULL) == MA_SUCCESS)
        {
            if (selectedPlaybackDeviceIndex < (int)count)
            {
                playbackConfig.playback.pDeviceID = &pPlaybackInfos[selectedPlaybackDeviceIndex].id;
            }
        }
    }

    if (ma_device_init(&context, &playbackConfig, &playbackDevice) != MA_SUCCESS)
    {
        std::cerr << "Error iniciando dispositivo de salida.\n";
        exit(1);
    }

    // Configuración del micrófono
    ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
    captureConfig.capture.format = ma_format_f32;
    captureConfig.capture.channels = 2;
    captureConfig.sampleRate = 48000;
    captureConfig.dataCallback = captureCallback;

    if (selectedCaptureDeviceIndex >= 0)
    {
        ma_device_info *pCaptureInfos;
        ma_uint32 count;
        if (ma_context_get_devices(&context, NULL, NULL, &pCaptureInfos, &count) == MA_SUCCESS)
        {
            if (selectedCaptureDeviceIndex < (int)count)
            {
                captureConfig.capture.pDeviceID = &pCaptureInfos[selectedCaptureDeviceIndex].id;
            }
        }
    }

    if (ma_device_init(&context, &captureConfig, &captureDevice) != MA_SUCCESS)
    {
        std::cerr << "Error iniciando dispositivo de entrada.\n";
        exit(1);
    }

    ma_device_start(&playbackDevice);
    ma_device_start(&captureDevice);
}

void cleanupAudio()
{
    for (int i = 0; i < NUM_SOUNDS; ++i)
    {
        ma_decoder_uninit(&decoders[i]);
    }

    ma_device_uninit(&playbackDevice);
    ma_device_uninit(&captureDevice);
    ma_context_uninit(&context);
}

void toggleSound(int index)
{
    if (index < 0 || index >= NUM_SOUNDS)
        return;

    std::lock_guard<std::mutex> lock(bufferMutex);
    bufferPositions[index] = 0;
    isPlaying[index] = true;
}

void editDeviceSelection()
{
    ma_context temp;
    if (ma_context_init(NULL, 0, NULL, &temp) == MA_SUCCESS)
    {
        selectDevices(&temp);
        ma_context_uninit(&temp);
        std::cout << "Configuración actualizada. Reinicia el programa para aplicar los cambios.\n";
    }
}

int main()
{
    try
    {
        initAudio();

        std::cout << "Presiona una tecla para asignarla a un sonido.\n";
        std::cout << "Presiona D para editar dispositivos de audio.\n";
        std::cout << "Presiona ESC para salir.\n";

        int nextSoundIndex = 0;

        while (gRunning)
        {
            if (_kbhit())
            {
                char key = _getch();
                if (key == 27)
                    break; // ESC
                if (key == 'D' || key == 'd')
                {
                    editDeviceSelection();
                    break;
                }

                int assigned = config.getSoundForKey(key);
                if (assigned != -1)
                {
                    toggleSound(assigned);
                }
                else if (nextSoundIndex < NUM_SOUNDS)
                {
                    config.setKeyForSound(key, nextSoundIndex);
                    toggleSound(nextSoundIndex);
                    ++nextSoundIndex;
                }
                else
                {
                    std::cout << "No quedan sonidos para asignar.\n";
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        cleanupAudio();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Excepción atrapada: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Error desconocido atrapado.\n";
    }

    return 0;
}
