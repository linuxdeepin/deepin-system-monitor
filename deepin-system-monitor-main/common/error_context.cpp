// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "error_context.h"
#include "ddlog.h"

#include <QString>

using namespace DDLog;

class ErrorContextData : public QSharedData
{
public:
    // default constructor
    ErrorContextData() {
        qCDebug(app) << "ErrorContextData default constructor";
    }
    // copy constructor
    ErrorContextData(const ErrorContextData &rhs)
        : QSharedData(rhs)
        , __alignment(rhs.__alignment)
        , m_code(rhs.m_code)
        , m_subCode(rhs.m_subCode)
        , m_errName(rhs.m_errName)
        , m_errMessage(rhs.m_errMessage)
    {
        qCDebug(app) << "ErrorContextData copy constructor";
    }
    // copy assignment
    ErrorContextData &operator=(const ErrorContextData &rhs)
    {
        // qCDebug(app) << "ErrorContextData copy assignment";
        if (this != &rhs) {
            // qCDebug(app) << "copying data";
            m_code = rhs.m_code;
            m_subCode = rhs.m_subCode;
            m_errName = rhs.m_errName;
            m_errMessage = rhs.m_errMessage;
        }
        return *this;
    }
    ~ErrorContextData() {
        // qCDebug(app) << "ErrorContextData destructor";
    }

    friend class ErrorContext;

private:
    int __alignment {0};
    // error code
    int m_code {0};
    // sub error code
    int m_subCode {0};
    // error name
    QString m_errName {};
    // error detail description
    QString m_errMessage {};
};

// default constructor
ErrorContext::ErrorContext(QObject* parent)
    : QObject (parent)
    , data(new ErrorContextData())
{
    qCDebug(app) << "ErrorContext default constructor";
}

// overloaded constructor
ErrorContext::ErrorContext(int code, int subCode, const QString &errName, const QString &errMessage, QObject* parent)
    : QObject (parent)
    , data(new ErrorContextData())
{
    qCDebug(app) << "ErrorContext overloaded constructor";
    // data = new ErrorContextData();
    data->m_code = code;
    data->m_subCode = subCode;
    data->m_errName = errName;
    data->m_errMessage = errMessage;
}
// copy constructor
ErrorContext::ErrorContext(const ErrorContext &rhs)
    : data(rhs.data)
{
    qCDebug(app) << "ErrorContext copy constructor";
}
// assignment operator
ErrorContext &ErrorContext::operator=(const ErrorContext &rhs)
{
    // qCDebug(app) << "ErrorContext assignment operator";
    if (this != &rhs) {
        // qCDebug(app) << "assigning data";
        data.operator = (rhs.data);
    }
    return *this;
}
ErrorContext::~ErrorContext() {
    // qCDebug(app) << "ErrorContext destructor";
}

// overloaded compare operator
bool ErrorContext::operator==(const ErrorContext &other) const
{
    // qCDebug(app) << "ErrorContext operator==";
    return (data->m_code == other.getCode() && data->m_subCode == other.getSubCode() &&
            data->m_errName == other.getErrorName() && data->m_errMessage == getErrorMessage());
}

// get error code
int ErrorContext::getCode() const
{
    // qCDebug(app) << "ErrorContext getCode:" << data->m_code;
    return data->m_code;
}
// set error code
void ErrorContext::setCode(int code)
{
    // qCDebug(app) << "ErrorContext setCode:" << code;
    data->m_code = code;
}

// get sub error code
int ErrorContext::getSubCode() const
{
    // qCDebug(app) << "ErrorContext getSubCode:" << data->m_subCode;
    return data->m_subCode;
}
// set sub error code
void ErrorContext::setSubCode(int subCode)
{
    // qCDebug(app) << "ErrorContext setSubCode:" << subCode;
    data->m_subCode = subCode;
}

// get error name
QString ErrorContext::getErrorName() const
{
    // qCDebug(app) << "ErrorContext getErrorName:" << data->m_errName;
    return data->m_errName;
}

// set error name
void ErrorContext::setErrorName(const QString &errName)
{
    // qCDebug(app) << "ErrorContext setErrorName:" << errName;
    data->m_errName = errName;
}

// get error message
QString ErrorContext::getErrorMessage() const
{
    // qCDebug(app) << "ErrorContext getErrorMessage:" << data->m_errMessage;
    return data->m_errMessage;
}

// set error message
void ErrorContext::setErrorMessage(const QString &errMessage)
{
    // qCDebug(app) << "ErrorContext setErrorMessage:" << errMessage;
    data->m_errMessage = errMessage;
}

// convenient ! operator
bool operator!(const ErrorContext &ec) noexcept
{
    // qCDebug(app) << "ErrorContext operator!";
    return !ec.operator bool();
}

// check if it's a valid error context
bool ErrorContext::isValid() const
{
    // qCDebug(app) << "ErrorContext isValid";
    return operator bool();
}

// bool operator
ErrorContext::operator bool() const
{
    // qCDebug(app) << "ErrorContext operator bool";
    return (data->m_code != 0 || data->m_subCode != 0);
}

// reset error context
void ErrorContext::reset()
{
    // qCDebug(app) << "ErrorContext reset";
    data->m_code = 0;
    data->m_subCode = 0;
    data->m_errName = "";
    data->m_errMessage = "";
}
