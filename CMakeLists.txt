cmake_minimum_required(VERSION 3.13)

# version.txt is the single source of the version
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" KICKSTART_VERSION LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version.txt)

project(kickstart
    VERSION ${KICKSTART_VERSION}
    DESCRIPTION "Init ramdisk for punchboot"
    HOMEPAGE_URL https://github.com/jonasblixt/kickstart
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

option(KICKSTART_BUILD_INIT "Build the init, statically linked" ON)
option(KICKSTART_BUILD_TESTS "Build the unit tests" OFF)
option(KICKSTART_WERROR "Treat compiler warnings as errors" ON)
set(KICKSTART_BPAK_SOURCE_DIR "" CACHE PATH
    "Build libbpak from this source tree instead of using an installed libbpak")
set(KICKSTART_BPAK_TOOL "bpak" CACHE FILEPATH "The bpak tool of the build host")

# Build time configuration, kickstart has no configuration file. Paths are
# block devices on the target, kickstart does not look up partitions.
set(KICKSTART_ROOT_A "" CACHE STRING "Block device with the root image of system A")
set(KICKSTART_ROOT_B "" CACHE STRING "Block device with the root image of system B")
set(KICKSTART_ROOT_PART "rootfs" CACHE STRING
    "Part of the root image with the file system, the merkle tree is <part>-hash-tree")
set(KICKSTART_ROOT_FSTYPE "squashfs" CACHE STRING "File system of the root image")
set(KICKSTART_ROOT_OPTIONS "" CACHE STRING "Mount options of the root file system")
set(KICKSTART_PBSTATE_PRIMARY "" CACHE STRING "Block device of the primary pbstate, optional")
set(KICKSTART_PBSTATE_BACKUP "" CACHE STRING "Block device of the backup pbstate, optional")
set(KICKSTART_CHOSEN "/proc/device-tree/chosen" CACHE STRING
    "The /chosen node of the device tree from punchboot")
set(KICKSTART_INIT "/sbin/init" CACHE STRING "Init of the root file system")
set(KICKSTART_DEVICE_TIMEOUT_MS 10000 CACHE STRING
    "Milliseconds to wait for the block devices")
set(KICKSTART_VERITY_ON_CORRUPTION "restart" CACHE STRING
    "What dm-verity does on a corrupted block: eio, restart or panic")
set_property(CACHE KICKSTART_VERITY_ON_CORRUPTION PROPERTY STRINGS eio restart panic)
option(KICKSTART_REQUIRE_PB_KEYS
    "Only accept root images signed with keys in /chosen/pb,slc-available-keys" ON)
set(KICKSTART_LOG_LEVEL "info" CACHE STRING "Log level: error, warning, info or debug")
set_property(CACHE KICKSTART_LOG_LEVEL PROPERTY STRINGS error warning info debug)
set(KICKSTART_KEYSTORE "" CACHE FILEPATH
    "bpak keystore archive with the keys of the root images, compiled into the init")

option(KICKSTART_SANITIZE "Build with AddressSanitizer and UBSan, for the tests" OFF)

add_compile_options(-Wall -Wextra)
if (KICKSTART_WERROR)
    add_compile_options(-Werror)
endif()

if (KICKSTART_SANITIZE)
    add_compile_options(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# libbpak with the mbedtls crypto backend
if (KICKSTART_BPAK_SOURCE_DIR)
    add_subdirectory(${KICKSTART_BPAK_SOURCE_DIR} ${CMAKE_BINARY_DIR}/bpak EXCLUDE_FROM_ALL)
    set(KICKSTART_BPAK_LIBRARY bpak-static)
else()
    find_path(BPAK_INCLUDE_DIR bpak/bpak.h REQUIRED)
    find_library(BPAK_STATIC_LIBRARY NAMES libbpak.a bpak REQUIRED)
    add_library(kickstart-bpak INTERFACE)
    target_include_directories(kickstart-bpak INTERFACE ${BPAK_INCLUDE_DIR})
    target_link_libraries(kickstart-bpak INTERFACE ${BPAK_STATIC_LIBRARY})
    set(KICKSTART_BPAK_LIBRARY kickstart-bpak)
endif()

# Everything but the build time configuration, the unit tests link this
add_library(kickstart-core STATIC
    src/boot.c
    src/chosen.c
    src/crc32.c
    src/dm.c
    src/image.c
    src/log.c
    src/pbstate.c
    src/rootfs.c
    src/util.c
)
target_include_directories(kickstart-core PUBLIC src)
# The libraries are linked by name, a static link picks the archives
target_link_libraries(kickstart-core PUBLIC ${KICKSTART_BPAK_LIBRARY} mbedcrypto)

function(kickstart_generate_keystore keystore output)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND}
            -DBPAK=${KICKSTART_BPAK_TOOL}
            -DKEYSTORE=${keystore}
            -DOUTPUT=${output}
            -P ${PROJECT_SOURCE_DIR}/cmake/GenerateKeystore.cmake
        DEPENDS ${keystore} ${PROJECT_SOURCE_DIR}/cmake/GenerateKeystore.cmake
        COMMENT "Generating the keystore of ${keystore}"
    )
