#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <erl_interface.h>
#include <ei.h>
#include "erl_msgs.h"


int erl_to_dc(const char* inbytes, struct ipope_drop_copy* outdc)
{

    char atom[15] = { 0 };
    int ver = 0;
    int arity = 0;
    int i = 0;

    ei_decode_version(inbytes, &i, &ver);
    assert(0 == ei_decode_tuple_header(inbytes, &i, &arity));
    assert(9 == arity);
    assert(0 == ei_decode_atom(inbytes, &i, atom));
    assert(0 == strcmp(atom, "drop_copy"));
    ei_decode_ulong(inbytes, &i, (unsigned long*) &outdc->symbol_id);
    ei_decode_ulong(inbytes, &i, (unsigned long*) &outdc->passive_party);
    ei_decode_ulong(inbytes, &i, (unsigned long*) &outdc->aggressor_party);
    ei_decode_ulong(inbytes, &i, (unsigned long*) &outdc->exec_size);
    ei_decode_ulong(inbytes, &i, (unsigned long*) &outdc->exec_price);
    ei_decode_ulonglong(inbytes, &i, (unsigned long long*) &outdc->passive_order_id);
    ei_decode_ulonglong(inbytes, &i, (unsigned long long*) &outdc->aggressor_order_id);
    ei_decode_ulonglong(inbytes, &i, (unsigned long long*) &outdc->exec_ts);

    return i;
}

int dc_to_erl(const struct ipope_drop_copy* indc, char* buf)
{
    int i = 0;

    ei_encode_version(buf, &i);
    ei_encode_tuple_header(buf, &i, 9);
    ei_encode_atom(buf, &i, "drop_copy");
    ei_encode_ulong(buf, &i, indc->symbol_id);
    ei_encode_ulong(buf, &i, indc->passive_party);
    ei_encode_ulong(buf, &i, indc->aggressor_party);
    ei_encode_ulong(buf, &i, indc->exec_size);
    ei_encode_ulong(buf, &i, indc->exec_price);
    ei_encode_ulonglong(buf, &i, indc->passive_order_id);
    ei_encode_ulonglong(buf, &i, indc->aggressor_order_id);
    ei_encode_ulonglong(buf, &i, indc->exec_ts);

    return i;
}

