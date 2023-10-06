#include "PostgresqlParser.h"

#include "Log.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <netinet/in.h>

void PostgresqlParser::parse(const char* s, int size)
{
    buffer.append(s, size);

    while (true)
    {
        switch (state_)
        {
        case ParseType:
        {
            // Type 1 byte
            if (buffer.size() >= 1)
            {
                // '\0' - StartupMessage, CancelRequest, SslRequest without type
                if (buffer.front() != '\0')
                {
                    currentPacket_.type = buffer.front();
                    buffer.erase(0, 1);
                }
                state_ = ParseLength;
            }
            else
            {
                return;
            }
            break;
        }
        case ParseLength:
        {
            // Length 4 byte
            if (buffer.size() >= 4)
            {
                currentPacket_.length = netToHost((uint8_t*)buffer.data());
                currentPacket_.data.reserve(currentPacket_.length);

                buffer.erase(0, 4);
                state_ = ParseData;
            }
            else
            {
                return;
            }
            break;
        }
        case ParseData:
        {
            size_t lengthRequired = std::max(currentPacket_.length - 4, 0); // 4 byte sizeof length
            if (buffer.length() >= lengthRequired)
            {
                currentPacket_.data.append(buffer.c_str(), lengthRequired);
                buffer.erase(0, lengthRequired);
                
                packets_.push_back(std::move(currentPacket_));
                state_ = ParseType;
            }
            else
            {
                return;
            }
            break;
        }
        default:
        {
            LOG_ERROR("Undefined state! %d\n", state_);
            assert(false);
        }
        }
    }
}

std::vector<PostgresqlPacket> PostgresqlParser::getPackets()
{
    return std::move(packets_);
}

int32_t PostgresqlParser::netToHost(uint8_t number[4])
{
    int32_t result = 0;
    memcpy(&result, number, 4);
    return ntohl(result);
}
