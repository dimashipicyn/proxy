
target_sources(${PROJECT_NAME} PUBLIC
  main.cpp
  TcpSocket.h
  TcpSocket.cpp
  Server.h
  Server.cpp
  NonCopyable.h
  Log.h
  PostgresqlParser.h
  PostgresqlParser.cpp
  Utils.h
  Utils.cpp
)

if (WIN32)
target_sources(${PROJECT_NAME} PUBLIC
  GetOpt.h
  GetOpt.cpp
)
endif()