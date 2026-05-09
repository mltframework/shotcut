find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_mainwindow mainwindowtest.cpp)
target_link_libraries(test_mainwindow PRIVATE shotcut_objects Qt6::Test)
target_include_directories(test_mainwindow PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME MainWindowTest COMMAND test_mainwindow)
set_tests_properties(MainWindowTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
