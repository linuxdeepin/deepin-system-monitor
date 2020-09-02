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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ERROR_CONTEXT_H
#define ERROR_CONTEXT_H

#include <QSharedDataPointer>

class QString;
class ErrorContextData;

/**
* @brief ErrorContext Error context to keep track of internal error
*/
class ErrorContext
{
public:
    enum ErrorCode { kErrorTypeNoError = 0, kErrorTypeCommon, kErrorTypeSystem, kErrorTypeDBus };

    /**
    * @brief Default constructor
    */
    ErrorContext();
    /**
    * @brief Constructor with params
    */
    ErrorContext(int code, int subCode, const QString &errName, const QString &errMessage);
    /**
    * @brief Copy constructor
    */
    ErrorContext(const ErrorContext &);
    /**
    * @brief Copy assignment
    */
    ErrorContext &operator=(const ErrorContext &);
    ~ErrorContext();

    /**
    * @brief operator== Compare with other error context
    * @return true: equal; false: not equal
    */
    bool operator==(const ErrorContext &other) const;
    /**
    * @brief getCode Get error code
    * @return Error code
    */
    int getCode() const;
    /**
    * @brief setCode Set error code
    * @param code Error code
    */
    void setCode(int code);

    /**
    * @brief getSubCode Get sub error code
    * @return Sub error code
    */
    int getSubCode() const;
    /**
    * @brief setSubCode Set sub error code
    * @param subCode Sub error code
    */
    void setSubCode(int subCode);

    /**
    * @brief getErrorName Get error name
    * @return Error name
    */
    QString getErrorName() const;
    /**
    * @brief setErrorName Set error name
    * @param errName Error name
    */
    void setErrorName(const QString &errName);

    /**
    * @brief getErrorMessage Get error description
    * @return Error description
    */
    QString getErrorMessage() const;
    /**
    * @brief setErrorMessage Set error description
    * @param errMessage Error description
    */
    void setErrorMessage(const QString &errMessage);

    /**
    * @brief isValid Check if it's a valid error context
    * @return true: valid error context (error occured); false: invalid error context (no error)
    */
    bool isValid() const;
    /**
    * @brief operator bool Bool operator for ErrorContext class
    */
    explicit operator bool() const;

    /**
    * @brief reset Reset error context to invalid state
    */
    void reset();

private:
    QSharedDataPointer<ErrorContextData> data;
};

/**
* @brief operator! Not operator for ErrorCotext
* @param ec Error context to be checked
* @return true: valid error context (error occured); false: invalid error context (no error)
*/
bool operator!(const ErrorContext &ec) noexcept;

#endif  // ERROR_CONTEXT_H
