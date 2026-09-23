#ifndef PVZ2NATIVE_DEPENDENCIES_LIBC_INTERNAL_H
#define PVZ2NATIVE_DEPENDENCIES_LIBC_INTERNAL_H

#include <cstdint>
#include <string>

#include <pvz_tv/dependencies/dependency.h>

/* Shared between the libc_* modules only -- not part of the public dependency
 * surface. The printf/scanf grammar is implemented once here and reused by
 * stdio (printf/sprintf/snprintf/fprintf...), by liblog (__android_log_print
 * takes the same format string) and by the scanf family. */

namespace pvz_tv {
namespace libc {

/* Where a variadic call's arguments come from.
 *
 * Two shapes exist and they are not interchangeable:
 *  - a direct variadic call (printf, sprintf): arguments follow AAPCS, so the
 *    first four words are r0..r3 and the rest are on the stack past sp.
 *  - a va_list call (vsnprintf, __android_log_vprint): on ARM EABI a va_list is
 *    just a pointer to a contiguous argument block, so everything is read from
 *    guest memory at increasing addresses.
 * Getting this wrong is silent -- the format still "works", it just prints the
 * wrong words -- so the distinction is explicit at every construction site. */
class VaSource {
public:
    /* first_arg is the AAPCS argument index the varargs start at: 1 for
     * printf(fmt, ...), 2 for sprintf(dst, fmt, ...), 3 for snprintf. */
    static VaSource from_registers(GuestCall &c, int first_arg);
    static VaSource from_va_list(GuestCall &c, std::uint32_t va_addr);

    std::uint32_t next_u32();
    std::uint64_t next_u64();  /* 8-byte aligned, per AAPCS */
    double next_double();      /* softfp: a double IS its 64-bit pattern */

private:
    GuestCall *c_ = nullptr;
    bool from_regs_ = false;
    int arg_index_ = 0;        /* register/stack mode */
    std::uint32_t va_addr_ = 0; /* va_list mode */
};

/* Runs a guest format string against a source of arguments, returning what
 * printf would have produced. Each conversion is handed to the HOST's snprintf
 * one specifier at a time (the spec text taken verbatim from the guest string,
 * restricted to a safe character set), so field width/precision/flags behave
 * exactly like the real thing without reimplementing them. */
std::string format(GuestCall &c, std::uint32_t fmt_addr, VaSource &args);

/* The scanf half: consumes `input` per the guest format string, storing through
 * the pointers `args` yields. Returns the number of fields successfully
 * converted (the scanf return value), or -1 for "input exhausted before the
 * first conversion" (EOF). */
int scan(GuestCall &c, const std::string &input, std::uint32_t fmt_addr, VaSource &args);

/* --- lexicographic comparison over guest memory -------------------------- *
 *
 * One loop each for bytes and wide characters, because the whole comparison
 * family differs only in three flags. It used to be six near-identical copies
 * spread over three files -- strcmp/strncmp in libc_string, wcscmp/wcsncmp in
 * libc_wchar, and strcoll/wcscoll in libc_locale, the last two being verbatim
 * copies of the first two (in the C locale, collation IS byte order, which is
 * exactly why they were copied rather than shared).
 *
 * Both read (a, b) from r0/r1 and write the result to r0, so a handler is a
 * single call.
 *
 * `limit` is the maximum number of elements to compare; kUnbounded for the
 * n-less forms. `stop_at_nul` ends the comparison at a terminator, which is what
 * separates the string family (strcmp, wcscmp) from the memory family (memcmp,
 * wmemcmp). `fold_case` lowercases both sides first, for strcasecmp. */
constexpr std::uint32_t kUnbounded = 0xFFFFFFFFu;

void compare_bytes(GuestCall &c, std::uint32_t limit, bool stop_at_nul, bool fold_case);
void compare_wide(GuestCall &c, std::uint32_t limit, bool stop_at_nul);

}  // namespace libc

#if defined(_WIN32)
uint32_t wsa_to_linux_errno(int werr);
#endif

}  // namespace pvz_tv

#endif
