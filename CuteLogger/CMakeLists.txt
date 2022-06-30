CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(CuteLogger)

FIND_PACKAGE(Qt5Core REQUIRED)

ADD_DEFINITIONS(-DCUTELOGGER_LIBRARY)

INCLUDE(GNUInstallDirs)

INCLUDE_DIRECTORIES(BEFORE include)

SET(sources
  src/Logger.cpp
  src/AbstractAppender.cpp
  src/AbstractStringAppender.cpp
  src/ConsoleAppender.cpp
  src/FileAppender.cpp
  src/RollingFileAppender.cpp
)

SET(includes
  include/Logger.h
  include/FileAppender.h
  include/CuteLogger_global.h
  include/ConsoleAppender.h
  include/AbstractStringAppender.h
  include/AbstractAppender.h
  include/RollingFileAppender.h
 )


# OutputDebugAppender is only for Windows systems
IF(WIN32)
  SET(sources ${sources} src/OutputDebugAppender.cpp)
  SET(includes ${includes} include/OutputDebugAppender.h)
ENDIF(WIN32)


SET(library_target CuteLogger)

ADD_LIBRARY(${library_target} SHARED ${sources} ${includes})
TARGET_LINK_LIBRARIES(${library_target} Qt5::Core)
INSTALL(TARGETS ${library_target}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
