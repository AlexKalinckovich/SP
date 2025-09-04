cmake_minimum_required(VERSION 3.29)
project(SP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GUI subsystem (WIN32) — тогда приложение не будет открывать консоль.
add_executable(SP WIN32
        src/main.cpp
        src/Win32Window.cpp
        include/components/MenuBar.h
        src/components/MenuBar.cpp
        include/components/OverlayWindow.h
        src/components/OverlayWindow.cpp
        include/components/AboutDialog.h
        src/components/AboutDialog.cpp
        include/utils/IdleMonitor.h
        src/utils/IdleMonitor.cpp
        include/win32/MessageHandler.h
        include/win32/HashMapMessageHandler.h
        include/utils/ComponentManager.h
        src/utils/ComponentManager.cpp
)

# Подключаем include/ директорию (заголовки — там).
target_include_directories(SP PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Unicode APIs
target_compile_definitions(SP PRIVATE UNICODE _UNICODE)

target_link_libraries(SP PRIVATE
        user32
        gdi32
        comdlg32
)
