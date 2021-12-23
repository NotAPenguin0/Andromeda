# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.20.2)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(iconfonts-populate NONE)

include(ExternalProject)
ExternalProject_Add(iconfonts-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/NotAPenguin0/IconFontCppHeaders" "GIT_TAG" "e4c6b0e7bbba588e15195315a2f754e48cec3de1"
                    SOURCE_DIR          "C:/Users/Michiel/Documents/dev/andromeda/cmake-build-release/_deps/iconfonts-src"
                    BINARY_DIR          "C:/Users/Michiel/Documents/dev/andromeda/cmake-build-release/_deps/iconfonts-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)
