cmake_minimum_required(VERSION 3.15)
project(ProceduralTerrainProject)

add_subdirectory(thirdparty/MerlinEngine)

set(PROCEDURAL_TERRAIN_SOURCE
    ProceduralTerrain/main.cpp
)

add_executable(ProceduralTerrain
    ${PROCEDURAL_TERRAIN_SOURCE}
)
set_property(TARGET ProceduralTerrain PROPERTY CXX_STANDARD 17)

target_include_directories(ProceduralTerrain
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ProceduralTerrain
)

target_link_libraries(ProceduralTerrain
    PUBLIC
    Merlin
)
