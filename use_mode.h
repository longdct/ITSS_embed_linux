#ifndef USE_MODE_H
#define USE_MODE_H

enum use_mode
{
    OFF,
    ORDINAL,
    LIMITED
};

const char *mode_to_string(enum use_mode mode);

#endif // USE_MODE_H