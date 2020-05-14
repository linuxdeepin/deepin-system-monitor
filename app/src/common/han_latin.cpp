// Copyright (c) 2015 LiuLang. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include <QDebug>
#include <memory>

#include "unicode/parseerr.h"
#include "unicode/translit.h"
#include "unicode/utypes.h"

#include "han_latin.h"

#define TRANSLITERATION_HAN_LATIN "Han-Latin"
#define TRANSLITERATION_LATIN_ASCII "Latin-ASCII"

using namespace std;
using namespace icu;

namespace uconv {

static QString parseError(const QString &words, UErrorCode &ec, const UParseError &pe)
{
    QString errbuf {};

    std::string pre {}, post {};
#undef u_errorName
#define UERRORNAME U_DEF2_ICU_ENTRY_POINT_RENAME(u_errorName, U_ICU_VERSION_SUFFIX)
    errbuf = QString(
                 "Error[%1]: %2 convert %3 to PINYIN failed. [line:%4 offset:%5 "
                 "preContext: %6 postContext: %7]")
                 .arg(ec)
                 .arg(UERRORNAME(ec))
                 .arg(words)
                 .arg(pe.line)
                 .arg(pe.offset)
                 .arg(QString::fromStdString(UnicodeString(pe.preContext).toUTF8String(pre)))
                 .arg(QString::fromStdString(UnicodeString(pe.postContext).toUTF8String(post)));

    return errbuf;
}

QString convHanToLatin(const QString &words)
{
    QString result {};
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe {};
    unique_ptr<Transliterator> tr(Transliterator::createInstance(
        TRANSLITERATION_HAN_LATIN, UTransDirection::UTRANS_FORWARD, pe, ec));

    UnicodeString ubuf = UnicodeString::fromUTF8(StringPiece(words.toStdString()));
    // from han to latin
    tr->transliterate(ubuf);

    if (U_FAILURE(ec)) {
        qDebug() << parseError(words, ec, pe);
        result = words;
    } else {
        ec = U_ZERO_ERROR;
        pe = {};
        unique_ptr<Transliterator> tr2(Transliterator::createInstance(
            TRANSLITERATION_LATIN_ASCII, UTransDirection::UTRANS_FORWARD, pe, ec));
        // from latin to ascii (pinyin)
        tr2->transliterate(ubuf);

        if (U_FAILURE(ec)) {
            qDebug() << parseError(words, ec, pe);
            result = words;
        } else {
            std::string buffer;
            result = QString::fromStdString(ubuf.toUTF8String(buffer));
        }
    }

    return result;
}

}  // namespace uconv
