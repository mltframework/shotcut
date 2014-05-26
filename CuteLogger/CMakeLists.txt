CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(CuteLogger)

FIND_PACKAGE(Qt4 COMPONENTS QtCore REQUIRED)
INCLUDE(${QT_USE_FILE})

ADD_DEFINITIONS(-DCUTELOGGER_LIBRARY)

INCLUDE_DIRECTORIES(BEFORE include)

SET(sources
  src/Logger.cpp
  src/AbstractAppender.cpp
  src/AbstractStringAppender.cpp
  src/ConsoleAppender.cpp
  src/FileAppender.cpp
)

SET(includes
  include/Logger.h
  include/FileAppender.h
  include/CuteLogger_global.h
  include/ConsoleAppender.h
  include/AbstractStringAppender.h
  include/AbstractAppender.h
 )


# OutputDebugAppender is only for Windows systems
IF(WIN32)
  SET(sources ${sources} src/OutputDebugAppender.cpp)
  SET(includes ${includes} include/OutputDebugAppender.h)
ENDIF(WIN32)


SET(library_target Logger)

ADD_LIBRARY(${library_target} SHARED ${sources} ${includes})
TARGET_LINK_LIBRARIES(${library_target} ${QT_LIBRARIES})
INSTALL(TARGETS ${library_target} DESTINATION lib)
