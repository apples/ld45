cmake_minimum_required(VERSION 3.12)
project(GMTKJAM2019)

add_subdirectory(ext/ginseng)
add_subdirectory(ext/lua)
add_subdirectory(ext/sol2)
add_subdirectory(ext/soloud)
add_subdirectory(ext/metastuff)
add_subdirectory(ext/glm)
add_subdirectory(ext/lodepng)
add_subdirectory(ext/sushi)
add_subdirectory(ext/msdfgen)

if(EMSCRIPTEN)
    find_package(PythonInterp 2 REQUIRED)

    set(WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Web Output Directory")
    set(DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Source Directory")

    # Static

    file(GLOB_RECURSE STATIC_SOURCES CONFIGURE_DEPENDS static/*)
    add_custom_target(static_www
        COMMENT "Copying static files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static "${WWW_DIR}"
        SOURCES ${STATIC_SOURCES})

    # Data

    file(GLOB_RECURSE DATA_FILES CONFIGURE_DEPENDS ${DATA_DIR}/*)
    set(FILE_PACKAGER $ENV{EMSCRIPTEN}/tools/file_packager.py)
    set(DATA_FILE ${WWW_DIR}/game.data)
    set(DATA_LOADER ${WWW_DIR}/game.data.js)
    add_custom_command(
        OUTPUT ${DATA_FILE} ${DATA_LOADER}
        COMMAND ${PYTHON_EXECUTABLE} ${FILE_PACKAGER} ${DATA_FILE} --preload ${DATA_DIR}@data --js-output=${DATA_LOADER}
        COMMENT "Packaging data files"
        DEPENDS ${DATA_FILES})
    add_custom_target(data_files
        SOURCES ${DATA_FILES}
        DEPENDS ${DATA_FILE} ${DATA_LOADER})


    # Emscripten Ports

    string(CONCAT EMSCRIPTEN_PORTS_FLAGS
        " -s USE_SDL=2"
        " -s USE_SDL_IMAGE=2"
        " -s USE_SDL_NET=2"
        " -s USE_FREETYPE=1")

    # JS

    file(GLOB_RECURSE GAME_JS_SOURCES "src/*.js")

    # C++

    file(GLOB_RECURSE GAME_CPP_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
    add_executable(game ${GAME_CPP_SOURCES} ${GAME_JS_SOURCES})
    string(CONCAT GAME_LINK_FLAGS
        " -s TOTAL_MEMORY=134217728"
        " -s DISABLE_EXCEPTION_CATCHING=0"
        " -s FORCE_FILESYSTEM=1"
        " -s EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
        "${EMSCRIPTEN_PORTS_FLAGS}")
    string(CONCAT GAME_LINK_FLAGS_DEBUG
        " -g4"
        " --cpuprofiler"
        " --memoryprofiler"
        " -s ASSERTIONS=1"
        " -s DEMANGLE_SUPPORT=1")
    set_target_properties(game PROPERTIES
        SUFFIX .js
        LINK_FLAGS "${GAME_LINK_FLAGS}"
        LINK_FLAGS_DEBUG "${GAME_LINK_FLAGS_DEBUG}"
        RUNTIME_OUTPUT_DIRECTORY "${WWW_DIR}")
    target_compile_definitions(game PUBLIC $<$<CONFIG:Debug>:SOL_CHECK_ARGUMENTS>)
    target_compile_options(game PRIVATE
        "-std=c++1z"
        "SHELL:${EMSCRIPTEN_PORTS_FLAGS}")
    em_link_js_library(game ${GAME_JS_SOURCES})
    target_link_libraries(game
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud)
    add_dependencies(game static_www data_files)
else()
    set(DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Native Output Directory")
    set(DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client Data Source Directory")

    find_package(SDL2 REQUIRED)
    find_package(Freetype REQUIRED)

    # Data

    file(GLOB_RECURSE DATA_FILES CONFIGURE_DEPENDS ${DATA_DIR}/*)
    add_custom_command(
        OUTPUT ${DIST_DIR}/data
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_DIR} ${DIST_DIR}
        COMMENT "Copying data files"
        DEPENDS ${DATA_FILES})
    add_custom_target(data_files
        SOURCES ${DATA_FILES}
        DEPENDS ${DATA_DIR})

    # C++

    file(GLOB_RECURSE GAME_CPP_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
    add_executable(game ${GAME_CPP_SOURCES} ${GAME_JS_SOURCES})
    set_target_properties(game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${DIST_DIR}")
    target_compile_definitions(game PUBLIC $<$<CONFIG:Debug>:SOL_CHECK_ARGUMENTS>)
    target_compile_options(game PRIVATE
        "-std=c++1z")
    target_link_libraries(game
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud
        Freetype::Freetype
        SDL2_image
        SDL2
        Ws2_32)
    add_dependencies(game data_files)
endif()

