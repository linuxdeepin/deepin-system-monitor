#ifndef DLSCPU_H
#define DLSCPU_H

#include <QString>

struct lscpu_cxt;
class DLscpu
{

public:
    //Init
    DLscpu();
    ~DLscpu();

    //CPU Count
    int cpuCount() const;

    //Socket Count
    int socketCount() const;

    //CPU MHz
    QString curFreq() const;

    //CPU min MHz
    float minFreq() const;

    //CPU max MHz
    float maxFreq() const;

    //L1d cache
    QString l1dCache() const;

    //L1i cache
    QString l1iCache() const;

    //L2 cache
    QString l2Cache() const;

    //L3 cache
    QString l3Cache() const;

    //Vendor ID
    QString vendorId() const;

    //Model name
    QString modelName() const;

    //Virtualization
    QString virtualization() const;

private:
    lscpu_cxt *cxt = nullptr;
};

#endif // DLSCPU_H
