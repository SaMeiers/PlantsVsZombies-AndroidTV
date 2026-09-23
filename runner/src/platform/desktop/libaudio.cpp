#include <pvz_tv/dependencies/dependency.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

namespace pvz_tv {

static SDL_AudioDeviceID g_audio_device = 0;
static std::atomic<bool> g_audio_inited{false};
static std::mutex g_audio_lock;
static uint32_t g_bytes_per_sec = 176400;

static void ag_audio_init(GuestCall &c) {
    uint32_t sample_rate = c.arg(0);
    uint32_t channels = c.arg(1);
    uint32_t bits = c.arg(2);
    printf("[*] AGAudioInit(rate=%u, channels=%u, bits=%u)\n", sample_rate, channels, bits);

    std::lock_guard<std::mutex> lk(g_audio_lock);
    if (!g_audio_inited.load()) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "[-] SDL_InitSubSystem(AUDIO) failed: %s\n", SDL_GetError());
            c.set_result((uint32_t)-1);
            return;
        }

        uint32_t freq = sample_rate ? sample_rate : 44100;
        uint32_t ch = channels ? channels : 2;
        uint32_t bit_depth = (bits == 16) ? 16 : 8;
        g_bytes_per_sec = freq * ch * (bit_depth / 8);

        SDL_AudioSpec wanted, have;
        SDL_zero(wanted);
        wanted.freq = freq;
        wanted.format = (bit_depth == 16) ? AUDIO_S16SYS : AUDIO_U8;
        wanted.channels = (uint8_t)ch;
        wanted.samples = 1024;
        wanted.callback = nullptr;

        g_audio_device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &have, 0);
        if (g_audio_device == 0) {
            fprintf(stderr, "[-] SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
            c.set_result((uint32_t)-1);
            return;
        }

        SDL_PauseAudioDevice(g_audio_device, 0);
        g_audio_inited.store(true);
        printf("[+] SDL Audio initialized successfully! Device ID: %u (%d Hz, %d channels, bytes/sec=%u)\n",
               g_audio_device, have.freq, have.channels, g_bytes_per_sec);
    }

    c.set_result(0);
}

static void ag_audio_is_paused(GuestCall &c) {
    c.set_result(0);
}

static void ag_audio_write(GuestCall &c) {
    uint32_t buf_ptr = c.arg(0);
    uint32_t size_bytes = c.arg(1);

    if (!buf_ptr || !size_bytes || !c.in_bounds(buf_ptr, size_bytes)) {
        c.set_result(0);
        return;
    }

    // Flow control / backpressure: keep audio buffer around ~80-100ms
    uint32_t target_queued = g_bytes_per_sec / 10;
    if (target_queued == 0) target_queued = 17640;

    while (g_audio_inited.load() && g_audio_device != 0) {
        uint32_t queued = SDL_GetQueuedAudioSize(g_audio_device);
        if (queued <= target_queued) {
            break;
        }
        uint32_t excess = queued - target_queued;
        uint32_t sleep_ms = (excess * 1000) / g_bytes_per_sec;
        if (sleep_ms < 2) sleep_ms = 2;
        if (sleep_ms > 20) sleep_ms = 20;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    std::lock_guard<std::mutex> lk(g_audio_lock);
    if (g_audio_inited.load() && g_audio_device != 0) {
        SDL_QueueAudio(g_audio_device, c.img->mem + buf_ptr, size_bytes);
    }

    c.set_result(size_bytes);
}

static void ag_audio_uninit(GuestCall &c) {
    printf("[*] AGAudioUninit()\n");
    std::lock_guard<std::mutex> lk(g_audio_lock);
    g_audio_inited.store(false);
    if (g_audio_device != 0) {
        SDL_ClearQueuedAudio(g_audio_device);
        SDL_CloseAudioDevice(g_audio_device);
        g_audio_device = 0;
    }
    c.set_result(0);
}

static void ag_audio_set_read_cb(GuestCall &c) {
    c.set_result(0);
}

void register_libaudio(ImportTable &t) {
    t.add("AGAudioInit", ag_audio_init);
    t.add("AGAudioIsPaused", ag_audio_is_paused);
    t.add("AGAudioWrite", ag_audio_write);
    t.add("AGAudioUninit", ag_audio_uninit);
    t.add("AGAudioSetReadCallback", ag_audio_set_read_cb);
}

} // namespace pvz_tv
