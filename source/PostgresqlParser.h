#pragma once

#include <cstdint>
#include <vector>
#include <string>

struct PostgresqlPacket
{
    uint8_t type = 0;
    int32_t length = 0;
    std::string data;
};

class PostgresqlParser
{
public:
    void parse(const char* s, int size);
    
    std::vector<PostgresqlPacket> getPackets();

private:
    int32_t netToHost(uint8_t number[4]);

private:
    enum State
    {
        ParseType,
        ParseLength,
        ParseData
    };

    std::string buffer;
    std::vector<PostgresqlPacket> packets_;
    PostgresqlPacket currentPacket_;
    State state_ = ParseType;
};