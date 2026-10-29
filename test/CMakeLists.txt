find_library(CMOCKA_LIBRARY cmocka REQUIRED)
find_program(MKSQUASHFS mksquashfs REQUIRED)
find_program(OPENSSL openssl REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(FIXTURES ${CMAKE_CURRENT_BINARY_DIR}/fixtures)
set(FIXTURE_FILES
    ${FIXTURES}/keystore.bpak
    ${FIXTURES}/rootfs.bpak
    ${FIXTURES}/rootfs-hashed.bpak
    ${FIXTURES}/rootfs-unaligned.bpak
)

add_custom_command(
    OUTPUT ${FIXTURE_FILES}
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/mkfixtures.sh ${KICKSTART_BPAK_TOOL} ${FIXTURES}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mkfixtures.sh
    COMMENT "Creating the test images"
)

kickstart_generate_keystore(${FIXTURES}/keystore.bpak ${CMAKE_CURRENT_BINARY_DIR}/keystore.c)

add_library(kickstart-test STATIC common.c ${CMAKE_CURRENT_BINARY_DIR}/keystore.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/keystore.c PROPERTIES COMPILE_OPTIONS -w)
target_compile_definitions(kickstart-test PUBLIC KS_TEST_FIXTURES="${FIXTURES}")
target_link_libraries(kickstart-test PUBLIC kickstart-core ${CMOCKA_LIBRARY})
add_custom_target(kickstart-test-fixtures DEPENDS ${FIXTURE_FILES})
add_dependencies(kickstart-test kickstart-test-fixtures)

set(UNIT_TESTS
    test_boot
    test_chosen
    test_crc32
    test_image
    test_pbstate
    test_verity
)

foreach(test ${UNIT_TESTS})
    add_executable(${test} ${test}.c)
    target_link_libraries(${test} PRIVATE kickstart-test)
    add_test(NAME ${test} COMMAND ${test})
endforeach()

# The merkle trees of bpak are the hash trees of dm-verity
foreach(image rootfs rootfs-hashed rootfs-unaligned)
    add_test(NAME verity_ref_${image}
        COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/verity_ref.py
            ${FIXTURES}/${image}.bpak rootfs)
endforeach()
