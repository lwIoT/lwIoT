SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERISON 1)

SET(CMAKE_C_COMPILER avr-gcc)
SET(CMAKE_CXX_COMPILER avr-g++)
set(HAVE_BIG_ENDIAN False)


# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(MCU "atmega2560" CACHE STRING "AVR MCU type")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmcu=${MCU}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmcu=${MCU}")
