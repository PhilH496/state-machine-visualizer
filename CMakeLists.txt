cmake_minimum_required(VERSION 3.10)
project(StateMachineVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or download nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Source files
set(BACKEND_SOURCES
    src/main.cpp
    src/converter.cpp
    src/NFA.cpp
    src/DFA.cpp
)

# Header files
set(BACKEND_HEADERS
    src/converter.hpp
    include/NFA.h
    include/DFA.h
)

# Create executable
add_executable(state_machine_visualizer ${BACKEND_SOURCES} ${BACKEND_HEADERS})

# Link nlohmann/json
target_link_libraries(state_machine_visualizer nlohmann_json::nlohmann_json)

# Set output directory
set_target_properties(state_machine_visualizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)