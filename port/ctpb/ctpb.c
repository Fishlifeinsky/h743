#include "ctpb.h"

#include <stdlib.h>

//--------------------------------------------------------------------+
// 外部接口
//--------------------------------------------------------------------+

size_t ctpb_pack(ProtobufCMessage *msg, uint8_t **out) {
    size_t size = protobuf_c_message_get_packed_size(msg);
    if (size == 0) {
        *out = NULL;
        return 0;
    }
    *out = (uint8_t *)malloc(size);
    if (*out == NULL) return 0;
    return protobuf_c_message_pack(msg, *out);
}
