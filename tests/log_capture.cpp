#include <cstdarg>
#include <functional>
#include <sstream>
#include <vector>

#include <forth/forth.h>

#include "log_capture.h"

std::string LogCapturer::log;
static std::stringstream ss;

std::string vformat(const char *fmt, va_list ap)
{
    // Allocate a buffer on the stack that's big enough for us almost
    // all the time.
    size_t size = 1024;
    char buf[1024];

    // Try to vsnprintf into our buffer.
    va_list apcopy;
    va_copy (apcopy, ap);
    int needed = vsnprintf(&buf[0], size, fmt, ap);
    // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
    // buffer.  On Linux & OSX, it returns the length it would have needed.

    if (needed <= size && needed >= 0)
    {
        // It fit fine the first time, we're done.
        return std::string(&buf[0]);
    }
    else
    {
        // vsnprintf reported that it wanted to write more characters
        // than we allotted.  So do a malloc of the right size and try again.
        // This doesn't happen very often if we chose our initial size
        // well.
        std::vector<char> buf;
        size = needed;
        buf.resize (size);
        needed = vsnprintf(&buf[0], size, fmt, apcopy);
        return std::string(&buf[0]);
    }
}

static int on_log(ForthContext* ctx, const char* fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    std::string buf = vformat(fmt, ap);
    va_end (ap);

    LogCapturer::log += buf;

    return (int)buf.size();
}

LogCapturer::LogCapturer(ForthContext* ctx)
{
    log.clear();
    ss.clear();

    ctx->log = on_log;
}
