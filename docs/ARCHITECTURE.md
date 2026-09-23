<div align="center">

# Architecture

**English** | **[简体中文](./ARCHITECTURE.zh-cn.md)**

</div>

The game ships as 32-bit ARM libraries. This repository builds two things from
them:

- **armeabi-v7a**: the mod (`libHomura.so`) is loaded next to the original
  libraries and hooks them in place, the way the TV release always worked.
- **arm64-v8a and PC**: the original libraries cannot run natively, so a
  *runner* maps them into a 512 MB guest address space and executes their code
  on a [Dynarmic](https://github.com/SaMeiers/dynarmic) ARM32 JIT.

## Layout

| Path | What lives there |
| --- | --- |
| `app/` | The Android app: Java activities, the mod sources, and the CMake entry point Gradle drives. |
| `runner/include/pvz_tv/` | Public headers of the runner. |
| `runner/src/elf32/` | Loads the guest `.so` files: maps segments, applies relocations, resolves imports. |
| `runner/src/runtime/` | Guest heap, threads, mutexes and the zlib the guest calls into. |
| `runner/src/dependencies/` | The guest's libc, libm, libz, libdl, liblog and friends, implemented on the host. |
| `runner/src/platform/android/` | EGL/GLES/OpenSL backends, the JNI bridge and the runner's core loop. |
| `runner/src/platform/desktop/` | The same backends over SDL2 and glad. |
| `desktop/` | The PC player executable. |
| `third_party/` | dynarmic (submodule), the Boost headers it needs, and SDL/zlib/glad for the PC build. |

Everything outside `platform/` is shared: **a fix in the guest libc reaches both
the Android app and the PC player**. Most of the bugs this port has had came from
the two sides holding separate copies of the same file.

## How a guest call reaches the host

The loader turns every imported symbol into a synthetic `SVC #index`
instruction. When the JIT hits one, `CallSVC` looks the index up in the import
table and calls the matching handler, which reads arguments straight out of the
guest registers and memory:

```
guest code ── bl malloc ──▶ trampoline (SVC #n) ──▶ CallSVC ──▶ c_malloc(GuestCall&)
```

`GuestCall` (see [`dependency.h`](/runner/include/pvz_tv/dependencies/dependency.h))
is everything a handler may touch: arguments, guest memory, host file handles,
and hooks for the few cases that need to re-enter guest code.

## Talking to Android

The game expects the Transmension `NativeApp`/`BridgeApp` objects that its Java
side normally builds. The runner builds them itself in
`setup_transmension_bridge()` and stands in for the Java UI thread: when the
guest queues work for "Java" and writes to its wake-up pipe, the runner runs
that work right there, and turns the requests it recognises (showing the
keyboard, for instance) into real Android calls.

## Patching the guest

A few fixes are byte patches applied to the loaded image at startup. Before
adding one, check whether `libHomura.so` already hooks that function: it
inline-hooks hundreds of them, and a patch landing on a hook's jump instruction
corrupts it. `HookInit.cpp` and `Symbols.cpp` list what the mod hooks, and
`runner_core.cpp` skips its own touch-UI patches whenever the mod is loaded.

## Debugging

- Guest and runner logs go to logcat under the tags `RunnerGuest`, `RunnerCore`,
  `RunnerEGL`, `RunnerJNI` and `RunnerAudio`; on PC they go to stdout.
- A guest crash dumps registers plus the plausible return addresses on the guest
  stack, each resolved to `module+offset`.
- Debug builds also run a watchdog that reports, every two seconds, what each
  guest thread is doing.
- To map an offset back to a function, disassemble the guest library:
  `llvm-objdump -d --triple=thumbv7 -C libGameMain.so`.
