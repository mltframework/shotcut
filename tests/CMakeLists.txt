find_package(Qt6 6.4 REQUIRED COMPONENTS Core Test)

add_executable(test_gpuinfo
  test_gpuinfo.cpp
  ${CMAKE_SOURCE_DIR}/src/gpuinfo.cpp
)
target_include_directories(test_gpuinfo PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_gpuinfo PRIVATE Qt6::Core Qt6::Test)
if(WIN32)
  target_link_libraries(test_gpuinfo PRIVATE dxgi dxguid)
endif()
add_test(NAME gpuinfo COMMAND test_gpuinfo)

add_executable(test_mix_transition_order
  test_mix_transition_order.cpp
  ${CMAKE_SOURCE_DIR}/src/models/mixtransitionorder.cpp
)
target_include_directories(test_mix_transition_order PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_mix_transition_order PRIVATE Qt6::Core Qt6::Test PkgConfig::mlt++)
add_test(NAME mix_transition_order COMMAND test_mix_transition_order)

add_executable(test_dataqueue
  test_dataqueue.cpp
)
target_include_directories(test_dataqueue PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_dataqueue PRIVATE Qt6::Core Qt6::Test)
add_test(NAME dataqueue COMMAND test_dataqueue)

add_executable(test_exportpresetdialog
  test_exportpresetdialog.cpp
  ${CMAKE_SOURCE_DIR}/src/dialogs/addencodepresetdialog.cpp
)
target_include_directories(test_exportpresetdialog PRIVATE ${CMAKE_SOURCE_DIR}/src/dialogs)
target_link_libraries(test_exportpresetdialog PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME exportpresetdialog COMMAND test_exportpresetdialog -platform offscreen)

add_executable(test_addonqmlgenerator
  test_addonqmlgenerator.cpp
  ${CMAKE_SOURCE_DIR}/src/controllers/addonqmlgenerator.cpp
)
target_include_directories(test_addonqmlgenerator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_addonqmlgenerator PRIVATE Qt6::Core Qt6::Test PkgConfig::mlt++)
add_test(NAME addonqmlgenerator COMMAND test_addonqmlgenerator)
