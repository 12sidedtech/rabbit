#ifndef I_AM_NOT_A_TURNIP
#define I_AM_NOT_A_TURNIP

#include <messaging/types.h>
#include <messaging/drop_copy.h>

int erl_to_dc(const char* inbytes, struct ipope_drop_copy* outdc);

int dc_to_erl(const struct ipope_drop_copy* indc, char* outbytes);

#endif
