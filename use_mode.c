#include "use_mode.h"

const char *mode_to_string(enum use_mode mode)
{
    static const char *modes[] = {"OFF", "NORMAL", "LIMITED"};
    return modes[mode];
}