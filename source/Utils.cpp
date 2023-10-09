#include "Utils.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <ctime>

std::string getError()
{
    char buffer[BUFSIZ] = {};
#ifdef _WIN32
    strerror_s(buffer, BUFSIZ, errno);
#elif __APPLE__
    strerror_r(errno, buffer, BUFSIZ);
#else
    char* result = strerror_r(errno, buffer, BUFSIZ);
    return std::string(result);
#endif
    return std::string(buffer);
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
std::string getTimeStamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream datetime;
    datetime << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");

    return datetime.str();
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif