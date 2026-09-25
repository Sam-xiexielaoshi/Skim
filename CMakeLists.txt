cmake_minimum_required(VERSION 3.16)

project(Skim VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    AxContainer
    Pdf
)

qt_standard_project_setup()

set(QSS_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/DocumentExtractor.qss
)

set_source_files_properties(
    ${QSS_FILE}
    PROPERTIES
    QT_RESOURCE_ALIAS "DocumentExtractor.qss"
)

qt_add_executable(Skim
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DocumentExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DocumentExtractor.h

    ${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc
)

set(CHECKMARK_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/checkmark.svg
)

set_source_files_properties(
    ${CHECKMARK_FILE}
    PROPERTIES
    QT_RESOURCE_ALIAS "checkmark.svg"
)

set(APP_ICON_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/DocumentExtractor_256.png
)

set_source_files_properties(
    ${APP_ICON_FILE}
    PROPERTIES
    QT_RESOURCE_ALIAS "DocumentExtractor_256.png"
)

qt_add_resources(Skim "app_resources"
    PREFIX "/"
    FILES
        ${QSS_FILE}
        ${CHECKMARK_FILE}
        ${APP_ICON_FILE}
)

target_link_libraries(Skim PRIVATE
    Qt6::Widgets
    Qt6::AxContainer
    Qt6::Pdf
)

set_target_properties(Skim PROPERTIES
    WIN32_EXECUTABLE ON
)