cmake_minimum_required(VERSION 3.21)
project(DebuggerTemplate VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# Board core: no widgets, so every safety rule can be tested with no display.
add_library(debugger_core STATIC
    src/board/BoardLink.h
    src/board/BoardProfile.h
    src/board/BoardWorker.cpp
    src/board/BoardWorker.h
    src/board/FakeBoard.cpp
    src/board/FakeBoard.h
    src/board/Session.cpp
    src/board/Session.h
    src/board/StatusPoller.cpp
    src/board/StatusPoller.h
    src/logic/MoveLimits.h
    src/logic/MoveWatch.h
    src/logic/StopCheck.h
)
target_include_directories(debugger_core PUBLIC src)
target_link_libraries(debugger_core PUBLIC Qt6::Core)

add_library(debugger_ui STATIC
    resources/theme.qrc
    src/ui/HeaderBar.cpp
    src/ui/HeaderBar.h
    src/ui/HomePage.cpp
    src/ui/HomePage.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MovePage.cpp
    src/ui/MovePage.h
    src/ui/SafetyInputFilter.cpp
    src/ui/SafetyInputFilter.h
    src/ui/StateDot.cpp
    src/ui/StateDot.h
    src/ui/Theme.cpp
    src/ui/Theme.h
)
target_link_libraries(debugger_ui PUBLIC debugger_core Qt6::Widgets)

add_executable(debugger_template src/main.cpp src/MakeBoardLink.cpp src/MakeBoardLink.h)
target_link_libraries(debugger_template PRIVATE debugger_ui)
target_compile_definitions(debugger_template PRIVATE TOOL_VERSION="${PROJECT_VERSION}")

enable_testing()

function(add_core_test test_name)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE debugger_core Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_core_test(test_logic)
add_core_test(test_worker)
add_core_test(test_session)

add_executable(test_ui tests/test_ui.cpp)
target_link_libraries(test_ui PRIVATE debugger_ui Qt6::Test)
add_test(NAME test_ui COMMAND test_ui)
set_tests_properties(test_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The real main.cpp with a board link that aborts when made.
add_executable(screenshot_run_under_test src/main.cpp tests/forbidden_board_link.cpp)
target_include_directories(screenshot_run_under_test PRIVATE src)
target_link_libraries(screenshot_run_under_test PRIVATE debugger_ui)
target_compile_definitions(screenshot_run_under_test PRIVATE TOOL_VERSION="${PROJECT_VERSION}")
add_test(NAME screenshot_run
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/check_screenshot_run.sh $<TARGET_FILE:screenshot_run_under_test>)

add_test(NAME no_hard_coded_colors
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/check_no_hard_coded_colors.sh ${CMAKE_SOURCE_DIR}/src)
