#ifndef PVZ2NATIVE_SURFACE_H
#define PVZ2NATIVE_SURFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* How big the drawing surface is. One value, one setter, four readers.
 *
 * This used to be three separate stores plus two locals in main.c, and every one
 * of them always held the same number:
 *
 *   runtime::window_width/height     what onSurfaceChanged is passed
 *   dex::screen_width/height         what Graphics_GetScreenSizeInPixels answers
 *   libgles g_drawable_w/h           what gl_glViewport substitutes for a zero
 *                                    -size composite viewport
 *   main.c g_win_w/h                 the denominator of the touch mapping
 *
 * They existed because those four layers must not include each other, not
 * because the value differs -- so session.cpp fanned the same pair out by hand,
 * in two places (startup and resize), and a fifth reader meant remembering a
 * third. Two of the three stores were also plain (non-atomic) ints written from
 * the SDL thread and read from the frame thread. Their defaults had drifted apart
 * too: 960x540 in runtime, 1280x720 in dex, 0x0 in the GL layer -- so whichever
 * layer was read before the fan-out ran answered a different resolution.
 *
 * C-callable because main.c (the SDL host loop) is C and the readers are C++,
 * same arrangement as config.h.
 */

/* The live surface size, in pixels. Called by the host: once before the session
 * starts, then on every window resize. Safe from any thread. */
void pvz2_surface_set(uint32_t width, uint32_t height);
uint32_t pvz2_surface_width(void);
uint32_t pvz2_surface_height(void);

/* The touch coordinate space, frozen at the FIRST pvz2_surface_set -- i.e. the
 * launch resolution -- and never updated afterwards.
 *
 * This is not a caching trick, it is what the engine does: LawnApp's touch scaler
 * keys off mOrigScreenWidth, which SetWidthHeight fills in once at startup and
 * the resize path never revisits. onSurfaceChanged updates the projection but not
 * that. So window pixels must always be mapped back into this fixed space; making
 * it follow the window (identity mapping) puts every click in the wrong place
 * once the window is not exactly the launch size. Freezing it here rather than in
 * the host means no caller has to remember the rule. */
uint32_t pvz2_touch_space_width(void);
uint32_t pvz2_touch_space_height(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
