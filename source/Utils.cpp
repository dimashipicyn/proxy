#include "Utils.h"

#include <cerrno>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

std::string getError()
{
    char buffer[BUFSIZ] = {};
    strerror_s(buffer, BUFSIZ, errno);
    return std::string(buffer);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
std::string getTimeStamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream datetime;
    datetime << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%X");

    return datetime.str();
}
#pragma clang diagnostic pop