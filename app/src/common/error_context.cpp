#include "error_context.h"

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

ErrorContext &ErrorContext::operator=(const ErrorContext &rhs)
{
    if (this != &rhs) {
        data.operator=(rhs.data);
    }
    return *this;
}
ErrorContext::~ErrorContext() {}

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

QString ErrorContext::getErrorName() const
{
    return data->m_errName;
}
void ErrorContext::setErrorName(const QString &errName)
{
    data->m_errName = errName;
}

QString ErrorContext::getErrorMessage() const
{
    return data->m_errMessage;
}
void ErrorContext::setErrorMessage(const QString &errMessage)
{
    data->m_errMessage = errMessage;
}

bool operator!(const ErrorContext &ec) noexcept
{
    return !ec.operator bool();
}

bool ErrorContext::isValid() const
{
    return operator bool();
}

ErrorContext::operator bool() const
{
    return (data->m_code != 0 || data->m_subCode != 0);
}

void ErrorContext::reset()
{
    data->m_code = 0;
    data->m_subCode = 0;
    data->m_errName = "";
    data->m_errMessage = "";
}