endfunction()

if (KICKSTART_BUILD_INIT)
    # The init is linked statically, the libc must allow that without
    # obligations for the image: glibc and uClibc-ng are LGPL, use musl
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <features.h>
        #if defined(__GLIBC__) || defined(__UCLIBC__)
        #error LGPL libc
        #endif
        int main(void) { return 0; }" KICKSTART_LIBC_IS_PERMISSIVE)

    if (NOT KICKSTART_LIBC_IS_PERMISSIVE)
        message(FATAL_ERROR
            "The init is statically linked and the C library is glibc or uClibc-ng "
            "(LGPL). Build it with a musl toolchain, or configure with "
            "-DKICKSTART_BUILD_INIT=OFF to only build the unit tests.")
    endif()

    foreach(var KICKSTART_ROOT_A KICKSTART_ROOT_B KICKSTART_KEYSTORE)
        if ("${${var}}" STREQUAL "")
            message(FATAL_ERROR "${var} must be set to build the init")
        endif()
    endforeach()

    if ("${KICKSTART_PBSTATE_PRIMARY}" STREQUAL "")
        set(no_primary TRUE)
    else()
        set(no_primary FALSE)
    endif()
    if ("${KICKSTART_PBSTATE_BACKUP}" STREQUAL "")
        set(no_backup TRUE)
    else()
        set(no_backup FALSE)
    endif()
    if (NOT no_primary STREQUAL no_backup)
        message(FATAL_ERROR "Set both or none of KICKSTART_PBSTATE_PRIMARY and _BACKUP")
    endif()

    foreach(var KICKSTART_ROOT_A KICKSTART_ROOT_B KICKSTART_ROOT_PART KICKSTART_ROOT_FSTYPE
                KICKSTART_ROOT_OPTIONS KICKSTART_PBSTATE_PRIMARY KICKSTART_PBSTATE_BACKUP
                KICKSTART_CHOSEN KICKSTART_INIT)
        if ("${${var}}" MATCHES "[\"\\\\]")
            message(FATAL_ERROR "${var} must not contain quotes or backslashes")
        endif()
    endforeach()

    if (NOT KICKSTART_DEVICE_TIMEOUT_MS MATCHES "^[0-9]+$")
        message(FATAL_ERROR "KICKSTART_DEVICE_TIMEOUT_MS must be a number")
    endif()

    string(TOUPPER "${KICKSTART_VERITY_ON_CORRUPTION}" mode)
    if (NOT mode MATCHES "^(EIO|RESTART|PANIC)$")
        message(FATAL_ERROR "KICKSTART_VERITY_ON_CORRUPTION must be eio, restart or panic")
    endif()
    set(KS_CONFIG_VERITY_MODE KS_VERITY_${mode})

    string(TOUPPER "${KICKSTART_LOG_LEVEL}" level)
    if (level STREQUAL "ERROR")
        set(level ERR)
    elseif (level STREQUAL "WARNING")
        set(level WARN)
    elseif (level STREQUAL "DEBUG")
        set(level DBG)
    elseif (NOT level STREQUAL "INFO")
        message(FATAL_ERROR "KICKSTART_LOG_LEVEL must be error, warning, info or debug")
    endif()
    set(KS_CONFIG_LOG_LEVEL KS_LOG_${level})

    if (KICKSTART_REQUIRE_PB_KEYS)
        set(KS_CONFIG_REQUIRE_PB_KEYS true)
    else()
        set(KS_CONFIG_REQUIRE_PB_KEYS false)
    endif()

    configure_file(include/kickstart/build_config.h.in
        ${CMAKE_BINARY_DIR}/include/kickstart/build_config.h @ONLY)

    # A keystore archive, or its code from 'bpak generate keystore --name
    # kickstart'
    get_filename_component(keystore ${KICKSTART_KEYSTORE} ABSOLUTE)
    if (keystore MATCHES "\\.c$")
        set(keystore_c ${keystore})
    else()
        set(keystore_c ${CMAKE_BINARY_DIR}/keystore.c)
        kickstart_generate_keystore(${keystore} ${keystore_c})
    endif()

    add_executable(kickstart src/main.c src/config.c ${keystore_c})
    target_include_directories(kickstart PRIVATE ${CMAKE_BINARY_DIR}/include)
    target_link_libraries(kickstart PRIVATE kickstart-core)
    target_link_options(kickstart PRIVATE -static -Wl,--gc-sections)
    # The generated code initializes a flexible array member
    set_source_files_properties(${keystore_c} PROPERTIES COMPILE_OPTIONS -w)

    include(GNUInstallDirs)
    install(TARGETS kickstart RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
endif()

if (KICKSTART_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
