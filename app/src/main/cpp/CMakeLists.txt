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

else() # arm64-v8a: the guest libraries run on the shared Dynarmic runner
    # Repository root, so this file does not care where it is checked out.
    get_filename_component(PVZTV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

    add_subdirectory("${PVZTV_ROOT}/third_party" "${CMAKE_BINARY_DIR}/third_party")
    add_subdirectory("${PVZTV_ROOT}/runner" "${CMAKE_BINARY_DIR}/runner")

    # pvztv_runner builds as libnative_code.so here (see runner/CMakeLists.txt).

    find_library(log-lib log)
    add_library(Homura SHARED "${PVZTV_ROOT}/runner/src/platform/android/homura_bridge.cpp")
    target_include_directories(Homura PRIVATE
        Homura/include
        PvZ/include
        "${PVZTV_DYNARMIC_SRC_DIR}"
    )
    target_link_libraries(Homura PRIVATE pvztv_runner ${log-lib})
endif()
