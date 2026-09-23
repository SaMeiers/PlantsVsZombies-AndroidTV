<div align="center">

# 架构说明

**[English](./ARCHITECTURE.md)** | **简体中文**

</div>

游戏本体是 32 位 ARM 库. 本仓库基于它们构建两种产物:

- **armeabi-v7a**: 模组 (`libHomura.so`) 与原始库一同加载并就地 hook, 与 TV 版一贯的做法相同.
- **arm64-v8a 与 PC**: 原始库无法原生运行, 因此由 *runner* 把它们映射进 512 MB 的
  guest 地址空间, 并在 [Dynarmic](https://github.com/SaMeiers/dynarmic) 的 ARM32 JIT 上执行.

## 目录结构

| 路径 | 内容 |
| --- | --- |
| `app/` | Android 应用: Java Activity, 模组源码, 以及 Gradle 调用的 CMake 入口. |
| `runner/include/pvz_tv/` | runner 的公共头文件. |
| `runner/src/elf32/` | 加载 guest `.so`: 映射段, 应用重定位, 解析导入符号. |
| `runner/src/runtime/` | guest 的堆, 线程, 互斥量, 以及 guest 调用的 zlib. |
| `runner/src/dependencies/` | 在宿主上实现的 guest libc, libm, libz, libdl, liblog 等. |
| `runner/src/platform/android/` | EGL/GLES/OpenSL 后端, JNI 桥接, 以及 runner 主循环. |
| `runner/src/platform/desktop/` | 同样的后端, 基于 SDL2 与 glad. |
| `desktop/` | PC 播放器可执行文件. |
| `third_party/` | dynarmic (子模块), 它所需的 Boost 头文件, 以及 PC 构建用的 SDL/zlib/glad. |

`platform/` 之外的代码全部共享: **修一处 guest libc, Android 应用与 PC 播放器同时受益**.
本移植此前的多数 bug, 都源于两边各留了一份相同文件的副本.

## guest 调用如何到达宿主

加载器会把每个导入符号变成一条合成的 `SVC #index` 指令. JIT 执行到它时,
`CallSVC` 按索引在导入表中查找并调用对应的 handler, handler 直接从 guest 寄存器和内存读取参数:

```
guest 代码 ── bl malloc ──▶ 跳板 (SVC #n) ──▶ CallSVC ──▶ c_malloc(GuestCall&)
```

`GuestCall` (见 [`dependency.h`](/runner/include/pvz_tv/dependencies/dependency.h))
囊括了 handler 可以接触的一切: 参数, guest 内存, 宿主文件句柄,
以及少数需要重新进入 guest 代码的场景所用的钩子.

## 与 Android 交互

游戏期望拿到通常由 Java 侧构造的 Transmension `NativeApp`/`BridgeApp` 对象.
runner 在 `setup_transmension_bridge()` 中自行构造它们, 并代替 Java UI 线程:
当 guest 把工作排入 "Java" 队列并写入唤醒管道时, runner 就地执行该工作,
并把能识别的请求 (例如弹出键盘) 转换成真正的 Android 调用.

## 给 guest 打补丁

少数修复是在启动时对已加载镜像做的字节补丁. 添加之前请先确认 `libHomura.so`
是否已经 hook 了该函数: 模组 inline hook 了数百个函数,
补丁若落在 hook 的跳转指令上就会把它破坏. `HookInit.cpp` 与 `Symbols.cpp`
列出了模组 hook 的内容, 而 `runner_core.cpp` 会在模组已加载时跳过自己的触控补丁.

## 调试

- guest 与 runner 的日志输出到 logcat, 标签为 `RunnerGuest`, `RunnerCore`,
  `RunnerEGL`, `RunnerJNI` 和 `RunnerAudio`; PC 上则输出到 stdout.
- guest 崩溃时会打印寄存器, 以及 guest 栈上可能的返回地址, 并解析为 `模块+偏移`.
- Debug 构建还会运行一个 watchdog, 每两秒报告各 guest 线程的状态.
- 要把偏移还原成函数, 反汇编 guest 库:
  `llvm-objdump -d --triple=thumbv7 -C libGameMain.so`.
