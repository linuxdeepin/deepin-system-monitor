/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "error_context.h"

#include <QString>

class ErrorContextData : public QSharedData
{
public:
    ErrorContextData() {}
    ErrorContextData(const ErrorContextData &rhs)
        : QSharedData(rhs)
        , m_code(rhs.m_code)
        , m_subCode(rhs.m_subCode)
        , m_errName(rhs.m_errName)
        , m_errMessage(rhs.m_errMessage)
    {
    }
    ErrorContextData &operator=(const ErrorContextData &rhs)
    {
        Q_UNUSED(padding);
        if (this != &rhs) {
            m_code = rhs.m_code;
            m_subCode = rhs.m_subCode;
            m_errName = rhs.m_errName;
            m_errMessage = rhs.m_errMessage;
        }
        return *this;
    }
    ~ErrorContextData() {}

    friend class ErrorContext;

private:
    int m_code {0};
    int m_subCode {0};
    int padding;  // 4bytes padding to avoid warning
    QString m_errName {};
    QString m_errMessage {};
};

ErrorContext::ErrorContext()
    : data(new ErrorContextData())
{
}

ErrorContext::ErrorContext(int code, int subCode, const QString &errName, const QString &errMessage)
{
    data = new ErrorContextData();
    data->m_code = code;
    data->m_subCode = subCode;
    data->m_errName = errName;
    data->m_errMessage = errMessage;
}
ErrorContext::ErrorContext(const ErrorContext &rhs)
    : data(rhs.data)
{
}

//重载=
ErrorContext &ErrorContext::operator=(const ErrorContext &rhs)
{
    if (this != &rhs) {
        data.operator = (rhs.data);
    }
    return *this;
}
ErrorContext::~ErrorContext() {}

//重载==
bool ErrorContext::operator==(const ErrorContext &other) const
{
    return (data->m_code == other.getCode() && data->m_subCode == other.getSubCode() &&
            data->m_errName == other.getSubCode() && data->m_errMessage == getErrorMessage());
}

int ErrorContext::getCode() const
{
    return data->m_code;
}
void ErrorContext::setCode(int code)
{
    data->m_code = code;
}

int ErrorContext::getSubCode() const
{
    return data->m_subCode;
}
void ErrorContext::setSubCode(int subCode)
{
    data->m_subCode = subCode;
}

//获取错误名
QString ErrorContext::getErrorName() const
{
    return data->m_errName;
}

//设置错误名
void ErrorContext::setErrorName(const QString &errName)
{
    data->m_errName = errName;
}

//获取错误信息
QString ErrorContext::getErrorMessage() const
{
    return data->m_errMessage;
}

//设置错误信息
void ErrorContext::setErrorMessage(const QString &errMessage)
{
    data->m_errMessage = errMessage;
}

bool operator!(const ErrorContext &ec) noexcept
{
    return !ec.operator bool();
}

//判断是否有效
bool ErrorContext::isValid() const
{
    return operator bool();
}

ErrorContext::operator bool() const
{
    return (data->m_code != 0 || data->m_subCode != 0);
}

//重置参数
void ErrorContext::reset()
{
    data->m_code = 0;
    data->m_subCode = 0;
    data->m_errName = "";
    data->m_errMessage = "";
}
