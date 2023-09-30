cmake_minimum_required(VERSION 3.0.0)
project(cpp_template_proj VERSION 0.1.0 LANGUAGES C CXX)

add_executable(${PROJECT_NAME}
  main.cpp
  TcpSocket.h
  TcpSocket.cpp
  Server.h
  Server.cpp
)

#add_subdirectory(client)

if(ASAN_ENABLED)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /fsanitize=address)
  else()
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
  endif()
endif()
