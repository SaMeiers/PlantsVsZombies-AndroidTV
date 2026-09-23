#include <pvz_tv/dependencies/dependency.h>
#include <pvz_tv/dependencies/libc_internal.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <ctime>
#include <zlib.h>

namespace pvz_tv {

namespace {

void f_fseeko(GuestCall &c) {
    std::FILE *f = c.file(c.arg(0));
    int32_t off = (int32_t)c.arg(1);
    int whence = (int)c.arg(2);
    c.set_result(f ? (uint32_t)std::fseek(f, off, whence) : (uint32_t)-1);
}

void f_ftello(GuestCall &c) {
    std::FILE *f = c.file(c.arg(0));
    long pos = f ? std::ftell(f) : -1L;
    c.set_result64((uint64_t)pos);
}

void f_fgetws(GuestCall &c) {
    uint32_t dst = c.arg(0);
    int n = (int)c.arg(1);
    std::FILE *f = c.file(c.arg(2));
    if (!f || n <= 0 || !c.in_bounds(dst, (uint32_t)(n * 4))) {
        c.set_result(0);
        return;
    }
    std::vector<wchar_t> buf((size_t)n);
    if (!fgetws(buf.data(), n, f)) {
        c.set_result(0);
        return;
    }
    for (int i = 0; i < n && buf[i] != 0; ++i) {
        c.write32(dst + (uint32_t)(i * 4), (uint32_t)buf[i]);
    }
    c.set_result(dst);
}

void f_perror(GuestCall &c) {
    const char *s = (const char*)c.ptr(c.arg(0));
    if (s) std::perror(s);
}

void f_ftime(GuestCall &c) {
    uint32_t tb_ptr = c.arg(0);
    if (tb_ptr && c.in_bounds(tb_ptr, 8)) {
        // struct timeb: time_t time; unsigned short millitm, timezone, dstflag.
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - secs);
        c.write32(tb_ptr, (uint32_t)secs.count());
        c.write16(tb_ptr + 4, (uint16_t)ms.count());
        c.write16(tb_ptr + 6, 0);
    }
    c.set_result(0);
}

void f_frexp(GuestCall &c) {
    double x = c.argd(0);
    uint32_t exp_ptr = c.arg(2);
    int exp = 0;
    double r = std::frexp(x, &exp);
    if (exp_ptr && c.in_bounds(exp_ptr, 4)) {
        c.write32(exp_ptr, (uint32_t)exp);
    }
    c.set_resultd(r);
}

void f_ldexp(GuestCall &c) {
    double x = c.argd(0);
    int exp = (int)c.arg(2);
    c.set_resultd(std::ldexp(x, exp));
}

void f_lrintf(GuestCall &c) {
    float x = c.argf(0);
    c.set_result((uint32_t)std::lrint(x));
}

void f_memmem(GuestCall &c) {
    uint32_t h = c.arg(0);
    size_t hl = c.arg(1);
    uint32_t n = c.arg(2);
    size_t nl = c.arg(3);
    const void *hay = c.ptr(h, (uint32_t)hl);
    const void *ndl = c.ptr(n, (uint32_t)nl);
    if (!hay || !ndl || nl > hl) {
        c.set_result(0);
        return;
    }
    const uint8_t *p = (const uint8_t*)hay;
    const uint8_t *nbytes = (const uint8_t*)ndl;
    for (size_t i = 0; i + nl <= hl; ++i) {
        if (std::memcmp(p + i, nbytes, nl) == 0) {
            c.set_result((uint32_t)(h + i));
            return;
        }
    }
    c.set_result(0);
}

void f_getrlimit(GuestCall &c) {
    uint32_t rlp = c.arg(1);
    if (rlp && c.in_bounds(rlp, 8)) {
        c.write32(rlp, 1024);     // rlim_cur
        c.write32(rlp + 4, 1024); // rlim_max
    }
    c.set_result(0);
}

void f_setrlimit(GuestCall &c) {
    c.set_result(0);
}

void f_getrusage(GuestCall &c) {
    uint32_t usage = c.arg(1);
    if (usage && c.in_bounds(usage, 72)) {
        void *p = c.ptr(usage, 72);
        if (p) std::memset(p, 0, 72);
    }
    c.set_result(0);
}

void f_chown(GuestCall &c) {
    c.set_result(0);
}

void f_pthread_attr_getschedpolicy(GuestCall &c) {
    uint32_t p = c.arg(1);
    if (p && c.in_bounds(p, 4)) c.write32(p, 0);
    c.set_result(0);
}

void f_pthread_attr_getstacksize(GuestCall &c) {
    uint32_t p = c.arg(1);
    if (p && c.in_bounds(p, 4)) c.write32(p, 1024 * 1024);
    c.set_result(0);
}

void f_wcsrtombs(GuestCall &c) {
    uint32_t dst = c.arg(0);
    uint32_t src_ptr = c.arg(1);
    size_t len = c.arg(2);
    if (!src_ptr || !c.in_bounds(src_ptr, 4)) {
        c.set_result((uint32_t)-1);
        return;
    }
    uint32_t src = c.read32(src_ptr);
    size_t count = 0;
    while (count < len) {
        if (!c.in_bounds(src, 4)) break;
        uint32_t wc = c.read32(src);
        src += 4;
        if (wc == 0) break;
        if (dst && c.in_bounds(dst + (uint32_t)count, 1)) {
            c.write8(dst + (uint32_t)count, (uint8_t)(wc & 0x7F));
        }
        ++count;
    }
    c.set_result((uint32_t)count);
}

void f_inet_ntoa(GuestCall &c) {
    uint32_t in = c.arg(0);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                  (in >> 0) & 0xFF, (in >> 8) & 0xFF,
                  (in >> 16) & 0xFF, (in >> 24) & 0xFF);
    c.set_result(c.dup_cstr(buf));
}

