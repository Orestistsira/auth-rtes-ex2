cmake_minimum_required(VERSION 3.16)
project(prod_cons_timer C)

set(CMAKE_C_STANDARD 99)

add_executable(prod_cons_timer main.c queue.c queue.h timer.c timer.h)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(prod_cons_timer m Threads::Threads)
