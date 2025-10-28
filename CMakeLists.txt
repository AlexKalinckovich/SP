cmake_minimum_required(VERSION 3.29)
project(SP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(SP WIN32
        C:/Users/brota/CLionProjects/ProcessLib/processInfo.h
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
        include/utils/FileManager.h
        include/utils/EncodingDetector.h
        src/utils/EncodingDetector.cpp
        src/utils/FileManager.cpp
        include/utils/ErrorFormater.h
        src/utils/ErrorFormater.cpp
        include/components/FontSelectorComponent.h
        src/components/FontSelectorComponent.cpp
        include/components/excelView/ExcelLikeView.h
        src/components/excelView/ExcelLikeView.cpp
        src/components/excelView/ExcelLikeView_MessageHandlers.cpp
        src/components/excelView/ExcelLikeView_CellOperations.cpp
        include/meta_info/message_codes.h
)

target_include_directories(SP PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(SP PRIVATE UNICODE _UNICODE)

target_link_libraries(SP PRIVATE
        user32
        gdi32
        comdlg32
)
