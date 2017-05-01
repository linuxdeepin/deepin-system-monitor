#ifndef HASHQSTRING_H
#define HASHQSTRING_H

#include <QString>
#include <string>

namespace std {
    template<> struct hash<QString>
    {
        size_t operator()(const QString& k) const
        {
            return hash<string>()(k.toStdString());
        }
    };
}

#endif // HASHQSTRING_H
