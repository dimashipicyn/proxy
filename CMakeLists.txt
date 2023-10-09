cmake_minimum_required(VERSION 3.20.0)
project(proxy VERSION 0.1.0 LANGUAGES C CXX)

add_executable(${PROJECT_NAME})
add_subdirectory(source)

if (MSVC)
target_compile_options(${PROJECT_NAME} PUBLIC /Wall /pedantic)
else()
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Werror -pedantic)
endif()

if(ASAN_ENABLED)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /fsanitize=address)
  else()
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
  endif()
endif()

if(GPERF_ENABLED)
target_compile_options(${PROJECT_NAME} PUBLIC -pg)
endif()