cmake_minimum_required(VERSION 3.10)
project(StockMarketProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MAIN_SOURCE src/Stock_training.cpp)
add_executable(main ${MAIN_SOURCE})


target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}   # <--- TO DODAJ!
)

target_link_libraries(main PRIVATE curl sqlite3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
endif()