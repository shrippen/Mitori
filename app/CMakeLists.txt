cmake_minimum_required(VERSION 3.21)
project(mitori VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia Sql Network Concurrent LinguistTools)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package(KF6 REQUIRED COMPONENTS Kirigami)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# Pronunciation in the glossary; without the module the speaker buttons stay hidden.
find_package(Qt6 6.8 QUIET COMPONENTS TextToSpeech)
# The screen's brightness over Solid (platform/devicelevels) on Linux.
if(NOT ANDROID)
    find_package(Qt6 6.8 REQUIRED COMPONENTS DBus)
endif()

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

# Shinki (shinki/README.md) offered in the style choice: only to check it, until every page
# reaches it through app/qml/Look. Off in every published build.
option(MITORI_SHINKI "Offer the Shinki style (development only, until the pages use Look)" OFF)
option(MITORI_BUILD_TESTS "Build the C++ unit tests" OFF)

# The app's QML module, services and resources as a static library: the app and the UI test
# (tests/tst_ui.cpp) link the same.
# The version for "About Mitori" (services/appearance) and the application.
add_compile_definitions(MITORI_VERSION="${PROJECT_VERSION}")

qt_add_library(mitori_app STATIC)
set_source_files_properties(qml/Look/Look.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(mitori_app
    URI Mitori
    QML_FILES
        qml/Main.qml
        qml/StartPage.qml
        qml/ProgrammePage.qml
        qml/TechniqueSheet.qml
        qml/StepList.qml
        qml/CommentPage.qml
        qml/PlayerPage.qml
        qml/PlayerView.qml
        qml/SettingsPage.qml
        qml/RatingRow.qml
        qml/GradeStand.qml
        qml/TodayPage.qml
        qml/NotesPage.qml
        qml/NoteEditor.qml
        qml/NoteRow.qml
        qml/AllNotesPage.qml
        qml/SearchPage.qml
        qml/StatsPage.qml
        qml/ExamPage.qml
        qml/ExamRoundPage.qml
        qml/ExamBoard.qml
        qml/ExamEndPage.qml
        qml/TechniqueMapPage.qml
        qml/CoursesPage.qml
        qml/CourseRow.qml
        qml/CoursePage.qml
        qml/CourseNewPage.qml
        qml/coursedays.js
        qml/comment.js
        qml/VideosPage.qml
        qml/GradesPage.qml
        qml/grades.js
        qml/MediaView.qml
        qml/MediaCompose.qml
        qml/CameraPage.qml
        qml/ConflictsPage.qml
        qml/YearsPage.qml
        qml/StyleChoice.qml
        qml/AppearancePage.qml
        qml/WelcomePage.qml
        qml/Look/Look.qml
        qml/Look/LookTile.qml
        qml/Look/LookHint.qml
        qml/Look/LookButton.qml
        qml/Look/LookScope.qml
    SOURCES
        catalog/catalog.h
        catalog/catalog.cpp
        services/programme.h
        services/programme.cpp
        services/glossary.h
        services/glossary.cpp
        player/speaker.h
        player/speaker.cpp
        services/videos.h
        services/videos.cpp
        services/practice.h
        services/practice.cpp
        services/exam.h
        services/exam.cpp
        store/practicelog.h
        store/practicelog.cpp
        store/schedule.h
        store/schedule.cpp
        store/searchkey.h
        store/icsreader.h
        store/icsreader.cpp
        store/coursebook.h
        store/coursebook.cpp
        store/device.h
        services/courses.h
        services/courses.cpp
        platform/devicecalendar.h
        platform/devicecalendar.cpp
        platform/devicelevels.h
        platform/devicelevels.cpp
        services/levels.h
        services/levels.cpp
        services/appearance.h
        services/appearance.cpp
        store/bundlecrypto.h
        store/bundlecrypto.cpp
        store/bundleshelf.h
        store/bundleshelf.cpp
        services/bundles.h
        services/bundles.cpp
        services/keyedlistmodel.h
        services/keyedlistmodel.cpp
        services/allnotes.h
        services/allnotes.cpp
        store/mediastore.h
        store/mediastore.cpp
        store/zipwriter.h
        store/zipwriter.cpp
        store/notebookexport.h
        store/notebookexport.cpp
        store/zipreader.h
        store/zipreader.cpp
        store/tablemerge.h
        store/tablemerge.cpp
        store/backuparchive.h
        store/backuparchive.cpp
        services/backup.h
        services/backup.cpp
        store/webdav.h
        store/webdav.cpp
        store/syncaccount.h
        store/syncaccount.cpp
        services/syncjob.h
        services/syncjob.cpp
        services/sync.h
        services/sync.cpp
        services/media.h
        services/media.cpp
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

# Shinki (shinki/, the club newspaper's style) as the QML module "Shinki" with its fonts, and the
# Look layer's Shinki files (app/qml/Look/+shinki, chosen by the file selector "shinki" at start,
# Appearance). Only where Shinki can be chosen or is tested: its fonts are 1.3 MB.
if(MITORI_SHINKI OR MITORI_BUILD_TESTS)
    file(GLOB SHINKI_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ../shinki/qml/Shinki/*)
    qt_add_resources(mitori_app "shinki"
        PREFIX "/qt/qml/Shinki"
        BASE ../shinki/qml/Shinki
        FILES ${SHINKI_FILES}
    )
    file(GLOB SHINKI_FONTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ../shinki/fonts/*.ttf)
    qt_add_resources(mitori_app "shinki_fonts"
        PREFIX "/shinki/fonts"
        BASE ../shinki/fonts
        FILES ${SHINKI_FONTS}
    )
    file(GLOB LOOK_SHINKI RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS qml/Look/+shinki/*.qml)
    qt_add_resources(mitori_app "look_shinki"
        PREFIX "/qt/qml/Mitori"
        FILES ${LOOK_SHINKI}
    )
    target_compile_definitions(mitori_app PRIVATE MITORI_SHINKI_FILES)
endif()

# Monocypher (app/third_party, BSD-2-Clause / CC0): Argon2id, BLAKE2b and XChaCha20-Poly1305 for the
# video bundles (store/bundlecrypto).
add_library(monocypher STATIC third_party/monocypher/monocypher.c third_party/monocypher/monocypher.h)
target_include_directories(monocypher PUBLIC third_party/monocypher)
set_target_properties(monocypher PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The generated type registration includes the headers by name.
target_include_directories(mitori_app PUBLIC catalog services store player platform)
target_link_libraries(mitori_app PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Sql Qt6::Network Qt6::Concurrent KF6::Kirigami monocypher)
if(NOT ANDROID)
    target_link_libraries(mitori_app PUBLIC Qt6::DBus)
endif()
if(TARGET Qt6::TextToSpeech)
    target_link_libraries(mitori_app PUBLIC Qt6::TextToSpeech)
    target_compile_definitions(mitori_app PRIVATE MITORI_SPEECH)
else()
    message(STATUS "Qt6 TextToSpeech not found: the glossary has no pronunciation")
endif()

qt_add_executable(mitori main.cpp)
target_link_libraries(mitori PRIVATE mitori_app mitori_appplugin)
target_include_directories(mitori PRIVATE services)
# The QML import scan links static plugins; a shared Qt needs none, and Kirigami's plugin
# targets are not exported (one warning per plugin). Android keeps it for androiddeployqt.
if(NOT ANDROID)
    set_target_properties(mitori PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
endif()

if(MITORI_SHINKI)
    target_compile_definitions(mitori_app PRIVATE MITORI_SHINKI)
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
    target_compile_definitions(tst_programme PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}"
        TEST_DATA_DIR="${CMAKE_BINARY_DIR}/test-data/programme")
    target_link_libraries(tst_programme PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    set_target_properties(tst_programme PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_programme mitori)
    add_test(NAME programme COMMAND tst_programme)

    qt_add_executable(tst_exam tests/tst_exam.cpp services/exam.cpp services/exam.h
        services/programme.cpp services/programme.h catalog/catalog.cpp catalog/catalog.h)
    target_include_directories(tst_exam PRIVATE catalog services store)
    target_compile_definitions(tst_exam PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_exam PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    set_target_properties(tst_exam PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_exam mitori)
    add_test(NAME exam COMMAND tst_exam)

    qt_add_executable(tst_glossary tests/tst_glossary.cpp services/glossary.cpp services/glossary.h
        player/speaker.cpp player/speaker.h catalog/catalog.cpp catalog/catalog.h)
    target_include_directories(tst_glossary PRIVATE catalog services store player)
    target_compile_definitions(tst_glossary PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_glossary PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    if(TARGET Qt6::TextToSpeech)
        target_link_libraries(tst_glossary PRIVATE Qt6::TextToSpeech)
        target_compile_definitions(tst_glossary PRIVATE MITORI_SPEECH)
    endif()
    set_target_properties(tst_glossary PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_glossary mitori)
    add_test(NAME glossary COMMAND tst_glossary)

    qt_add_executable(tst_practice tests/tst_practice.cpp services/practice.cpp services/practice.h
        store/practicelog.cpp store/practicelog.h store/tablemerge.cpp store/tablemerge.h store/schedule.cpp store/schedule.h store/mediastore.cpp store/mediastore.h)
    target_include_directories(tst_practice PRIVATE services store)
    target_compile_definitions(tst_practice PRIVATE TEST_DATA_DIR="${CMAKE_BINARY_DIR}/test-data/practice")
    target_link_libraries(tst_practice PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
    set_target_properties(tst_practice PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME practice COMMAND tst_practice)

    qt_add_executable(tst_schedule tests/tst_schedule.cpp store/schedule.cpp store/schedule.h store/practicelog.h)
    target_include_directories(tst_schedule PRIVATE store)
    target_link_libraries(tst_schedule PRIVATE Qt6::Core Qt6::Test)
    set_target_properties(tst_schedule PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME schedule COMMAND tst_schedule)

    qt_add_executable(tst_allnotes tests/tst_allnotes.cpp services/allnotes.cpp services/allnotes.h
        services/keyedlistmodel.cpp services/keyedlistmodel.h services/practice.cpp services/practice.h
        services/courses.cpp services/courses.h services/programme.cpp services/programme.h
        store/practicelog.cpp store/practicelog.h store/tablemerge.cpp store/tablemerge.h store/schedule.cpp store/schedule.h
        store/coursebook.cpp store/coursebook.h store/icsreader.cpp store/icsreader.h store/device.h
        platform/devicecalendar.cpp platform/devicecalendar.h catalog/catalog.cpp catalog/catalog.h store/mediastore.cpp store/mediastore.h store/zipwriter.cpp store/zipwriter.h store/notebookexport.cpp store/notebookexport.h)
    target_include_directories(tst_allnotes PRIVATE catalog services store platform)
    target_compile_definitions(tst_allnotes PRIVATE MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_allnotes PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
    set_target_properties(tst_allnotes PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_allnotes mitori)
    add_test(NAME allnotes COMMAND tst_allnotes)

    qt_add_executable(tst_media tests/tst_media.cpp store/mediastore.cpp store/mediastore.h
        services/practice.cpp services/practice.h store/practicelog.cpp store/practicelog.h store/tablemerge.cpp store/tablemerge.h
        store/schedule.cpp store/schedule.h services/courses.cpp services/courses.h
        store/coursebook.cpp store/coursebook.h store/icsreader.cpp store/icsreader.h store/device.h
        platform/devicecalendar.cpp platform/devicecalendar.h catalog/catalog.cpp catalog/catalog.h store/zipwriter.cpp store/zipwriter.h store/notebookexport.cpp store/notebookexport.h)
    target_include_directories(tst_media PRIVATE catalog services store platform)
    target_link_libraries(tst_media PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
    set_target_properties(tst_media PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME media COMMAND tst_media)

    qt_add_executable(tst_backup tests/tst_backup.cpp services/backup.cpp services/backup.h
        store/backuparchive.cpp store/backuparchive.h store/zipreader.cpp store/zipreader.h
        store/zipwriter.cpp store/zipwriter.h store/mediastore.cpp store/mediastore.h
        services/practice.cpp services/practice.h store/practicelog.cpp store/practicelog.h store/tablemerge.cpp store/tablemerge.h
        store/schedule.cpp store/schedule.h services/courses.cpp services/courses.h
        store/coursebook.cpp store/coursebook.h store/icsreader.cpp store/icsreader.h store/device.h
        platform/devicecalendar.cpp platform/devicecalendar.h catalog/catalog.cpp catalog/catalog.h store/notebookexport.cpp store/notebookexport.h)
    target_include_directories(tst_backup PRIVATE catalog services store platform)
    target_link_libraries(tst_backup PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
    set_target_properties(tst_backup PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME backup COMMAND tst_backup)

    qt_add_executable(tst_sync tests/tst_sync.cpp services/sync.cpp services/sync.h services/syncjob.cpp services/syncjob.h
        store/syncaccount.cpp store/syncaccount.h store/webdav.cpp store/webdav.h
        store/backuparchive.cpp store/backuparchive.h store/zipreader.cpp store/zipreader.h
        store/zipwriter.cpp store/zipwriter.h store/mediastore.cpp store/mediastore.h
        services/practice.cpp services/practice.h store/practicelog.cpp store/practicelog.h store/tablemerge.cpp store/tablemerge.h
        store/schedule.cpp store/schedule.h services/courses.cpp services/courses.h
        store/coursebook.cpp store/coursebook.h store/icsreader.cpp store/icsreader.h store/device.h
        platform/devicecalendar.cpp platform/devicecalendar.h catalog/catalog.cpp catalog/catalog.h store/notebookexport.cpp store/notebookexport.h)
    target_include_directories(tst_sync PRIVATE catalog services store platform)
    target_link_libraries(tst_sync PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Concurrent Qt6::Test)
    set_target_properties(tst_sync PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME sync COMMAND tst_sync)

    qt_add_executable(tst_export tests/tst_export.cpp store/zipwriter.cpp store/zipwriter.h
        store/notebookexport.cpp store/notebookexport.h store/mediastore.cpp store/mediastore.h
        services/courses.cpp services/courses.h store/coursebook.cpp store/coursebook.h store/tablemerge.cpp store/tablemerge.h
        store/icsreader.cpp store/icsreader.h store/device.h platform/devicecalendar.cpp platform/devicecalendar.h
        catalog/catalog.cpp catalog/catalog.h)
    target_include_directories(tst_export PRIVATE catalog services store platform)
    target_link_libraries(tst_export PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
    set_target_properties(tst_export PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME export COMMAND tst_export)

    qt_add_executable(tst_keyedlistmodel tests/tst_keyedlistmodel.cpp services/keyedlistmodel.cpp services/keyedlistmodel.h)
    target_include_directories(tst_keyedlistmodel PRIVATE services)
    target_link_libraries(tst_keyedlistmodel PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    set_target_properties(tst_keyedlistmodel PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME keyedlistmodel COMMAND tst_keyedlistmodel)

    qt_add_executable(tst_ics tests/tst_ics.cpp store/icsreader.cpp store/icsreader.h)
    target_include_directories(tst_ics PRIVATE store)
    target_compile_definitions(tst_ics PRIVATE TEST_ICS="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/lehrgaenge.ics")
    target_link_libraries(tst_ics PRIVATE Qt6::Core Qt6::Test)
    set_target_properties(tst_ics PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME ics COMMAND tst_ics)

    qt_add_executable(tst_courses tests/tst_courses.cpp services/courses.cpp services/courses.h
        store/coursebook.cpp store/coursebook.h store/tablemerge.cpp store/tablemerge.h store/icsreader.cpp store/icsreader.h store/device.h
        platform/devicecalendar.cpp platform/devicecalendar.h catalog/catalog.cpp catalog/catalog.h store/mediastore.cpp store/mediastore.h store/zipwriter.cpp store/zipwriter.h store/notebookexport.cpp store/notebookexport.h)
    target_include_directories(tst_courses PRIVATE catalog services store platform)
    target_compile_definitions(tst_courses PRIVATE TEST_ICS="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/lehrgaenge.ics"
        MITORI_DATEN_JSON="${MITORI_DATEN_JSON}")
    target_link_libraries(tst_courses PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
    set_target_properties(tst_courses PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_dependencies(tst_courses mitori)
    add_test(NAME courses COMMAND tst_courses)
    qt_add_executable(tst_bundlecrypto tests/tst_bundlecrypto.cpp store/bundlecrypto.cpp store/bundlecrypto.h)
    target_include_directories(tst_bundlecrypto PRIVATE store)
    target_compile_definitions(tst_bundlecrypto PRIVATE TEST_BUNDLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/bundle")
    target_link_libraries(tst_bundlecrypto PRIVATE Qt6::Core Qt6::Test monocypher)
    set_target_properties(tst_bundlecrypto PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME bundlecrypto COMMAND tst_bundlecrypto)

    qt_add_executable(tst_bundles tests/tst_bundles.cpp services/bundles.cpp services/bundles.h
        store/bundleshelf.cpp store/bundleshelf.h store/bundlecrypto.cpp store/bundlecrypto.h)
    target_include_directories(tst_bundles PRIVATE services store)
    target_compile_definitions(tst_bundles PRIVATE TEST_BUNDLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/bundle")
    target_link_libraries(tst_bundles PRIVATE Qt6::Core Qt6::Qml Qt6::Network Qt6::Concurrent Qt6::Test monocypher)
    set_target_properties(tst_bundles PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME bundles COMMAND tst_bundles)

    qt_add_executable(tst_levels tests/tst_levels.cpp services/levels.cpp services/levels.h
        platform/devicelevels.cpp platform/devicelevels.h)
    target_include_directories(tst_levels PRIVATE services platform)
    target_link_libraries(tst_levels PRIVATE Qt6::Core Qt6::Qml Qt6::DBus Qt6::Test)
    set_target_properties(tst_levels PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME levels COMMAND tst_levels)

    qt_add_executable(tst_appearance tests/tst_appearance.cpp services/appearance.cpp services/appearance.h)
    target_include_directories(tst_appearance PRIVATE services)
    target_link_libraries(tst_appearance PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
    set_target_properties(tst_appearance PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    if(MITORI_SHINKI)
        target_compile_definitions(tst_appearance PRIVATE MITORI_SHINKI)
    endif()
    add_test(NAME appearance COMMAND tst_appearance)

    # The pages with Kirigami and Kante, offscreen; screenshots into <build>/ui-shots. Plays a video
    # only with MITORI_VIDEOS (the videos are not in the repository).
    find_package(Qt6 REQUIRED COMPONENTS QuickTest)
    qt_add_executable(tst_ui tests/tst_ui.cpp)
    target_link_libraries(tst_ui PRIVATE mitori_app mitori_appplugin Qt6::QuickTest)
    target_compile_definitions(tst_ui PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
        UI_SHOTS_DIR="${CMAKE_BINARY_DIR}/ui-shots" UI_DATA_DIR="${CMAKE_BINARY_DIR}/ui-data"
        TEST_BUNDLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/bundle"
        TEST_MEDIA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/media")
    set_target_properties(tst_ui PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
    add_test(NAME ui COMMAND tst_ui)
    set_tests_properties(ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Material")
    # The same pages as Shinki (shinki/README.md, "Prüfen"); its shots in ui-shots/shinki.
    add_test(NAME ui_shinki COMMAND tst_ui)
    set_tests_properties(ui_shinki PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Material;MITORI_TEST_STYLE=shinki")
    # Both runs share the test data (ui-data): never at the same time.
    set_tests_properties(ui ui_shinki PROPERTIES RESOURCE_LOCK ui_data)
endif()

install(TARGETS mitori
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
