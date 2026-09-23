#include <pvz_tv/dependencies/dependency.h>
#include <pvz_tv/surface.h>

#include <cstring>

namespace pvz_tv {

namespace {

constexpr uint32_t kFakeLooper = 0x0000B100;
constexpr uint32_t kFakeWindow = 0x0000B200;
constexpr uint32_t kFakeConfig = 0x0000B300;

void a_config_new(GuestCall &c) {
    c.set_result(kFakeConfig);
}

void a_config_delete(GuestCall &c) {
    c.set_result(0);
}

void a_config_from_asset(GuestCall &c) {
    c.set_result(0);
}

void a_config_get_language(GuestCall &c) {
    uint32_t out = c.arg(1);
    if (out && c.in_bounds(out, 2)) {
        c.write8(out, 'e');
        c.write8(out + 1, 'n');
    }
}

void a_config_get_country(GuestCall &c) {
    uint32_t out = c.arg(1);
    if (out && c.in_bounds(out, 2)) {
        c.write8(out, 'U');
        c.write8(out + 1, 'S');
    }
}

void a_config_get_density(GuestCall &c) {
    // ACONFIGURATION_DENSITY_HIGH = 240
    c.set_result(240);
}

void a_config_get_keyboard(GuestCall &c) {
    c.set_result(1); // ACONFIGURATION_KEYBOARD_NOKEYS
}

void a_config_get_keys_hidden(GuestCall &c) {
    c.set_result(1); // ACONFIGURATION_KEYSHIDDEN_ANY
}

void a_config_get_mcc(GuestCall &c) {
    c.set_result(0);
}

void a_config_get_mnc(GuestCall &c) {
    c.set_result(0);
}

void a_config_get_nav_hidden(GuestCall &c) {
    c.set_result(1);
}

void a_config_get_navigation(GuestCall &c) {
    c.set_result(1); // ACONFIGURATION_NAVIGATION_NONAV
}

void a_config_get_orientation(GuestCall &c) {
    c.set_result(2); // ACONFIGURATION_ORIENTATION_LAND
}

void a_config_get_screen_long(GuestCall &c) {
    c.set_result(1); // ACONFIGURATION_SCREENLONG_NO
}

void a_config_get_screen_size(GuestCall &c) {
    c.set_result(3); // ACONFIGURATION_SCREENSIZE_LARGE
}

void a_config_get_sdk_version(GuestCall &c) {
    c.set_result(19); // Android 4.4 KitKat
}

void a_config_get_touchscreen(GuestCall &c) {
    c.set_result(3); // ACONFIGURATION_TOUCHSCREEN_FINGER
}

void a_config_get_ui_mode_night(GuestCall &c) {
    c.set_result(1); // ACONFIGURATION_UI_MODE_NIGHT_NO
}

void a_config_get_ui_mode_type(GuestCall &c) {
    c.set_result(4); // ACONFIGURATION_UI_MODE_TYPE_TELEVISION
}

void a_looper_prepare(GuestCall &c) {
    c.set_result(kFakeLooper);
}

void a_looper_for_thread(GuestCall &c) {
    c.set_result(kFakeLooper);
}

void a_looper_add_fd(GuestCall &c) {
    c.set_result(1);
}

void a_looper_remove_fd(GuestCall &c) {
    c.set_result(1);
}

void a_looper_poll_once(GuestCall &c) {
    c.set_result((uint32_t)-3); // ALOOPER_POLL_TIMEOUT
}

void a_window_acquire(GuestCall &c) {}
void a_window_release(GuestCall &c) {}

void a_window_from_surface(GuestCall &c) {
    c.set_result(kFakeWindow);
}

void a_window_get_width(GuestCall &c) {
    c.set_result(pvz2_surface_width());
}

void a_window_get_height(GuestCall &c) {
    c.set_result(pvz2_surface_height());
}

void a_window_set_buffers_geom(GuestCall &c) {
    c.set_result(0);
}

void a_bitmap_get_info(GuestCall &c) {
    uint32_t info_ptr = c.arg(2);
    if (info_ptr && c.in_bounds(info_ptr, 16)) {
        // AndroidBitmapInfo: width(4), height(4), stride(4), format(4), flags(4)
        c.write32(info_ptr, pvz2_surface_width());
        c.write32(info_ptr + 4, pvz2_surface_height());
        c.write32(info_ptr + 8, pvz2_surface_width() * 4);
        c.write32(info_ptr + 12, 1); // ANDROID_BITMAP_FORMAT_RGBA_8888
    }
    c.set_result(0);
}

void a_bitmap_lock_pixels(GuestCall &c) {
    uint32_t addr_ptr = c.arg(2);
    if (addr_ptr && c.in_bounds(addr_ptr, 4)) {
        c.write32(addr_ptr, 0);
    }
    c.set_result(0);
}

void a_bitmap_unlock_pixels(GuestCall &c) {
    c.set_result(0);
}

void sys_prop_get(GuestCall &c) {
    std::string key = c.cstr(c.arg(0));
    uint32_t val_ptr = c.arg(1);
    std::string val;
    if (key == "ro.build.version.sdk") {
        val = "19";
    } else if (key == "ro.product.model") {
        val = "Android TV";
    } else if (key == "ro.product.manufacturer") {
        val = "PopCap";
    } else {
        val = "";
    }
    if (val_ptr && c.in_bounds(val_ptr, (uint32_t)val.size() + 1)) {
        c.put_cstr(val_ptr, val);
    }
    c.set_result((uint32_t)val.size());
}

} // namespace

void register_libandroid(ImportTable &t) {
    t.add("AConfiguration_new", a_config_new);
    t.add("AConfiguration_delete", a_config_delete);
    t.add("AConfiguration_fromAssetManager", a_config_from_asset);
    t.add("AConfiguration_getCountry", a_config_get_country);
    t.add("AConfiguration_getLanguage", a_config_get_language);
    t.add("AConfiguration_getDensity", a_config_get_density);
    t.add("AConfiguration_getKeyboard", a_config_get_keyboard);
    t.add("AConfiguration_getKeysHidden", a_config_get_keys_hidden);
    t.add("AConfiguration_getMcc", a_config_get_mcc);
    t.add("AConfiguration_getMnc", a_config_get_mnc);
    t.add("AConfiguration_getNavHidden", a_config_get_nav_hidden);
    t.add("AConfiguration_getNavigation", a_config_get_navigation);
    t.add("AConfiguration_getOrientation", a_config_get_orientation);
    t.add("AConfiguration_getScreenLong", a_config_get_screen_long);
    t.add("AConfiguration_getScreenSize", a_config_get_screen_size);
    t.add("AConfiguration_getSdkVersion", a_config_get_sdk_version);
    t.add("AConfiguration_getTouchscreen", a_config_get_touchscreen);
    t.add("AConfiguration_getUiModeNight", a_config_get_ui_mode_night);
    t.add("AConfiguration_getUiModeType", a_config_get_ui_mode_type);

    t.add("ALooper_prepare", a_looper_prepare);
    t.add("ALooper_forThread", a_looper_for_thread);
    t.add("ALooper_addFd", a_looper_add_fd);
    t.add("ALooper_removeFd", a_looper_remove_fd);
    t.add("ALooper_pollOnce", a_looper_poll_once);

    t.add("ANativeWindow_acquire", a_window_acquire);
    t.add("ANativeWindow_release", a_window_release);
    t.add("ANativeWindow_fromSurface", a_window_from_surface);
    t.add("ANativeWindow_getWidth", a_window_get_width);
    t.add("ANativeWindow_getHeight", a_window_get_height);
    t.add("ANativeWindow_setBuffersGeometry", a_window_set_buffers_geom);

    t.add("AndroidBitmap_getInfo", a_bitmap_get_info);
    t.add("AndroidBitmap_lockPixels", a_bitmap_lock_pixels);
    t.add("AndroidBitmap_unlockPixels", a_bitmap_unlock_pixels);

    t.add("__system_property_get", sys_prop_get);
}

} // namespace pvz_tv
