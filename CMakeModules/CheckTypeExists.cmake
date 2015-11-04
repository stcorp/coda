#
# Check if a type exists in include files
#
# CHECK_TYPE_EXISTS - macro which checks the type exists in include files.
# TYPE - type
# FILES  - include files to check
# VARIABLE - variable to return result
#
# If CMAKE_REQUIRED_FLAGS is set then those flags will be passed into the
# compile of the program likewise if CMAKE_REQUIRED_LIBRARIES is set then
# those libraries will be linked against the test program


MACRO(CHECK_TYPE_EXISTS TYPE FILES VARIABLE)
  IF("${VARIABLE}" MATCHES "^${VARIABLE}$")
    SET(CHECK_TYPE_EXISTS_CONTENT "/* */\n")
    SET(MACRO_CHECK_TYPE_EXISTS_FLAGS ${CMAKE_REQUIRED_FLAGS})
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(CHECK_TYPE_EXISTS_LIBS 
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ENDIF(CMAKE_REQUIRED_LIBRARIES)
    FOREACH(FILE ${FILES})
      SET(CHECK_TYPE_EXISTS_CONTENT
        "${CHECK_TYPE_EXISTS_CONTENT}#include <${FILE}>\n")
    ENDFOREACH(FILE)
    SET(CHECK_TYPE_EXISTS_CONTENT
      "${CHECK_TYPE_EXISTS_CONTENT}\nint main()\n{\n${TYPE} var;\n  return 0;\n}\n")

    FILE(WRITE ${CMAKE_BINARY_DIR}/CMakeTmp/CheckTypeExists.c 
      "${CHECK_TYPE_EXISTS_CONTENT}")

    MESSAGE(STATUS "Looking for ${TYPE}")
    TRY_COMPILE(${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/CMakeTmp/CheckTypeExists.c
      CMAKE_FLAGS 
      -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_TYPE_EXISTS_FLAGS}
      "${CHECK_TYPE_EXISTS_LIBS}"
      OUTPUT_VARIABLE OUTPUT)
    IF(${VARIABLE})
      MESSAGE(STATUS "Looking for ${TYPE} - found")
      SET(${VARIABLE} 1 CACHE INTERNAL "Have type ${TYPE}")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log 
        "Determining if the ${TYPE} "
        "exist passed with the following output:\n"
        "${OUTPUT}\nFile ${CMAKE_BINARY_DIR}/CMakeTmp/CheckTypeExists.c:\n"
        "${CHECK_TYPE_EXISTS_CONTENT}\n")
    ELSE(${VARIABLE})
      MESSAGE(STATUS "Looking for ${TYPE} - not found.")
      SET(${VARIABLE} "" CACHE INTERNAL "Have type ${TYPE}")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log 
        "Determining if the ${TYPE} "
        "exist failed with the following output:\n"
        "${OUTPUT}\nFile ${CMAKE_BINARY_DIR}/CMakeTmp/CheckTypeExists.c:\n"
        "${CHECK_TYPE_EXISTS_CONTENT}\n")
    ENDIF(${VARIABLE})
  ENDIF("${VARIABLE}" MATCHES "^${VARIABLE}$")
ENDMACRO(CHECK_TYPE_EXISTS)
