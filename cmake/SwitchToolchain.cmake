# CMake toolchain para Nintendo Switch (devkitA64)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# devkitPro paths
set(DEVKITPRO $ENV{DEVKITPRO})
set(DEVKITA64 ${DEVKITPRO}/devkitA64)
set(LIBNX ${DEVKITPRO}/libnx)
set(PORTLIBS ${DEVKITPRO}/portlibs/switch)

# Compilers
set(CMAKE_C_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITA64}/bin/aarch64-none-elf-g++)
set(CMAKE_AR ${DEVKITA64}/bin/aarch64-none-elf-ar)
set(CMAKE_RANLIB ${DEVKITA64}/bin/aarch64-none-elf-ranlib)
set(CMAKE_STRIP ${DEVKITA64}/bin/aarch64-none-elf-strip)

# Flags
set(ARCH_FLAGS "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE")
set(CMAKE_C_FLAGS "${ARCH_FLAGS} -D__SWITCH__ -I${LIBNX}/include -I${PORTLIBS}/include" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${ARCH_FLAGS} -D__SWITCH__ -I${LIBNX}/include -I${PORTLIBS}/include -std=gnu++17" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "-specs=${LIBNX}/switch.specs -L${LIBNX}/lib -L${PORTLIBS}/lib -lnx" CACHE STRING "")

# Search paths
set(CMAKE_FIND_ROOT_PATH ${DEVKITA64} ${LIBNX} ${PORTLIBS})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Platform flags para borealis
set(PLATFORM_SWITCH ON)
set(USE_DEKO3D ON)
set(PLATFORM_DESKTOP OFF)
