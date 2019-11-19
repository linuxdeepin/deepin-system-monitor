#ifndef PROCESS_ENTRY_H
#define PROCESS_ENTRY_H

#include <QSharedDataPointer>

class ProcessEntryData;

class ProcessEntry
{
public:
    ProcessEntry();
    ProcessEntry(const ProcessEntry &);
    ProcessEntry &operator=(const ProcessEntry &);
    ~ProcessEntry();

private:
    QSharedDataPointer<ProcessEntryData> data;
};

#endif // PROCESS_ENTRY_H