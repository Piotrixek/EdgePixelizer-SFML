cmake_minimum_required(VERSION 3.15)

# Project Name
project(EdgePixelConverter LANGUAGES CXX C)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Library Paths ---
set(SFML_DIR "C:/libs/SFML-3.0.0/lib/cmake/SFML" CACHE PATH "Path to SFML 3.0.0 cmake config")
set(OpenCV_DIR "C:/libs/opencv/build" CACHE PATH "Path to OpenCV build directory")
set(IMGUI_DIR "C:/libs/imgui" CACHE PATH "Path to ImGui source directory")
set(IMGUI_SFML_DIR "C:/libs/imgui-sfml" CACHE PATH "Path to ImGui-SFML source directory")
set(TINYFILEDIALOGS_DIR "C:/libs/tinyfiledialogs" CACHE PATH "Path to tinyfiledialogs source directory")


# --- Find Packages ---
find_package(SFML 3.0 COMPONENTS System Window Graphics Network Audio REQUIRED)
if(NOT SFML_FOUND)
    message(FATAL_ERROR "SFML not found! Set SFML_DIR correctly.")
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found! Set OpenCV_DIR correctly.")
endif()

find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL not found! Ensure graphics drivers are installed.")
endif()

# --- ImGui Source Files ---
add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_sources(imgui_lib PRIVATE
    ${IMGUI_SFML_DIR}/imgui-SFML.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC
    SFML::System
    SFML::Window
    SFML::Graphics
)

# --- tinyfiledialogs Library ---
add_library(tinyfd_lib STATIC
    "${TINYFILEDIALOGS_DIR}/tinyfiledialogs.c"
)
target_include_directories(tinyfd_lib PUBLIC
    ${TINYFILEDIALOGS_DIR}
)


# --- Define Executable ---
add_executable(${PROJECT_NAME}
    main.cpp
)

# --- Link Executable ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Network
    SFML::Audio
    ${OpenCV_LIBS}
    imgui_lib
    OpenGL::GL
    tinyfd_lib
)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE comdlg32 ole32 shell32)
endif()


# --- Include Directories for Main Project ---
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TINYFILEDIALOGS_DIR}
)

# --- DLL COPYING VIA CMAKE ---
if(WIN32)
    message(STATUS "Adding Post-Build DLL copy commands for Windows")
    set(DEST_DIR $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    # --- SFML DLLs ---
    set(SFML_BIN_DIR "C:/libs/SFML-3.0.0/bin")
    set(SFML_SYSTEM_DLL_BASE "sfml-system")
    set(SFML_WINDOW_DLL_BASE "sfml-window")
    set(SFML_GRAPHICS_DLL_BASE "sfml-graphics")
    set(SFML_NETWORK_DLL_BASE "sfml-network")
    set(SFML_AUDIO_DLL_BASE "sfml-audio")
    set(SFML_VERSION_SUFFIX "-3")
    set(SFML_DEBUG_SUFFIX "-d")
    set(SFML_OPENAL_DLL "${SFML_BIN_DIR}/openal32.dll")

    # Loop through SFML components and add copy commands
    foreach(COMPONENT ${SFML_SYSTEM_DLL_BASE} ${SFML_WINDOW_DLL_BASE} ${SFML_GRAPHICS_DLL_BASE} ${SFML_NETWORK_DLL_BASE} ${SFML_AUDIO_DLL_BASE})
        set(DEBUG_DLL_PATH "${SFML_BIN_DIR}/${COMPONENT}${SFML_DEBUG_SUFFIX}${SFML_VERSION_SUFFIX}.dll")
        set(RELEASE_DLL_PATH "${SFML_BIN_DIR}/${COMPONENT}${SFML_VERSION_SUFFIX}.dll")

        # Check if DLLs exist before adding command
        if(EXISTS "${DEBUG_DLL_PATH}" AND EXISTS "${RELEASE_DLL_PATH}")
             add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SFML_BIN_DIR}/${COMPONENT}$<$<CONFIG:Debug>:${SFML_DEBUG_SUFFIX}>${SFML_VERSION_SUFFIX}.dll"
                    "${DEST_DIR}"
                COMMENT "Copying ${COMPONENT} DLL for $<CONFIG>"
                VERBATIM
            )
        else()
            message(WARNING "SFML DLL not found for component ${COMPONENT}. Skipping copy command. Searched:\n  ${DEBUG_DLL_PATH}\n  ${RELEASE_DLL_PATH}")
        endif()
    endforeach()

    # Copy OpenAL separately checking existence first
    if(EXISTS "${SFML_OPENAL_DLL}")
         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SFML_OPENAL_DLL}"
                "${DEST_DIR}"
            COMMENT "Copying OpenAL DLL"
            VERBATIM
        )
    else()
         message(WARNING "OpenAL DLL not found at ${SFML_OPENAL_DLL}. Skipping copy command.")
    endif()


    # --- OpenCV DLLs ---
    set(OPENCV_BIN_DIR "C:/libs/opencv/build/x64/vc16/bin") # Using vc16 as fixed
    set(OPENCV_DLL_BASE "opencv_world4110")
    set(OPENCV_DEBUG_SUFFIX "d")
    set(OPENCV_DEBUG_DLL "${OPENCV_BIN_DIR}/${OPENCV_DLL_BASE}${OPENCV_DEBUG_SUFFIX}.dll")
    set(OPENCV_RELEASE_DLL "${OPENCV_BIN_DIR}/${OPENCV_DLL_BASE}.dll")

    # Check if DLLs exist before adding command
    if(EXISTS "${OPENCV_DEBUG_DLL}" AND EXISTS "${OPENCV_RELEASE_DLL}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENCV_BIN_DIR}/${OPENCV_DLL_BASE}$<$<CONFIG:Debug>:${OPENCV_DEBUG_SUFFIX}>.dll"
                "${DEST_DIR}"
            COMMENT "Copying OpenCV DLL for $<CONFIG>"
            VERBATIM
        )
    else()
        message(WARNING "OpenCV world DLL not found. Skipping copy command. Searched:\n  ${OPENCV_DEBUG_DLL}\n  ${OPENCV_RELEASE_DLL}")
    endif()

endif()
# --- ***** END REVISED DLL COPYING ***** ---


message(STATUS "SFML include dir: ${SFML_INCLUDE_DIR}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV includes: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenGL library: ${OPENGL_gl_LIBRARY}")

