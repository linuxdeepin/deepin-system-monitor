#include "dlscpu.h"

extern "C"{
    #include "../3rdparty/lscpu.h"
}

DLscpu::DLscpu()
{
    cxt = initCpuInfo();
}

DLscpu::~DLscpu()
{
    freeCpuInfo(cxt);
}

int DLscpu::cpuCount() const
{
    if(cxt){
        return static_cast<int>(cxt->npresents);
    }
    return 0;
}

int DLscpu::socketCount() const
{
    if(cxt && (*cxt->cputypes)){
        return static_cast<int>((*cxt->cputypes)->nsockets);
    }
    return 0;
}

QString DLscpu::curFreq() const
{
    if(cxt && (*cxt->cpus) && (*cxt->cpus)->mhz)
        return QString((*cxt->cpus)->mhz);
    return QString();
}

float DLscpu::minFreq() const
{
    if(cxt && (*cxt->cpus)){
        return (*cxt->cpus)->mhz_min_freq;
    }
    return 0;
}

float DLscpu::maxFreq() const
{
    if(cxt && (*cxt->cpus)){
        return (*cxt->cpus)->mhz_max_freq;
    }
    return 0;
}

QString DLscpu::l1dCache() const
{
    if(cxt && (*cxt->cpus)){
        lscpu_cache *cache = lscpu_cpu_get_cache(cxt, *cxt->cpus, "L1d");
        if(cache){
            return QString::number(cache->size/1024);
        }
    }
    return QString();
}

QString DLscpu::l1iCache() const
{
    if(cxt && (*cxt->cpus)){
        lscpu_cache *cache = lscpu_cpu_get_cache(cxt, *cxt->cpus, "L1i");
        if(cache){
            return QString::number(cache->size/1024);
        }
    }
    return QString();
}

QString DLscpu::l2Cache() const
{
    if(cxt && (*cxt->cpus)){
        lscpu_cache *cache = lscpu_cpu_get_cache(cxt, *cxt->cpus, "L2");
        if(cache){
            return QString::number(cache->size/1024);
        }
    }
    return QString();
}

QString DLscpu::l3Cache() const
{
    if(cxt && (*cxt->cpus)){
        lscpu_cache *cache = lscpu_cpu_get_cache(cxt, *cxt->cpus, "L3");
        if(cache){
            return QString::number(cache->size/1024);
        }
    }
    return QString();
}

QString DLscpu::vendorId() const
{
    if(cxt && (*cxt->cputypes) && (*cxt->cputypes)->vendor)
        return QString((*cxt->cputypes)->vendor);
    return QString();
}

QString DLscpu::modelName() const
{
    if(cxt && (*cxt->cputypes) && (*cxt->cputypes)->modelname)
        return QString((*cxt->cputypes)->modelname);
    return QString();
}

QString DLscpu::virtualization() const
{
    if(cxt && cxt->virt && cxt->virt->cpuflag){
        if (!strcmp(cxt->virt->cpuflag, "svm"))
            return "AMD-V";
        else if (!strcmp(cxt->virt->cpuflag, "vmx"))
            return "VT-x";
        else
            return QString(cxt->virt->cpuflag);
    }
    return QString();
}
