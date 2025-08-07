cmake_minimum_required(VERSION 3.16)
project(BOBAOS C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Read directory from setup.sh
if(NOT DEFINED SCRIPT_PLATFORM_DIR)
    message(FATAL_ERROR "SCRIPT_PLATFORM_DIR not set. Pleas run ./setup.sh")
endif()

if(NOT DEFINED CROSS_PREFIX)
    message(FATAL_ERROR "CROSS_PREFIX not set. Pleas run ./setup.sh")
endif()

# Cross Compiler Toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER ${CROSS_PREFIX}x86_64-elf-gcc)
set(LINKER ${CROSS_PREFIX}x86_64-elf-ld)
set(OBJCOPY ${CROSS_PREFIX}x86_64-elf-objcopy)

# Compiler Flags
set(KERNEL_C_FLAGS 
	-g -ffreestanding -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -Wno-unused-parameter -finline-functions -fno-builtin -Wno-cpp -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Werror
	)
set(INCLUDE_DIRS 
	-I${CMAKE_SOURCE_DIR}/src/kernel -I${CMAKE_SOURCE_DIR}/inc
	)

# Handle bootloader and filesystem files
set(BOOT_ASM_BOOTLOADER_SRC ${CMAKE_SOURCE_DIR}/src/boot/boot.asm)
set(BOOT_ASM_BOOTLOADER_BIN ${CMAKE_BINARY_DIR}/bin/boot.bin)

set(BOOT_ASM_STAGE2_SRC ${CMAKE_SOURCE_DIR}/src/boot/stage2.asm)
set(BOOT_ASM_STAGE2_BIN ${CMAKE_BINARY_DIR}/bin/stage2.bin)

set(FILESYSTEM_ASM_FAT32HEADER_SRC ${CMAKE_SOURCE_DIR}/src/boot/fat32Header.asm)
SET(FILESYSTEM_ASM_FAT32HEADER_BIN ${CMAKE_BINARY_DIR}/bin/fat32Header.bin)

set(FILESYSTEM_ASM_FSINFO_SRC ${CMAKE_SOURCE_DIR}/src/boot/fsinfo.asm)
set(FILESYSTEM_ASM_FSINFO_BIN ${CMAKE_BINARY_DIR}/bin/fsinfo.bin)

set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)

add_custom_command(
	OUTPUT ${BOOT_ASM_BOOTLOADER_BIN}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
	COMMAND nasm -f bin ${BOOT_ASM_BOOTLOADER_SRC} -o ${BOOT_ASM_BOOTLOADER_BIN}
	DEPENDS ${BOOT_ASM_BOOTLOADER_SRC}
)

add_custom_command(
	OUTPUT ${BOOT_ASM_STAGE2_BIN}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
	COMMAND nasm -f bin ${BOOT_ASM_STAGE2_SRC} -o ${BOOT_ASM_STAGE2_BIN}
	DEPENDS ${BOOT_ASM_STAGE2_SRC}
)

add_custom_command(
	OUTPUT ${FILESYSTEM_ASM_FSINFO_BIN}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND nasm -f bin ${FILESYSTEM_ASM_FSINFO_SRC} -o ${FILESYSTEM_ASM_FSINFO_BIN}
	DEPENDS ${FILESYSTEM_ASM_FSINFO_SRC}
)

add_custom_command(
	OUTPUT ${FILESYSTEM_ASM_FAT32HEADER_BIN}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND nasm -f bin ${FILESYSTEM_ASM_FAT32HEADER_SRC} -o ${FILESYSTEM_ASM_FAT32HEADER_BIN}
	DEPENDS ${FILESYSTEM_ASM_FAT32HEADER_SRC}
)

# Handle kernel.asm
set(KERNEL_ASM_SRC ${CMAKE_SOURCE_DIR}/src/kernel/kernel.asm)
set(KERNEL_ASM_OBJ ${CMAKE_BINARY_DIR}/kernel.asm.o)

# Find all C-files
file(GLOB_RECURSE KERNEL_C_SOURCES CONFIGURE_DEPENDS src/kernel/*.c)

# Find all ASM-files but exclude kernel.asm
file(GLOB_RECURSE KERNEL_ASM_SOURCES CONFIGURE_DEPENDS src/kernel/*.asm)
list(FILTER KERNEL_ASM_SOURCES EXCLUDE REGEX ".*/kernel.asm$")

# Compile kernel.asm
add_custom_command(
		OUTPUT ${KERNEL_ASM_OBJ}
		COMMAND nasm -f elf64 -g ${KERNEL_ASM_SRC} -o ${KERNEL_ASM_OBJ}
		DEPENDS ${KERNEL_ASM_SRC}
)

# Compile ASM-Files except kernel.asm 
set(ASM_OBJECTS "")
foreach(ASM_FILE IN LISTS KERNEL_ASM_SOURCES)
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR}/src/kernel ${ASM_FILE})
    set(OBJ_PATH ${CMAKE_BINARY_DIR}/${REL_PATH}.o)
	get_filename_component(OBJ_DIR ${OBJ_PATH} DIRECTORY)
    add_custom_command(
            OUTPUT ${OBJ_PATH}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
            COMMAND nasm -f elf64 -g ${ASM_FILE} -o ${OBJ_PATH}
            DEPENDS ${ASM_FILE}
    )
    list(APPEND ASM_OBJECTS ${OBJ_PATH})