void f_getservbyname(GuestCall &c) {
    c.set_result(0);
}

void f_zError(GuestCall &c) {
    int err = (int)c.arg(0);
    const char *msg = zError(err);
    c.set_result(c.dup_cstr(msg ? msg : "unknown error"));
}

/* --- Bionic fortified / memory stubs --- */

void f___memcpy_chk(GuestCall &c) {
    uint32_t dest = c.arg(0);
    uint32_t src = c.arg(1);
    uint32_t len = c.arg(2);
    uint32_t dest_len = c.arg(3);
    if (len > dest_len) {
        c.log("[-] __memcpy_chk overflow! len=%u > dest_len=%u", len, dest_len);
    }
    if (len > 0 && c.in_bounds(dest, len) && c.in_bounds(src, len)) {
        std::memcpy(c.ptr(dest, len), c.ptr(src, len), len);
    }
    c.set_result(dest);
}

void f___strncpy_chk2(GuestCall &c) {
    uint32_t dest = c.arg(0);
    uint32_t src = c.arg(1);
    uint32_t n = c.arg(2);
    uint32_t dest_len = c.arg(3);
    if (n > dest_len) {
        c.log("[-] __strncpy_chk2 overflow! n=%u > dest_len=%u", n, dest_len);
    }
    if (n > 0 && c.in_bounds(dest, n) && c.in_bounds(src, 1)) {
        const char *s = (const char*)c.ptr(src, 1);
        char *d = (char*)c.ptr(dest, n);
        std::strncpy(d, s, n);
    }
    c.set_result(dest);
}

void f___vsnprintf_chk(GuestCall &c) {
    uint32_t dst = c.arg(0);
    uint32_t maxlen = c.arg(1);
    uint32_t fmt_ptr = c.arg(4);
    uint32_t ap_addr = c.arg(5);
    auto src = libc::VaSource::from_va_list(c, ap_addr);
    std::string res = libc::format(c, fmt_ptr, src);
    if (maxlen > 0 && dst != 0 && c.in_bounds(dst, 1)) {
        size_t n = (res.size() < (size_t)maxlen) ? res.size() : (size_t)(maxlen - 1);
        if (n > 0 && c.in_bounds(dst, (uint32_t)n)) {
            std::memcpy(c.ptr(dst, (uint32_t)n), res.data(), n);
        }
        c.write8(dst + (uint32_t)n, 0);
    }
    c.set_result((uint32_t)res.size());
}

