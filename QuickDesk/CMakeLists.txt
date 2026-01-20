cmake_minimum_required(VERSION 3.16)

project(QuickDesk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt installation path (can be overridden via environment or command line)
if(DEFINED ENV{Qt6_DIR})
    set(CMAKE_PREFIX_PATH $ENV{Qt6_DIR})
elseif(EXISTS "C:/QtPro/6.8.4/msvc2022_64")
    set(CMAKE_PREFIX_PATH "C:/QtPro/6.8.4/msvc2022_64")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Core)

get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "is multi config:" ${GENERATOR_IS_MULTI_CONFIG})

if(GENERATOR_IS_MULTI_CONFIG)
    # 限制多配置类型
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "limit build type" FORCE)
    # 多配置类型在配置时有多个构建类型，不知道具体编译的构建类型，在编译时才知道
else()
    set(allowableBuildTypes Debug Release RelWithDebInfo)
    # default RelWithDebInfo
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
    elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBuildTypes)
        message(FATAL_ERROR "[${PROJECT_NAME}] Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
    message(STATUS "[${PROJECT_NAME}] single config:BUILD_TYPE:${CMAKE_BUILD_TYPE}")
endif()

# Compiler set
message(STATUS "[${PROJECT_NAME}] C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID}")
if (MSVC)
    # FFmpeg cannot be compiled natively by MSVC version < 12.0 (2013)
    if(MSVC_VERSION LESS 1800)
        message(FATAL_ERROR "[${PROJECT_NAME}] ERROR: MSVC version is older than 12.0 (2013).")
    endif()

    message(STATUS "[${PROJECT_NAME}] Set Warnings as error")
    # warning level 3 and all warnings as errors
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/W3> $<$<COMPILE_LANGUAGE:CXX>:/WX> /wd4566 /wd4996)

    # avoid warning C4819
    add_compile_options(/utf-8)

    # ensure we use minimal "windows.h" lib without the crazy min max macros
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

    # disable SAFESEH - avoid "LNK2026: module unsafe"(Qt5.15&&vs2019)
    add_link_options(/SAFESEH:NO)
endif()

if (NOT MSVC)
    message(STATUS "[${PROJECT_NAME}] Set warnings as error")
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)

if(UNIX)
    add_compile_options(-Wno-error=pedantic)
    add_compile_options(-Wno-error=implicit-fallthrough)
endif()

    # disable some warning
    add_compile_options(-Wno-nested-anon-types -Wno-c++17-extensions -Wno-overloaded-virtual)
endif()

# QD define

# check arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QD_CPU_ARCH x64)
else()
    set(QD_CPU_ARCH x86)
endif()
message(STATUS "[${PROJECT_NAME}] CPU_ARCH:${QD_CPU_ARCH}")

set(QD_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../output/${QD_CPU_ARCH}/${CMAKE_BUILD_TYPE}")
message(STATUS "[${PROJECT_NAME}] QD_OUTPUT_DIRECTORY:${QD_OUTPUT_DIRECTORY}")

# Source files
set(MANAGER_SOURCES
    src/manager/NativeMessaging.cpp
    src/manager/ProcessManager.cpp
    src/manager/ServerManager.cpp
    src/manager/HostManager.cpp
    src/manager/ClientManager.cpp
)

set(MANAGER_HEADERS
    src/manager/NativeMessaging.h
    src/manager/ProcessManager.h
    src/manager/ServerManager.h
    src/manager/HostManager.h
    src/manager/ClientManager.h
)

set(CONTROLLER_SOURCES
    src/controller/MainController.cpp
)

set(CONTROLLER_HEADERS
    src/controller/MainController.h
)

set(ALL_SOURCES
    main.cpp
    ${MANAGER_SOURCES}
    ${CONTROLLER_SOURCES}
)

set(ALL_HEADERS
    ${MANAGER_HEADERS}
    ${CONTROLLER_HEADERS}
)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(QuickDesk
    ${ALL_SOURCES}
    ${ALL_HEADERS}
)

qt_add_qml_module(QuickDesk
    URI QuickDesk
    VERSION 1.0
    QML_FILES 
        Main.qml
)

# Include directories
target_include_directories(QuickDesk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(QuickDesk PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(QuickDesk
    PRIVATE 
    Qt6::Quick 
    Qt6::Core
)

include(GNUInstallDirs)
install(TARGETS QuickDesk
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Output dir

function (_get_all_cmake_targets out_var current_dir)
    get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

    foreach(subdir ${subdirs})
        _get_all_cmake_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()

_get_all_cmake_targets(all_targets ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "[${PROJECT_NAME}] all targets: ${all_targets}")

# $<0:> 使用生成器表达式为每个config设置RUNTIME_OUTPUT_DIRECTORY，这样multi config就不会自动追加CMAKE_BUILD_TYPE子目录了
# 1. multi config介绍 https://cmake.org/cmake/help/latest/prop_gbl/GENERATOR_IS_MULTI_CONFIG.html
# 2. multi config在不用表达式生成器时自动追加子目录说明 https://cmake.org/cmake/help/latest/prop_tgt/RUNTIME_OUTPUT_DIRECTORY.html
# 3. 使用表达式生成器禁止multi config自动追加子目录解决方案 https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
# 一般多配置时CMAKE_BUILD_TYPE默认为空，不应该使用此变量，当前项目打包脚本执行CMake时会指定CMAKE_BUILD_TYPE变量
foreach(target ${all_targets})
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${QD_OUTPUT_DIRECTORY}/$<0:>"
        LIBRARY_OUTPUT_DIRECTORY "${QD_OUTPUT_DIRECTORY}/$<0:>"
        ARCHIVE_OUTPUT_DIRECTORY "${QD_OUTPUT_DIRECTORY}/$<0:>")
endforeach()
