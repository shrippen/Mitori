cmake_minimum_required(VERSION 3.21)
project(mitori VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia Sql LinguistTools)
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

# The app's QML module, services and resources as a static library: the app and the UI test
# (tests/tst_ui.cpp) link the same.
qt_add_library(mitori_app STATIC)
qt_add_qml_module(mitori_app
    URI Mitori
    QML_FILES
        qml/Main.qml
        qml/StartPage.qml
        qml/ProgrammePage.qml
        qml/TechniqueSheet.qml
        qml/CommentPage.qml
        qml/PlayerPage.qml
        qml/SettingsPage.qml
        qml/RatingRow.qml
        qml/GradeStand.qml
        qml/TodayPage.qml
        qml/NotesPage.qml
        qml/NoteEditor.qml
        qml/NoteRow.qml
        qml/AllNotesPage.qml
        qml/SearchPage.qml
    SOURCES
        catalog/catalog.h
        catalog/catalog.cpp
        services/programme.h
        services/programme.cpp
        services/videos.h
        services/videos.cpp
        services/practice.h
        services/practice.cpp
        store/practicelog.h
        store/practicelog.cpp
        store/schedule.h
        store/schedule.cpp
        store/searchkey.h
)

# Texts: English in the code, German in i18n/mitori_de.ts (update: cmake --build <dir> --target
# update_translations). The .qm files go into the resources under :/i18n. mitori_en.ts holds only
# the English plural forms; it also makes English win where it comes before German in the
# user's languages.
qt_add_translations(mitori_app
    TS_FILES i18n/mitori_de.ts
    PLURALS_TS_FILE i18n/mitori_en.ts
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete
)

qt_add_resources(mitori_app "daten"
    PREFIX "/daten"
    BASE ${MITORI_DATEN_DIR}
    FILES ${MITORI_DATEN_JSON}
)

# Icons for platforms without an icon theme (scripts/sync-icons.sh).
file(GLOB_RECURSE ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS icons/*)
qt_add_resources(mitori_app "icons"
    PREFIX "/icons"
    BASE icons
    FILES ${ICON_FILES}
)

# Kante (scripts/sync-kante.sh) as the QML module "Kante" in the resources.
file(GLOB_RECURSE KANTE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS qml/Kante/*)
qt_add_resources(mitori_app "kante"
    PREFIX "/qt/qml/Kante"
    BASE qml/Kante
    FILES ${KANTE_FILES}
)

# The generated type registration includes the headers by name.
target_include_directories(mitori_app PUBLIC catalog services store)
target_link_libraries(mitori_app PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Sql KF6::Kirigami)

qt_add_executable(mitori main.cpp)
target_link_libraries(mitori PRIVATE mitori_app mitori_appplugin)
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

    qt_add_executable(tst_programme tests/tst_programme.cpp services/programme.cpp services/programme.h
        catalog/catalog.cpp catalog/catalog.h)
    target_include_directories(tst_programme PRIVATE catalog services store)
    target_compile_definitions(tst_programme PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_programme PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    set_target_properties(tst_programme PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_programme mitori)
    add_test(NAME programme COMMAND tst_programme)

    qt_add_executable(tst_practice tests/tst_practice.cpp services/practice.cpp services/practice.h
        store/practicelog.cpp store/practicelog.h store/schedule.cpp store/schedule.h)
    target_include_directories(tst_practice PRIVATE services store)
    target_link_libraries(tst_practice PRIVATE Qt6::Core Qt6::Qml Qt6::Sql Qt6::Test)
    set_target_properties(tst_practice PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME practice COMMAND tst_practice)

    qt_add_executable(tst_schedule tests/tst_schedule.cpp store/schedule.cpp store/schedule.h store/practicelog.h)
    target_include_directories(tst_schedule PRIVATE store)
    target_link_libraries(tst_schedule PRIVATE Qt6::Core Qt6::Test)
    set_target_properties(tst_schedule PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME schedule COMMAND tst_schedule)

    # The pages with Kirigami and Kante, offscreen; screenshots into <build>/ui-shots. Plays a video
    # only with MITORI_VIDEOS (the videos are not in the repository).
    find_package(Qt6 REQUIRED COMPONENTS QuickTest)
    qt_add_executable(tst_ui tests/tst_ui.cpp)
    target_link_libraries(tst_ui PRIVATE mitori_app mitori_appplugin Qt6::QuickTest)
    target_compile_definitions(tst_ui PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
        UI_SHOTS_DIR="${CMAKE_BINARY_DIR}/ui-shots")
    set_target_properties(tst_ui PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME ui COMMAND tst_ui)
    set_tests_properties(ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Material")
endif()

install(TARGETS mitori
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
