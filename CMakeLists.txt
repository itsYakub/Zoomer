# ------------------------------------------------------------------------------
# OpenGL Template
# https://github.com/itsYakub/OpenGL-Template.git
# ------------------------------------------------------------------------------
# Author:
# https://github.com/itsYakub
# ------------------------------------------------------------------------------
# Naming convention:
# - *.c - source file extension
# - *.h - header file extension
# - src/ - source files directory
# - include/ - header files direcotry
# - bin/ - binary / final exec. directory
# ------------------------------------------------------------------------------
# LICENCE (MIT):
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
# ------------------------------------------------------------------------------

# --------------------
# Section: CMake setup
# --------------------

cmake_minimum_required(VERSION 3.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ----------------------
# Section: Fetch Content
# ----------------------

include(FetchContent)

# -----------------
# Sub-Section: GLAD
# -----------------

FetchContent_Declare(
	glad
	GIT_REPOSITORY https://github.com/Dav1dde/glad.git
	GIT_TAG v0.1.36
)

set(GLAD_PROFILE "core"	CACHE STRING "OpenGL profile")
set(GLAD_GENERATOR "c"	CACHE STRING "Language to generate the binding for")

# -----------------
# Sub-Section: SDL2
# -----------------

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.30.7
)

# -----------------
# Sub-Section: cglm
# -----------------

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm
    GIT_TAG v0.9.4
)
FetchContent_MakeAvailable(glad SDL2 cglm)

# ----------------------
# Section: Project setup
# ----------------------

project(
	opengl_template
	VERSION 1.0
	DESCRIPTION "Basic template for OpenGL development in C"
	HOMEPAGE_URL "https://github.com/itsYakub/OpenGL-Template.git"
	LANGUAGES C CXX
)

# --------------------
# Sub-Section: Sources
# --------------------

set(
	SOURCES
	${CMAKE_SOURCE_DIR}/main.c
	# ...
)

# ---------------------
# Sub-Section: Includes
# ---------------------

set(
	INCLUDE_DIRECTORIES
	# ...
)

# ----------------------
# Sub-Section: Libraries
# ----------------------

set(
	LINK_LIBRARIES
	glad
	SDL2
    cglm
    X11
    m
    # ...
)

# ---------------------
# Section: Target setup
# ---------------------

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

# ----------------------------------
# Section: Compiler & Linker options
# ----------------------------------

if (LINUX)

    # LINUX: Telling the linker to statically link the libgcc and libstdc++ to out project.
    # (Source: https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)

    # LINUX: Setting the target executable's suffix (for linux: *.out).
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".out")

endif(LINUX)

if (WIN32)

    # WINDOWS: Telling the compiler to build a window application (If you want to make a console application, change this option to: `-mconsole`).
    # (Source: https://gcc.gnu.org/onlinedocs/gcc/Cygwin-and-MinGW-Options.html)
    target_compile_options(${PROJECT_NAME} PRIVATE -mwindows)

    # WINDOWS: Telling the linker to statically link our application.
    # (Source: https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
    target_link_options(${PROJECT_NAME} PRIVATE -static)

    # WINDOWS: Setting the target executable's suffix (for Win32: *.exe).
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".exe")

endif(WIN32)

if (APPLE)

    # APPLE: Telling the linker to link to: IOKit - Access hardware devices and drivers from your apps and services.
    # (Source: https://developer.apple.com/documentation/iokit)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")

    # APPLE: Telling the linker to link to: Cocoa - native object-oriented application programming interface.
    # (Source: https://en.wikipedia.org/wiki/Cocoa_(API))
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")

    # APPLE: Telling the linker to link to: OpenGL - cross-language, cross-platform application programming interface for rendering 2D and 3D vector graphics.
    # (Source: https://en.wikipedia.org/wiki/OpenGL)
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")

    # APPLE: Setting the target executable's suffix (for MacOS: *.app).
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".app")

endif(APPLE)
