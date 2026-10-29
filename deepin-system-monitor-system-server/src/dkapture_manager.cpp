// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dkapture_manager.h"
#include "ddlog.h"

#include <QDebug>


QScopedPointer<DKaptureManager> DKaptureManager::self;

DKaptureManager *DKaptureManager::instance()
{
    if (self.isNull()) {
        self.reset(new DKaptureManager());
    }
    return self.data();
}

DKaptureManager::DKaptureManager()
    : m_available(false)
    , m_dk_instance(nullptr)
    , m_new_instance_func(nullptr)
{
    qCDebug(DDLog::app) << "Attempting to load libdkapture.so";
    m_library.reset(new QLibrary("libdkapture.so"));
    if (!m_library->load()) {
        qCWarning(DDLog::app) << "Failed to load libdkapture.so:" << m_library->errorString();
        return;
    }

    qCInfo(DDLog::app) << "try resolveSymbols.";
    resolveSymbols();

    if (m_new_instance_func) {
        qCInfo(DDLog::app) << "create dkapture instance by symbols.";
        m_dk_instance = m_new_instance_func();
    } else {
        qCInfo(DDLog::app) << "create dkapture instance by API.";
        return;
        // m_dk_instance = DKapture::new_instance();
    }

    if (m_dk_instance) {
        if (open() == 0) {
                m_available = true;
                static int dk_lifetime = 128; // ms
                lifetime(dk_lifetime);
                qCInfo(DDLog::app) << "Successfully loaded and initialized dkapture library.";
        } else {
            m_available = false;
            qCWarning(DDLog::app) << "Failed to open dkapture instance.";
            delete m_dk_instance;
            m_dk_instance = nullptr;
        }
    } else {
        qCWarning(DDLog::app) << "Failed to create dkapture instance.";
    }
}

DKaptureManager::~DKaptureManager()
{
    if (m_dk_instance) {
        close();
        delete m_dk_instance;
        m_dk_instance = nullptr;
    }
    if (m_library->isLoaded()) {
        m_library->unload();
    }
}

bool DKaptureManager::isAvailable() const
{
    return m_available;
}

void DKaptureManager::resolveSymbols()
{
    // Try various mangled names for DKapture::new_instance()
    QStringList symbolNames = {
        "_ZN8DKapture12new_instanceEv",     // Most common GCC mangling
        "ZN8DKapture12new_instanceEv",      // Alternative mangling
        "_ZN8DKapture12new_instanceE",      // Without void parameter
        "DKapture_new_instance",            // C-style export
        "_Z19DKapture_new_instancev"        // Another possible mangling
    };
    
    for (const QString &symbolName : symbolNames) {
        m_new_instance_func = (new_instance_t)m_library->resolve(symbolName.toLocal8Bit().data());
        if (m_new_instance_func) {
            qCInfo(DDLog::app) << "Successfully resolved DKapture symbol:" << symbolName;
            return;
        }
        qCDebug(DDLog::app) << "Failed to resolve symbol:" << symbolName;
    }
    
    qCWarning(DDLog::app) << "Could not resolve any DKapture::new_instance symbols";
    qCWarning(DDLog::app) << "Library error:" << m_library->errorString();
}

int DKaptureManager::open(FILE *fp, DKapture::LogLevel lvl)
{
    if (!m_dk_instance) return -1;
    return m_dk_instance->open(fp, lvl);
}

unsigned long long DKaptureManager::lifetime(unsigned long long ms)
{
    if (!m_dk_instance) return 0;
    return m_dk_instance->lifetime(ms);
}

ssize_t DKaptureManager::read(std::vector<DKapture::DataType> &dts, DKapture::DKCallback cb, void *ctx)
{
    if (!isAvailable()) return -1;
    return m_dk_instance->read(dts, cb, ctx);
}

int DKaptureManager::close()
{
    if (!m_dk_instance) return -1;
    return m_dk_instance->close();
}
