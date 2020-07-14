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

#ifndef ERROR_CONTEXT_H
#define ERROR_CONTEXT_H

#include <QSharedDataPointer>

class QString;
class ErrorContextData;

class ErrorContext
{
public:
    enum ErrorCode { kErrorTypeNoError = 0, kErrorTypeCommon, kErrorTypeSystem, kErrorTypeDBus };

    ErrorContext();
    ErrorContext(int code, int subCode, const QString &errName, const QString &errMessage);
    ErrorContext(const ErrorContext &);
    ErrorContext &operator=(const ErrorContext &);
    ~ErrorContext();

    bool operator==(const ErrorContext &other) const;

    int getCode() const;
    void setCode(int code);

    int getSubCode() const;
    void setSubCode(int subCode);

    QString getErrorName() const;
    void setErrorName(const QString &errName);

    QString getErrorMessage() const;
    void setErrorMessage(const QString &errMessage);

    bool isValid() const;
    explicit operator bool() const;

    void reset();

private:
    QSharedDataPointer<ErrorContextData> data;
};

bool operator!(const ErrorContext &ec) noexcept;

#endif  // ERROR_CONTEXT_H
