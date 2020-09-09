#pragma once

#include <string>
#include <vector>

#include <forth/forth.h>

#include "log_capture.h"

void evalTestSection(ForthContext* ctx,
                     const char* eval_expression,
                     int expected_result,
                     const std::vector<int64_t>& expected_stack,
                     const std::string& expected_log = "")
{
    SECTION(eval_expression)
    {
        LogCapturer log_capturer(ctx);

        auto eval_ret = forth_eval(ctx, eval_expression);
        INFO(LogCapturer::log.c_str());
        CHECK(eval_ret == expected_result);
        if (eval_ret != expected_result)
            return;

        bool failed = false;

        REQUIRE(ctx->stack_pointer == (int)expected_stack.size());
        for (int i = 0, len = (int)expected_stack.size(); i < len; i++)
        {
            auto top = forth_top(ctx, len - i - 1);
            if (top && top->int_value != expected_stack[i])
            {
                failed = true;
                break;
            }
        }

        std::stringstream ss;
        for (int i = 0; i < ctx->stack_pointer; i++)
        {
            ss << ctx->stack[i].int_value << " ";
        }
        ss << " != ";
        for (auto val : expected_stack)
            ss << val << " ";
        auto str = ss.str();
        INFO(str.c_str());
        CHECK(!failed);

        REQUIRE(LogCapturer::log == expected_log);
    }
}

void evalTest(ForthContext* ctx, 
              const char* eval_expression,
              int expected_result,
              const std::vector<int64_t>& expected_stack,
              const std::string& expected_log = "")
{
    {
        INFO(eval_expression);

        LogCapturer log_capturer(ctx);

        auto eval_ret = forth_eval(ctx, eval_expression);
        INFO(LogCapturer::log.c_str());
        CHECK(eval_ret == expected_result);
        if (eval_ret != expected_result)
            return;

        bool failed = false;

        REQUIRE(ctx->stack_pointer == (int)expected_stack.size());
        for (int i = 0, len = (int)expected_stack.size(); i < len; i++)
        {
            auto top = forth_top(ctx, len - i - 1);
            if (top && top->int_value != expected_stack[i])
            {
                failed = true;
                break;
            }
        }

        std::stringstream ss;
        for (int i = 0; i < ctx->stack_pointer; i++)
        {
            ss << ctx->stack[i].int_value << " ";
        }
        ss << " != ";
        for (auto val : expected_stack)
            ss << val << " ";
        auto str = ss.str();
        INFO(str.c_str());
        CHECK(!failed);

        REQUIRE(LogCapturer::log == expected_log);
    }
}