void f_strnlen(GuestCall &c) {
    uint32_t s_addr = c.arg(0);
    uint32_t maxlen = c.arg(1);
    uint32_t len = 0;
    while (len < maxlen && c.in_bounds(s_addr + len, 1)) {
        if (c.read8(s_addr + len) == 0) break;
        ++len;
    }
    c.set_result(len);
}

void f___FD_SET_chk(GuestCall &c) {
    int fd = (int)c.arg(0);
    uint32_t set_ptr = c.arg(1);
    uint32_t set_size = c.arg(2);
    if (fd >= 0 && (uint32_t)(fd / 8) < set_size && c.in_bounds(set_ptr + (fd / 8), 1)) {
        uint8_t byte = c.read8(set_ptr + (fd / 8));
        byte |= (1 << (fd % 8));
        c.write8(set_ptr + (fd / 8), byte);
    }
}

void f___FD_ISSET_chk(GuestCall &c) {
    int fd = (int)c.arg(0);
    uint32_t set_ptr = c.arg(1);
    uint32_t set_size = c.arg(2);
    uint32_t res = 0;
    if (fd >= 0 && (uint32_t)(fd / 8) < set_size && c.in_bounds(set_ptr + (fd / 8), 1)) {
        uint8_t byte = c.read8(set_ptr + (fd / 8));
        res = (byte & (1 << (fd % 8))) ? 1 : 0;
    }
    c.set_result(res);
}

void f_android_set_abort_message(GuestCall &c) {
    uint32_t msg_ptr = c.arg(0);
    if (msg_ptr) {
        c.log("[android_set_abort_message] %s", c.cstr(msg_ptr).c_str());
    }
}

void f_sigemptyset(GuestCall &c) {
    uint32_t set_ptr = c.arg(0);
    if (set_ptr && c.in_bounds(set_ptr, 8)) {
        c.write32(set_ptr, 0);
        c.write32(set_ptr + 4, 0);
    }
    c.set_result(0);
}

/* --- Math --- */

void f_fminf(GuestCall &c) {
    float x = c.argf(0);
    float y = c.argf(1);
    c.set_resultf(std::fmin(x, y));
}

void f_sincos(GuestCall &c) {
    double x = c.argd(0);
    uint32_t sin_ptr = c.arg(2);
    uint32_t cos_ptr = c.arg(3);
    double s = std::sin(x);
    double co = std::cos(x);
    if (sin_ptr && c.in_bounds(sin_ptr, 8)) {
        uint64_t u;
        std::memcpy(&u, &s, 8);
        c.write32(sin_ptr, (uint32_t)u);
        c.write32(sin_ptr + 4, (uint32_t)(u >> 32));
    }
    if (cos_ptr && c.in_bounds(cos_ptr, 8)) {
        uint64_t u;
        std::memcpy(&u, &co, 8);
        c.write32(cos_ptr, (uint32_t)u);
        c.write32(cos_ptr + 4, (uint32_t)(u >> 32));
    }
}

void f_sincosf(GuestCall &c) {
    float x = c.argf(0);
    uint32_t sin_ptr = c.arg(1);
    uint32_t cos_ptr = c.arg(2);
    float s = std::sin(x);
    float co = std::cos(x);
    if (sin_ptr && c.in_bounds(sin_ptr, 4)) {
        uint32_t u;
        std::memcpy(&u, &s, 4);
        c.write32(sin_ptr, u);
    }
    if (cos_ptr && c.in_bounds(cos_ptr, 4)) {
        uint32_t u;
        std::memcpy(&u, &co, 4);
        c.write32(cos_ptr, u);
    }
}

/* --- File/dir/syslog --- */

void f_statvfs(GuestCall &c) {
    uint32_t buf = c.arg(1);
    if (buf && c.in_bounds(buf, 64)) {
        void *p = c.ptr(buf, 64);
        if (p) std::memset(p, 0, 64);
        c.write32(buf + 0, 4096);
        c.write32(buf + 4, 4096);
        c.write32(buf + 8, 1000000);
        c.write32(buf + 12, 500000);
        c.write32(buf + 16, 500000);
    }
    c.set_result(0);
}

