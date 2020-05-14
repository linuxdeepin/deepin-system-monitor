#ifndef COLLATOR_H
#define COLLATOR_H

#include <unicode/coleitr.h>
#include <unicode/coll.h>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include <QString>

class Collator
{
public:
    inline static Collator *instance()
    {
        Collator *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            sin = m_instance.load();
            if (!sin) {
                sin = new Collator();
                m_instance.store(sin);
            }
        }
        return sin;
    }

    int compare(const QString &left, const QString &right, bool caseIgnoreCompare = true) const;

private:
    Collator();
    ~Collator();

    icu::Collator *m_collator;

    static std::atomic<Collator *> m_instance;
    static std::mutex m_mutex;
};

#endif  // COLLATOR_H
