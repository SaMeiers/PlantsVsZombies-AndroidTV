#include <pvz_tv/config.h>
#include <cstring>

static pvz2_config_t s_default_config = []() {
    pvz2_config_t cfg;
    std::memset(&cfg, 0, sizeof(cfg));
    cfg.fps_limit = 60;
    cfg.persist_saves = 1;
    cfg.video_width = 1280;
    cfg.video_height = 720;
    std::strncpy(cfg.package_name, "com.trans.pvztv", sizeof(cfg.package_name) - 1);
    std::strncpy(cfg.user_locale, "en_US", sizeof(cfg.user_locale) - 1);
    std::strncpy(cfg.device_name, "AndroidTV", sizeof(cfg.device_name) - 1);
    std::strncpy(cfg.hardware_model, "arm64", sizeof(cfg.hardware_model) - 1);
    std::strncpy(cfg.os_version, "14", sizeof(cfg.os_version) - 1);
    return cfg;
}();

extern "C" void pvz2_config_load(const char *ini_path, const char *base_dir) {
    (void)ini_path;
    (void)base_dir;
}

extern "C" const pvz2_config_t *pvz2_config(void) {
    return &s_default_config;
}
