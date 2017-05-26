#ifndef NETWORKTOOLS_H
#define NETWORKTOOLS_H

namespace networkTools {
    typedef struct NetworkStatus {
        uint32_t sentBytes;
        uint32_t recvBytes;
        float sentKbs;
        float recvKbs;
    } NetworkStatus;
}

#endif // NETWORKTOOLS_H
