# Third-party dependencies.
#
# dynarmic and the Boost headers it needs ship with the repository (dynarmic as
# a submodule, Boost as the header subset in boost/), so a plain
# `git clone --recursive` is enough to build. Set PVZTV_DEPS_DIR to reuse copies
# you already have on disk instead, e.g.
#   cmake -DPVZTV_DEPS_DIR=C:/my/deps ...
# which is looked at for dynarmic/, boost/, SDL/, zlib/ and glad/.

set(PVZTV_DEPS_DIR "" CACHE PATH "Directory holding local copies of the dependencies")

# Returns the directory to use for a dependency: the local override if it
# exists, otherwise the in-tree copy.
function(pvztv_dep_dir out name in_tree)
    if(PVZTV_DEPS_DIR AND EXISTS "${PVZTV_DEPS_DIR}/${name}")
        set(${out} "${PVZTV_DEPS_DIR}/${name}" PARENT_SCOPE)
    else()
        set(${out} "${in_tree}" PARENT_SCOPE)
    endif()
endfunction()

# --- Boost (headers only: dynarmic uses boost::icl and boost::variant) -------
pvztv_dep_dir(PVZTV_BOOST_DIR boost "${CMAKE_CURRENT_SOURCE_DIR}/boost")
if(NOT EXISTS "${PVZTV_BOOST_DIR}/boost/version.hpp")
    message(FATAL_ERROR "Boost headers not found in ${PVZTV_BOOST_DIR}. "
                        "Point -DPVZTV_DEPS_DIR at a directory containing boost/.")
endif()
set(Boost_INCLUDE_DIR "${PVZTV_BOOST_DIR}" CACHE PATH "" FORCE)

# --- dynarmic ---------------------------------------------------------------
pvztv_dep_dir(PVZTV_DYNARMIC_DIR dynarmic "${CMAKE_CURRENT_SOURCE_DIR}/dynarmic")
if(NOT EXISTS "${PVZTV_DYNARMIC_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "dynarmic not found in ${PVZTV_DYNARMIC_DIR}. "
                        "Run: git submodule update --init --recursive")
endif()
set(DYNARMIC_FRONTENDS "A32" CACHE STRING "" FORCE)
set(DYNARMIC_TESTS OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory("${PVZTV_DYNARMIC_DIR}" "${CMAKE_BINARY_DIR}/dynarmic" EXCLUDE_FROM_ALL)
# fmt 10 makes its format strings consteval, which this dynarmic release trips
# over on the NDK's libc++.
if(TARGET fmt)
    target_compile_options(fmt PUBLIC "-DFMT_CONSTEVAL=")
endif()

# Where dynarmic's private headers live, for the few places that need them.
set(PVZTV_DYNARMIC_SRC_DIR "${PVZTV_DYNARMIC_DIR}/src" PARENT_SCOPE)

# --- desktop-only dependencies ----------------------------------------------
if(NOT ANDROID)
    pvztv_dep_dir(PVZTV_SDL_DIR SDL "${CMAKE_CURRENT_SOURCE_DIR}/SDL")
    pvztv_dep_dir(PVZTV_ZLIB_DIR zlib "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
    pvztv_dep_dir(PVZTV_GLAD_DIR glad "${CMAKE_CURRENT_SOURCE_DIR}/glad")
    # glad keeps its CMake project in a cmake/ subdirectory.
    foreach(dep "SDL;CMakeLists.txt" "ZLIB;CMakeLists.txt" "GLAD;cmake/CMakeLists.txt")
        list(GET dep 0 name)
        list(GET dep 1 marker)
        if(NOT EXISTS "${PVZTV_${name}_DIR}/${marker}")
            message(FATAL_ERROR "${name} not found in ${PVZTV_${name}_DIR}. "
                                "Run: git submodule update --init --recursive")
        endif()
    endforeach()

    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory("${PVZTV_GLAD_DIR}/cmake" "${CMAKE_BINARY_DIR}/glad" EXCLUDE_FROM_ALL)
    glad_add_library(glad REPRODUCIBLE LOADER API egl=1.5 gl:compatibility=2.0)
    add_subdirectory("${PVZTV_ZLIB_DIR}" "${CMAKE_BINARY_DIR}/zlib" EXCLUDE_FROM_ALL)
    add_subdirectory("${PVZTV_SDL_DIR}" "${CMAKE_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)

    set(PVZTV_ZLIB_INCLUDE_DIR "${PVZTV_ZLIB_DIR}" PARENT_SCOPE)
endif()
