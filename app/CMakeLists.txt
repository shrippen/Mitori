cmake_minimum_required(VERSION 3.21)
project(mitori VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 LinguistTools)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(KF6 REQUIRED COMPONENTS Kirigami)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

qt_standard_project_setup(REQUIRES 6.8)
# Module output in <build>/qml: scripts/qmllint.sh finds the Mitori module there.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

# Programme data: tools/daten.py checks daten/*.yaml and writes mitori-daten.json into the build.
set(MITORI_REPO ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(MITORI_DATEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/daten)
set(MITORI_DATEN_JSON ${MITORI_DATEN_DIR}/mitori-daten.json)
file(GLOB MITORI_DATEN_YAML CONFIGURE_DEPENDS ${MITORI_REPO}/daten/*.yaml)
add_custom_command(
    OUTPUT ${MITORI_DATEN_JSON}
    COMMAND Python3::Interpreter ${MITORI_REPO}/tools/daten.py --ausgabe ${MITORI_DATEN_DIR}
    DEPENDS ${MITORI_REPO}/tools/daten.py ${MITORI_REPO}/KORREKTUREN.md ${MITORI_DATEN_YAML}
    COMMENT "Checking programme data (tools/daten.py)"
    VERBATIM
)

qt_add_executable(mitori main.cpp)

qt_add_qml_module(mitori
    URI Mitori
    QML_FILES
        qml/Main.qml
        qml/StartPage.qml
    SOURCES
        catalog/catalog.h
        catalog/catalog.cpp
        services/programme.h
        services/programme.cpp
)

# Texts: English in the code, German in i18n/mitori_de.ts (update: cmake --build <dir> --target
# update_translations). The .qm files go into the resources under :/i18n.
qt_add_translations(mitori
    TS_FILES i18n/mitori_de.ts
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete
)

qt_add_resources(mitori "daten"
    PREFIX "/daten"
    BASE ${MITORI_DATEN_DIR}
    FILES ${MITORI_DATEN_JSON}
)

# Kante (scripts/sync-kante.sh) as the QML module "Kante" in the resources.
file(GLOB_RECURSE KANTE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS qml/Kante/*)
qt_add_resources(mitori "kante"
    PREFIX "/qt/qml/Kante"
    BASE qml/Kante
    FILES ${KANTE_FILES}
)

# The generated type registration includes the headers by name.
target_include_directories(mitori PRIVATE catalog services)
target_link_libraries(mitori PRIVATE Qt6::Quick Qt6::QuickControls2 KF6::Kirigami)
# The QML import scan links static plugins; a shared Qt needs none, and Kirigami's plugin
# targets are not exported (one warning per plugin). Android keeps it for androiddeployqt.
if(NOT ANDROID)
    set_target_properties(mitori PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
endif()

# Internal demo (Studio Weber data, screenshots only). Never in published builds.
option(MITORI_DEMO "Include the internal demo mode (development only)" OFF)
if(MITORI_DEMO)
    target_compile_definitions(mitori PRIVATE MITORI_DEMO)
endif()

if(ANDROID)
    set_property(TARGET mitori APPEND PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_target_properties(mitori PROPERTIES
        QT_ANDROID_PACKAGE_NAME "com.github.shrippen.mitori"
        QT_ANDROID_VERSION_NAME "0.1.0"
        QT_ANDROID_VERSION_CODE "100"
        QT_ANDROID_MIN_SDK_VERSION "28"
        QT_ANDROID_TARGET_SDK_VERSION "34"
        QT_ANDROID_COMPILE_SDK_VERSION "36"
        QT_ANDROID_SDK_BUILD_TOOLS_REVISION "36.0.0"
    )
    # Kirigami's QML plugin needs libomp.so from the NDK.
    file(GLOB LIBOMP_CANDIDATES "${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/*/lib/linux/aarch64/libomp.so")
    if(LIBOMP_CANDIDATES)
        list(GET LIBOMP_CANDIDATES 0 LIBOMP_PATH)
        set_target_properties(mitori PROPERTIES QT_ANDROID_EXTRA_LIBS "${LIBOMP_PATH}")
    else()
        message(FATAL_ERROR "libomp.so not found in the NDK (${ANDROID_NDK}); Kirigami cannot load without it")
    endif()
    # Kirigami's QML plugins: <prefix>/lib/cmake/KF6Kirigami -> <prefix>/lib/qml.
    get_filename_component(KF6_QML_DIR "${KF6Kirigami_DIR}/../../qml" ABSOLUTE)
    if(IS_DIRECTORY "${KF6_QML_DIR}")
        set_property(TARGET mitori APPEND PROPERTY QT_QML_IMPORT_PATH "${KF6_QML_DIR}")
    endif()
endif()

option(MITORI_BUILD_TESTS "Build the C++ unit tests" OFF)
if(MITORI_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_catalog tests/tst_catalog.cpp catalog/catalog.cpp catalog/catalog.h)
    target_include_directories(tst_catalog PRIVATE catalog)
    target_compile_definitions(tst_catalog PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_catalog PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    add_dependencies(tst_catalog mitori)
    set_target_properties(tst_catalog PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME catalog COMMAND tst_catalog)
endif()

install(TARGETS mitori
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