void f_fdopendir(GuestCall &c) {
    c.set_result(0xDEAD0001);
}

void f_openlog(GuestCall &c) { (void)c; }
void f_closelog(GuestCall &c) { (void)c; }
void f_syslog(GuestCall &c) {
    int prio = (int)c.arg(0);
    uint32_t fmt_ptr = c.arg(1);
    auto src = libc::VaSource::from_registers(c, 2);
    std::string msg = libc::format(c, fmt_ptr, src);
    c.log("[syslog:%d] %s", prio, msg.c_str());
}

/* --- Locale --- */

void f_newlocale(GuestCall &c) { c.set_result(1); }
void f_uselocale(GuestCall &c) { c.set_result(1); }
void f_freelocale(GuestCall &c) { (void)c; }
void f___ctype_get_mb_cur_max(GuestCall &c) { c.set_result(1); }

void f_iswalpha_l(GuestCall &c) { c.set_result(std::iswalpha((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswblank_l(GuestCall &c) { c.set_result(std::iswblank((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswcntrl_l(GuestCall &c) { c.set_result(std::iswcntrl((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswdigit_l(GuestCall &c) { c.set_result(std::iswdigit((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswlower_l(GuestCall &c) { c.set_result(std::iswlower((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswprint_l(GuestCall &c) { c.set_result(std::iswprint((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswpunct_l(GuestCall &c) { c.set_result(std::iswpunct((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswspace_l(GuestCall &c) { c.set_result(std::iswspace((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswupper_l(GuestCall &c) { c.set_result(std::iswupper((wint_t)c.arg(0)) ? 1 : 0); }
void f_iswxdigit_l(GuestCall &c){ c.set_result(std::iswxdigit((wint_t)c.arg(0)) ? 1 : 0); }

void f_towlower_l(GuestCall &c) { c.set_result((uint32_t)std::towlower((wint_t)c.arg(0))); }
void f_towupper_l(GuestCall &c) { c.set_result((uint32_t)std::towupper((wint_t)c.arg(0))); }

void f_strcoll_l(GuestCall &c) {
    c.set_result((uint32_t)std::strcmp(c.cstr(c.arg(0)).c_str(), c.cstr(c.arg(1)).c_str()));
}
void f_wcscoll_l(GuestCall &c) {
    libc::compare_wide(c, libc::kUnbounded, true);
}
void f_strxfrm_l(GuestCall &c) {
    uint32_t dst = c.arg(0);
    std::string src = c.cstr(c.arg(1));
    uint32_t n = c.arg(2);
    if (dst && n > 0 && c.in_bounds(dst, 1)) {
        size_t cp = (src.size() < (size_t)n) ? src.size() : (size_t)(n - 1);
        std::memcpy(c.ptr(dst, (uint32_t)cp), src.data(), cp);
        c.write8(dst + (uint32_t)cp, 0);
    }
    c.set_result((uint32_t)src.size());
}
void f_wcsxfrm_l(GuestCall &c) {
    c.set_result(0);
}
void f_strftime_l(GuestCall &c) {
    uint32_t s_ptr = c.arg(0);
    uint32_t max_sz = c.arg(1);
    std::string fmt = c.cstr(c.arg(2));
    uint32_t tm_ptr = c.arg(3);
    if (!s_ptr || max_sz == 0 || !tm_ptr || !c.in_bounds(tm_ptr, 44)) {
        c.set_result(0);
        return;
    }
    struct tm t{};
    t.tm_sec = c.read32(tm_ptr + 0);
    t.tm_min = c.read32(tm_ptr + 4);
    t.tm_hour = c.read32(tm_ptr + 8);
    t.tm_mday = c.read32(tm_ptr + 12);
    t.tm_mon = c.read32(tm_ptr + 16);
    t.tm_year = c.read32(tm_ptr + 20);
    t.tm_wday = c.read32(tm_ptr + 24);
    t.tm_yday = c.read32(tm_ptr + 28);
    t.tm_isdst = c.read32(tm_ptr + 32);
    std::vector<char> buf(max_sz);
    size_t written = std::strftime(buf.data(), max_sz, fmt.c_str(), &t);
    if (written > 0 && c.in_bounds(s_ptr, (uint32_t)written + 1)) {
        std::memcpy(c.ptr(s_ptr, (uint32_t)written + 1), buf.data(), written + 1);
    }
    c.set_result((uint32_t)written);
}

void f_strtold(GuestCall &c) {
    std::string s = c.cstr(c.arg(0));
    char *end = nullptr;
    double val = std::strtod(s.c_str(), &end);
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), c.arg(0) + (uint32_t)(end - s.c_str()));
    }
    c.set_resultd(val);
}
void f_strtold_l(GuestCall &c) { f_strtold(c); }

void f_strtoll_l(GuestCall &c) {
    std::string s = c.cstr(c.arg(0));
    char *end = nullptr;
    int base = (int)c.arg(2);
    long long val = std::strtoll(s.c_str(), &end, base);
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), c.arg(0) + (uint32_t)(end - s.c_str()));
    }
    c.set_result64((uint64_t)val);
}

void f_strtoull_l(GuestCall &c) {
    std::string s = c.cstr(c.arg(0));
    char *end = nullptr;
    int base = (int)c.arg(2);
    unsigned long long val = std::strtoull(s.c_str(), &end, base);
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), c.arg(0) + (uint32_t)(end - s.c_str()));
    }
    c.set_result64((uint64_t)val);
}

void f_mbtowc(GuestCall &c) {
    uint32_t pwc = c.arg(0);
    uint32_t s = c.arg(1);
    uint32_t n = c.arg(2);
    if (!s) { c.set_result(0); return; }
    if (n == 0 || !c.in_bounds(s, 1)) { c.set_result((uint32_t)-1); return; }
    uint8_t byte = c.read8(s);
    if (pwc && c.in_bounds(pwc, 4)) c.write32(pwc, (uint32_t)byte);
    c.set_result(byte ? 1 : 0);
}

void f_mbsrtowcs(GuestCall &c) {
    uint32_t dst = c.arg(0);
    uint32_t src_ptr = c.arg(1);
    uint32_t len = c.arg(2);
    if (!src_ptr || !c.in_bounds(src_ptr, 4)) {
        c.set_result((uint32_t)-1);
        return;
    }
    uint32_t src = c.read32(src_ptr);
    uint32_t count = 0;
    while (count < len) {
        if (!c.in_bounds(src, 1)) break;
        uint8_t b = c.read8(src++);
        if (b == 0) {
            if (dst && c.in_bounds(dst + count * 4, 4)) c.write32(dst + count * 4, 0);
            src = 0;
            break;
        }
        if (dst && c.in_bounds(dst + count * 4, 4)) c.write32(dst + count * 4, (uint32_t)b);
        ++count;
    }
    c.write32(src_ptr, src);
    c.set_result(count);
}

void f_mbsnrtowcs(GuestCall &c) {
    uint32_t dst = c.arg(0);
    uint32_t src_ptr = c.arg(1);
    uint32_t nmc = c.arg(2);
    uint32_t len = c.arg(3);
    if (!src_ptr || !c.in_bounds(src_ptr, 4)) {
        c.set_result((uint32_t)-1);
        return;
    }
    uint32_t src = c.read32(src_ptr);
    uint32_t count = 0, consumed = 0;
    while (count < len && consumed < nmc) {
        if (!c.in_bounds(src, 1)) break;
        uint8_t b = c.read8(src++);
        ++consumed;
        if (b == 0) {
            if (dst && c.in_bounds(dst + count * 4, 4)) c.write32(dst + count * 4, 0);
            src = 0;
            break;
        }
        if (dst && c.in_bounds(dst + count * 4, 4)) c.write32(dst + count * 4, (uint32_t)b);
        ++count;
    }
    c.write32(src_ptr, src);
    c.set_result(count);
}

void f_wcsnrtombs(GuestCall &c) {
    uint32_t dst = c.arg(0);
    uint32_t src_ptr = c.arg(1);
    uint32_t nwc = c.arg(2);
    uint32_t len = c.arg(3);
    if (!src_ptr || !c.in_bounds(src_ptr, 4)) {
        c.set_result((uint32_t)-1);
        return;
    }
    uint32_t src = c.read32(src_ptr);
    uint32_t count = 0, consumed = 0;
    while (count < len && consumed < nwc) {
        if (!c.in_bounds(src, 4)) break;
        uint32_t wc = c.read32(src);
        src += 4;
        ++consumed;
        if (wc == 0) {
            if (dst && c.in_bounds(dst + count, 1)) c.write8(dst + count, 0);
            src = 0;
            break;
        }
        if (dst && c.in_bounds(dst + count, 1)) c.write8(dst + count, (uint8_t)(wc & 0x7F));
        ++count;
    }
    c.write32(src_ptr, src);
    c.set_result(count);
}

void f_wcstod(GuestCall &c) {
    uint32_t wstr = c.arg(0);
    std::string s;
    uint32_t i = 0;
    while (c.in_bounds(wstr + i * 4, 4)) {
        uint32_t wc = c.read32(wstr + i * 4);
        if (wc == 0) break;
        s.push_back((char)wc);
        ++i;
    }
    char *end = nullptr;
    double val = std::strtod(s.c_str(), &end);
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), wstr + (uint32_t)(end - s.c_str()) * 4);
    }
    c.set_resultd(val);
}
void f_wcstof(GuestCall &c) {
    f_wcstod(c);
    c.set_resultf((float)c.argd(0));
}
void f_wcstold(GuestCall &c) { f_wcstod(c); }

void f_wcstoll(GuestCall &c) {
    uint32_t wstr = c.arg(0);
    std::string s;
    uint32_t i = 0;
    while (c.in_bounds(wstr + i * 4, 4)) {
        uint32_t wc = c.read32(wstr + i * 4);
        if (wc == 0) break;
        s.push_back((char)wc);
        ++i;
    }
    char *end = nullptr;
    long long val = std::strtoll(s.c_str(), &end, (int)c.arg(2));
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), wstr + (uint32_t)(end - s.c_str()) * 4);
    }
    c.set_result64((uint64_t)val);
}

void f_wcstoul(GuestCall &c) {
    uint32_t wstr = c.arg(0);
    std::string s;
    uint32_t i = 0;
    while (c.in_bounds(wstr + i * 4, 4)) {
        uint32_t wc = c.read32(wstr + i * 4);
        if (wc == 0) break;
        s.push_back((char)wc);
        ++i;
    }
    char *end = nullptr;
    unsigned long val = std::strtoul(s.c_str(), &end, (int)c.arg(2));
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), wstr + (uint32_t)(end - s.c_str()) * 4);
    }
    c.set_result((uint32_t)val);
}

void f_wcstoull(GuestCall &c) {
    uint32_t wstr = c.arg(0);
    std::string s;
    uint32_t i = 0;
    while (c.in_bounds(wstr + i * 4, 4)) {
        uint32_t wc = c.read32(wstr + i * 4);
        if (wc == 0) break;
        s.push_back((char)wc);
        ++i;
    }
    char *end = nullptr;
    unsigned long long val = std::strtoull(s.c_str(), &end, (int)c.arg(2));
    if (c.arg(1) && c.in_bounds(c.arg(1), 4)) {
        c.write32(c.arg(1), wstr + (uint32_t)(end - s.c_str()) * 4);
    }
    c.set_result64((uint64_t)val);
}

void f_swprintf(GuestCall &c) {
    c.set_result(0);
}

void f_slCreateEngine(GuestCall &c) {
    c.log("[OpenSL] slCreateEngine() -> SL_RESULT_SUCCESS");
    uint32_t pEngine = c.arg(0);
    if (pEngine && c.in_bounds(pEngine, 4)) {
        c.write32(pEngine, 0x00008500);
    }
    c.set_result(0);
}

} // namespace

void register_libc_extra(ImportTable &t) {
    t.add("fseeko", f_fseeko);
    t.add("ftello", f_ftello);
    t.add("fgetws", f_fgetws);
    t.add("perror", f_perror);
    t.add("ftime", f_ftime);
    t.add("frexp", f_frexp);
    t.add("ldexp", f_ldexp);
    t.add("lrintf", f_lrintf);
    t.add("memmem", f_memmem);
    t.add("getrlimit", f_getrlimit);
    t.add("setrlimit", f_setrlimit);
    t.add("getrusage", f_getrusage);
    t.add("chown", f_chown);
    t.add("pthread_attr_getschedpolicy", f_pthread_attr_getschedpolicy);
    t.add("pthread_attr_getstacksize", f_pthread_attr_getstacksize);
    t.add("wcsrtombs", f_wcsrtombs);
    t.add("inet_ntoa", f_inet_ntoa);
    t.add("getservbyname", f_getservbyname);
    t.add("zError", f_zError);

    /* Fortified memory/string */
    t.add("__memcpy_chk", f___memcpy_chk);
    t.add("__strncpy_chk2", f___strncpy_chk2);
    t.add("__vsnprintf_chk", f___vsnprintf_chk);
    t.add("strnlen", f_strnlen);
    t.add("__FD_SET_chk", f___FD_SET_chk);
    t.add("__FD_ISSET_chk", f___FD_ISSET_chk);
    t.add("android_set_abort_message", f_android_set_abort_message);
    t.add("sigemptyset", f_sigemptyset);

    /* Math */
    t.add("fminf", f_fminf);
    t.add("sincos", f_sincos);
    t.add("sincosf", f_sincosf);

    /* File/syslog */
    t.add("statvfs", f_statvfs);
    t.add("fdopendir", f_fdopendir);
    t.add("openlog", f_openlog);
    t.add("syslog", f_syslog);
    t.add("closelog", f_closelog);

    /* Locale */
    t.add("newlocale", f_newlocale);
    t.add("uselocale", f_uselocale);
    t.add("freelocale", f_freelocale);
    t.add("__ctype_get_mb_cur_max", f___ctype_get_mb_cur_max);

    t.add("iswalpha_l", f_iswalpha_l);
    t.add("iswblank_l", f_iswblank_l);
    t.add("iswcntrl_l", f_iswcntrl_l);
    t.add("iswdigit_l", f_iswdigit_l);
    t.add("iswlower_l", f_iswlower_l);
    t.add("iswprint_l", f_iswprint_l);
    t.add("iswpunct_l", f_iswpunct_l);
    t.add("iswspace_l", f_iswspace_l);
    t.add("iswupper_l", f_iswupper_l);
    t.add("iswxdigit_l", f_iswxdigit_l);
    t.add("towlower_l", f_towlower_l);
    t.add("towupper_l", f_towupper_l);

    t.add("strcoll_l", f_strcoll_l);
    t.add("wcscoll_l", f_wcscoll_l);
    t.add("strxfrm_l", f_strxfrm_l);
    t.add("wcsxfrm_l", f_wcsxfrm_l);
    t.add("strftime_l", f_strftime_l);

    t.add("strtold", f_strtold);
    t.add("strtold_l", f_strtold_l);
    t.add("strtoll_l", f_strtoll_l);
    t.add("strtoull_l", f_strtoull_l);

    t.add("mbtowc", f_mbtowc);
    t.add("mbsrtowcs", f_mbsrtowcs);
    t.add("mbsnrtowcs", f_mbsnrtowcs);
    t.add("wcsnrtombs", f_wcsnrtombs);

    t.add("wcstod", f_wcstod);
    t.add("wcstof", f_wcstof);
    t.add("wcstold", f_wcstold);
    t.add("wcstoll", f_wcstoll);
    t.add("wcstoul", f_wcstoul);
    t.add("wcstoull", f_wcstoull);

    t.add("swprintf", f_swprintf);
    t.add("slCreateEngine", f_slCreateEngine);
}

} // namespace pvz_tv
