#ifndef ERROR_CONTEXT_H
#define ERROR_CONTEXT_H

#include <QSharedDataPointer>
#include <QString>

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
