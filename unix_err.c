#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

void
err(int ec, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void) vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}

void
warnx(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void) vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}

void
warn(const char *fmt, ...)
{
    va_list args;

    printf("%d: ", errno);
    va_start(args, fmt);
    (void) vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}
