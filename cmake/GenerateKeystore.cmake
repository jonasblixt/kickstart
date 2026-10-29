# Generate the C code of a bpak keystore archive
#
# cmake -DBPAK=<bpak tool> -DKEYSTORE=<keystore.bpak> -DOUTPUT=<file.c> \
#       -P GenerateKeystore.cmake

execute_process(
    COMMAND ${BPAK} generate keystore ${KEYSTORE} --name kickstart
    OUTPUT_FILE ${OUTPUT}.tmp
    RESULT_VARIABLE result
)

if (NOT result EQUAL 0)
    file(REMOVE ${OUTPUT}.tmp)
    message(FATAL_ERROR "bpak generate keystore ${KEYSTORE} failed (${result})")
endif()

file(RENAME ${OUTPUT}.tmp ${OUTPUT})
