# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.20.2)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(simplejson-populate NONE)

include(ExternalProject)
ExternalProject_Add(simplejson-populate
                     "UPDATE_DISCONNECTED" "False" "GIT_REPOSITORY" "https://github.com/NotAPenguin0/SimpleJSON"
                    SOURCE_DIR          "C:/Users/Michiel/Documents/dev/andromeda/cmake-build-release/_deps/simplejson-src"
                    BINARY_DIR          "C:/Users/Michiel/Documents/dev/andromeda/cmake-build-release/_deps/simplejson-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)
