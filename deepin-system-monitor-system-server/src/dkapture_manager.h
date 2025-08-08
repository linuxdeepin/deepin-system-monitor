// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DKAPTURE_MANAGER_H
#define DKAPTURE_MANAGER_H

#include "dkapture.h"
#include <QScopedPointer>
#include <QLibrary>
#include <vector>

// Forward declaration
struct ProcPidStat;
struct ProcPidIo;
struct ProcPidStatm;
struct ProcPidStatus;
struct ProcPidCmdline;

class DKaptureManager
{
public:
    static DKaptureManager *instance();
    ~DKaptureManager();

    bool isAvailable() const;

    // Wrapper for DKapture APIs
    int open(FILE *fp = stdout, DKapture::LogLevel lvl = DKapture::DEBUG);
    unsigned long long lifetime(unsigned long long ms);
    ssize_t read(std::vector<DKapture::DataType> &dts, DKapture::DKCallback cb, void *ctx);
    int close();

private:
    DKaptureManager();

    DKaptureManager(const DKaptureManager &) = delete;
    DKaptureManager &operator=(const DKaptureManager &) = delete;

    void resolveSymbols();

    bool m_available;
    QScopedPointer<QLibrary> m_library;
    DKapture *m_dk_instance;

    // Function pointer for the static new_instance method
    typedef DKapture *(*new_instance_t)();
    new_instance_t m_new_instance_func;

    // The single instance
    static QScopedPointer<DKaptureManager> self;
};

#endif // DKAPTURE_MANAGER_H