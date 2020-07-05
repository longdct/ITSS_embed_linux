#ifndef USE_MODE_H
#define USE_MODE_H

enum use_mode
{
    OFF,
    NORMAL,
    LIMITED
};

/**
 * Convert enum type of mode into string
 * */
const char *mode_to_string(enum use_mode mode);

#endif // USE_MODE_H