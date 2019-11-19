#include "process_entry.h"

class ProcessEntryData : public QSharedData
{
public:

};

ProcessEntry::ProcessEntry() : data(new ProcessEntryData)
{

}

ProcessEntry::ProcessEntry(const ProcessEntry &rhs) : data(rhs.data)
{

}

ProcessEntry &ProcessEntry::operator=(const ProcessEntry &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

ProcessEntry::~ProcessEntry()
{

}
