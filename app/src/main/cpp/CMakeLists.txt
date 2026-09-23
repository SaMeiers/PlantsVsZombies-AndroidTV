cmake_minimum_required(VERSION 3.20) # 3.20 is for C++23
project(PvZTV)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(CMAKE_C_VISIBILITY_PRESET hidden)
        set(CMAKE_CXX_VISIBILITY_PRESET hidden) # 隐藏非 inline 符号
        set(CMAKE_VISIBILITY_INLINES_HIDDEN ON) # 隐藏 inline 符号
    endif ()
    string(APPEND CMAKE_C_FLAGS " -flto=thin")
    string(APPEND CMAKE_CXX_FLAGS " -flto=thin") # 启用链接时优化
endif ()

if (ANDROID_ABI STREQUAL "armeabi-v7a")
    file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
            "Homura/src/*.cpp"
            "PvZ/src/*.cpp"
    )
    add_library(Homura SHARED Main.cpp ${SRC_FILES})

    target_include_directories(Homura PRIVATE Homura/include PvZ/include)

    target_compile_options(Homura PRIVATE
            -Werror
            -Wall
            -Wimplicit-fallthrough
            -Wunused-but-set-variable
    )

    target_compile_definitions(Homura PRIVATE PVZ_VERSION=${PVZ_VERSION})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(Homura PRIVATE PVZ_DEBUG=1)
    endif ()

    add_subdirectory(ThirdParty)

    find_library(android-lib android)
    find_library(EGL-lib EGL)
    find_library(GLESv2-lib GLESv2)
    find_library(log-lib log)
    find_library(OpenSLES-lib OpenSLES)
    target_link_libraries(Homura PRIVATE
            Substrate
            ${android-lib}
            ${EGL-lib}
            ${GLESv2-lib}
            ${log-lib}
            ${OpenSLES-lib}
    )

else() # arm64-v8a: Dynarmic ARM32 translation runner + 64-bit Homura bridge
    set(DYNARMIC_FRONTENDS "A32" CACHE STRING "" FORCE)
    set(DYNARMIC_TESTS OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    set(Boost_INCLUDE_DIR "C:/proyectos/pvzpc-deps/boost")
    add_subdirectory("C:/proyectos/pvzpc-deps/dynarmic" ${CMAKE_BINARY_DIR}/dynarmic EXCLUDE_FROM_ALL)
    if (TARGET fmt)
        target_compile_options(fmt PUBLIC "-DFMT_CONSTEVAL=")
    endif()

    set(RUNNER_SRCS
        runner/src/elf32/elf32_loader.c
        runner/src/runtime/guest_runtime.cpp
        runner/src/runtime/guest_heap.cpp
        runner/src/runtime/guest_zlib.cpp
        runner/src/surface.cpp
        runner/src/config.cpp
        runner/src/dependencies/dependency.cpp
        runner/src/dependencies/libandroid.cpp
        runner/src/dependencies/libaudio_android.cpp
        runner/src/dependencies/libc_ctype.cpp
        runner/src/dependencies/libc_extra.cpp
        runner/src/dependencies/libc_locale.cpp
        runner/src/dependencies/libc_misc.cpp
        runner/src/dependencies/libc_pthread.cpp
        runner/src/dependencies/libc_socket.cpp
        runner/src/dependencies/libc_stdio.cpp
        runner/src/dependencies/libc_stdlib.cpp
        runner/src/dependencies/libc_string.cpp
        runner/src/dependencies/libc_time.cpp
        runner/src/dependencies/libc_unistd.cpp
        runner/src/dependencies/libc_wchar.cpp
        runner/src/dependencies/libdl.cpp
        runner/src/dependencies/libegl_android.cpp
        runner/src/dependencies/libgles_android.cpp
        runner/src/dependencies/liblog.cpp
        runner/src/dependencies/libm.cpp
        runner/src/dependencies/libstdcxx.cpp
        runner/src/dependencies/libz.cpp
        runner/src/dependencies/vfs.cpp
        runner/src/runner_core.cpp
        runner/src/runner_jni.cpp
    )

    add_library(native_code SHARED ${RUNNER_SRCS})
    target_include_directories(native_code PRIVATE
        runner/include
        runner/src
    )

    find_library(android-lib android)
    find_library(EGL-lib EGL)
    find_library(GLESv1-lib GLESv1_CM)
    find_library(GLESv2-lib GLESv2)
    find_library(log-lib log)
    find_library(OpenSLES-lib OpenSLES)
    find_library(z-lib z)

    target_link_libraries(native_code PRIVATE
        dynarmic
        ${android-lib}
        ${EGL-lib}
        ${GLESv1-lib}
        ${GLESv2-lib}
        ${log-lib}
        ${OpenSLES-lib}
        ${z-lib}
    )

    add_library(Homura SHARED runner/src/homura_bridge.cpp)
    target_include_directories(Homura PRIVATE
        runner/include
        runner/src
        Homura/include
        PvZ/include
        C:/proyectos/pvzpc-deps/dynarmic/src
    )
    target_link_libraries(Homura PRIVATE
        native_code
        ${log-lib}
    )
endif()
