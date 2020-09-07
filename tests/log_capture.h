#pragma once

#include <string>

struct ForthContext;

class LogCapturer final
{
public:
    LogCapturer(ForthContext* ctx);

    static std::string log;
};
