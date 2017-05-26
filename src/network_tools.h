#ifndef NETWORKTOOLS_H
#define NETWORKTOOLS_H

namespace networkTools {
    typedef struct networkStatus {
        uint32_t sentBytes;
        uint32_t recvBytes;
        float sentKbs;
        float recvKbs;
    } networkStatus;
}

#endif // NETWORKTOOLS_H
