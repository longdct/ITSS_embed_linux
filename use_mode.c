#include "use_mode.h"

const char *mode_to_string(enum use_mode mode)
{
    static const char *modes[] = {"OFF", "ORDINAL", "LIMITED"};
    return modes[mode];
}