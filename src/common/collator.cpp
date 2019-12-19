#include <QDebug>

#include "collator.h"

std::atomic<Collator *> Collator::m_instance;
std::mutex Collator::m_mutex;

int Collator::compare(const QString &left, const QString &right, bool caseIgnoreCompare) const
{
    icu::Collator::EComparisonResult result = icu::Collator::EQUAL;

    // if icu::Collator cannot be initialized, use builtin compare method (builtin method kinda
    // buggy right now)
    if (m_collator == nullptr)
        return QString::compare(left, right);

    icu::UnicodeString lbuf = icu::UnicodeString().setTo(left.utf16());
    icu::UnicodeString rbuf = icu::UnicodeString().setTo(right.utf16());
    icu::Collator::ECollationStrength cs = m_collator->getStrength();
    if (caseIgnoreCompare)
        m_collator->setStrength(icu::Collator::SECONDARY);
    else
        m_collator->setStrength(icu::Collator::TERTIARY);
    result = m_collator->compare(lbuf, rbuf);
    m_collator->setStrength(cs);
    if (result == icu::Collator::LESS) {
        return -1;
    } else if (result == icu::Collator::GREATER) {
        return 1;
    } else {
        return 0;
    }
}

Collator::Collator()
{
    UErrorCode ec = U_ZERO_ERROR;
    m_collator = icu::Collator::createInstance(ec);
    if (U_FAILURE(ec)) {
#undef u_errorName
#define UERRORNAME U_DEF2_ICU_ENTRY_POINT_RENAME(u_errorName, U_ICU_VERSION_SUFFIX)
        qDebug() << QString("Create collator failed: [%1] %2").arg(ec).arg(UERRORNAME(ec));
        m_collator = nullptr;
    }
}

Collator::~Collator()
{
    delete m_collator;
}
