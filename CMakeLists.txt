cmake_minimum_required(VERSION 3.16)
project(TaskTreeQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands so clangd in Neovim knows Qt symbols
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Qt MOC (Meta-Object Compiler) and UIC automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Automatically collect all .cpp and .h files in src/
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(TaskTreeQt ${SOURCES} ${HEADERS})

target_link_libraries(TaskTreeQt PRIVATE Qt6::Widgets)