endforeach()

# Compile C-Files
set(C_OBJECTS "")
foreach(C_FILE IN LISTS KERNEL_C_SOURCES)
    file(RELATIVE_PATH REL_PATH ${CMAKE_SOURCE_DIR}/src/kernel ${C_FILE})
    set(OBJ_PATH ${CMAKE_BINARY_DIR}/${REL_PATH}.o)
	get_filename_component(OBJ_DIR ${OBJ_PATH} DIRECTORY)
    add_custom_command(
            OUTPUT ${OBJ_PATH}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
            COMMAND ${CMAKE_C_COMPILER} -m64 ${KERNEL_C_FLAGS} ${INCLUDE_DIRS} -c ${C_FILE} -o ${OBJ_PATH}
            DEPENDS ${C_FILE}
    )
    list(APPEND C_OBJECTS ${OBJ_PATH})
endforeach()

# Link
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/kernelfull.elf
        COMMAND ${LINKER} -g -m elf_x86_64 -T ${CMAKE_SOURCE_DIR}/src/kernel_link.ld
		${KERNEL_ASM_OBJ} ${ASM_OBJECTS} ${C_OBJECTS}
        -o ${CMAKE_BINARY_DIR}/kernelfull.elf
		DEPENDS ${KERNEL_ASM_OBJ} ${C_OBJECTS} ${ASM_OBJECTS}
)

# objcopy kernel.bin
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/kernel.bin
        COMMAND ${OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/kernelfull.elf ${CMAKE_BINARY_DIR}/kernel.bin
        DEPENDS ${CMAKE_BINARY_DIR}/kernelfull.elf
)

# Create final os.bin file
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/os.bin
    COMMAND dd if=${CMAKE_BINARY_DIR}/bin/boot.bin of=${CMAKE_BINARY_DIR}/os.bin conv=notrunc
    COMMAND dd if=${CMAKE_BINARY_DIR}/bin/stage2.bin of=${CMAKE_BINARY_DIR}/os.bin seek=1 conv=notrunc
    COMMAND dd if=${CMAKE_BINARY_DIR}/kernel.bin of=${CMAKE_BINARY_DIR}/os.bin seek=3 conv=notrunc
    COMMAND dd if=${CMAKE_BINARY_DIR}/bin/fat32header.bin of=${CMAKE_BINARY_DIR}/os.bin seek=2048 conv=notrunc
    COMMAND dd if=${CMAKE_BINARY_DIR}/bin/fsinfo.bin of=${CMAKE_BINARY_DIR}/os.bin seek=2049 conv=notrunc
    COMMAND truncate -s 35M ${CMAKE_BINARY_DIR}/os.bin 
    DEPENDS ${BOOT_ASM_BOOTLOADER_BIN} ${FILESYSTEM_ASM_FAT32HEADER_BIN} ${FILESYSTEM_ASM_FSINFO_BIN} ${BOOT_ASM_STAGE2_BIN} ${CMAKE_BINARY_DIR}/kernel.bin
)

add_custom_target(BobaOS ALL
    COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/${SCRIPT_PLATFORM_DIR}/addFat32Tables.sh ${CMAKE_BINARY_DIR}/os.bin
	DEPENDS ${CMAKE_BINARY_DIR}/os.bin
 )

add_custom_target(build_all DEPENDS BobaOS)

# =============== CUSTOM TARGET FOR IDS THAT CAN SCAN THE MAKEFILE ===============

add_custom_target(dummy_target_for_clion_indexing
		SOURCES
		${KERNEL_C_SOURCES}
		${KERNEL_ASM_SOURCES}
		${KERNEL_ASM_SRC}
		${BOOT_ASM_BOOTLOADER_SRC}
		${BOOT_ASM_FSINFO_SRC}
		${BOOT_ASM_STAGE2_SRC}
)
add_executable(clion_dummy_target EXCLUDE_FROM_ALL
		${KERNEL_C_SOURCES}
)
target_compile_options(clion_dummy_target PRIVATE ${KERNEL_C_FLAGS})
target_include_directories(clion_dummy_target PRIVATE ${CMAKE_SOURCE_DIR}/src/kernel)
