
cmake_minimum_required(VERSION 3.5)

project(QTabFrameworkExample VERSION 1.0.0)

find_package(Qt6 COMPONENTS Widgets)

qt_wrap_cpp(METASOURCES QTabFramework.hpp)

add_executable(QTabFrameworkExample
    Example.cpp
    QTabFramework.cpp
    QTabFramework.hpp
    ${METASOURCES}
)
target_link_libraries(QTabFrameworkExample
    PRIVATE Qt6::Widgets
)
