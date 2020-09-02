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

#ifndef COLLATOR_H
#define COLLATOR_H

#include <QString>

#include <unicode/coll.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace util {
namespace common {

class Collator
{
public:
    /**
    * @brief instance Singleton instance
    */
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

    /**
    * @brief compare Compare unicode strings with ICU API
    * @param left Unicode string to be compared
    * @param right Unicode string to be compared
    * @param caseIgnoreCompare Compare string with case or with case ignored
    * @return -1: left < right; 1: left > right; 0: left = right
    */
    int compare(const QString &left, const QString &right, bool caseIgnoreCompare = true) const;

private:
    Collator();
    ~Collator();

    // instance
    icu::Collator *m_collator;

    // multi thread safe
    static std::atomic<Collator *> m_instance;
    static std::mutex m_mutex;
}; // !Collator

} // namespace common
} // namespace util

#endif  // COLLATOR_H
