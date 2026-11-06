# ZXing-C++ for the DVD's QR code (platform/qrreader): QR only, static, no network. In this
# directory's scope, so the settings do not reach the app. What is left out: README.md here.
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)     # into the app's shared library on Android
set(ZXING_READERS ON)
set(ZXING_C_API OFF)
foreach(format 1D AZTEC DATAMATRIX MAXICODE PDF417)
    set(ZXING_ENABLE_${format} OFF)
endforeach()
set(ZXING_ENABLE_QRCODE ON)
# The writer only for the tests (they draw their QR codes); it never ships.
if(MITORI_BUILD_TESTS)
    set(ZXING_WRITERS OLD)
else()
    set(ZXING_WRITERS OFF)
endif()

# EXCLUDE_FROM_ALL: its install rules stay out of the app's (AppImage, Flatpak).
add_subdirectory(core EXCLUDE_FROM_ALL)
