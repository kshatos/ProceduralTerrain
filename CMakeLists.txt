cmake_minimum_required(VERSION 3.15)
project(ProceduralTerrainProject)

add_subdirectory(thirdparty/MerlinEngine)

set(PROCEDURAL_TERRAIN_SOURCE
    ProceduralTerrain/main.cpp
    ProceduralTerrain/cube_sphere.cpp
    ProceduralTerrain/cube_sphere.hpp
    ProceduralTerrain/noise3d.cpp
    ProceduralTerrain/noise3d.hpp
    ProceduralTerrain/erosion.cpp
    ProceduralTerrain/erosion.hpp
)

add_executable(ProceduralTerrain
    ${PROCEDURAL_TERRAIN_SOURCE}
)
set_property(TARGET ProceduralTerrain PROPERTY CXX_STANDARD 17)
set_property(TARGET ProceduralTerrain PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:ProceduralTerrain>)

target_include_directories(ProceduralTerrain
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ProceduralTerrain
)

target_link_libraries(ProceduralTerrain
    PUBLIC
    Merlin
)
add_custom_command(
    TARGET ProceduralTerrain PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${MERLIN_ASSET_DIR} $<TARGET_FILE_DIR:ProceduralTerrain>/Assets
)
add_custom_command(
    TARGET ProceduralTerrain PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Assets $<TARGET_FILE_DIR:ProceduralTerrain>/CustomAssets
)