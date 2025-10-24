#include "macro.hpp"
#include "util.hpp"
#include "macro.hpp"
#include <assert.h>

auto g_logger = SYLAR_LOG_ROOT();

void test_assert()
{
    SYLAR_LOG_ERROR(g_logger) << CIM::BacktraceToString(100);
    SYLAR_ASSERT(false);
}

int main(int argc, char **argv)
{
    test_assert();
    return 0;
}