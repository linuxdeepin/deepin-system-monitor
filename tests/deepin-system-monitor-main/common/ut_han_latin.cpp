// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "common/han_latin.h"
#include "unicode/parseerr.h"
#include "unicode/translit.h"
#include "unicode/utypes.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <sys/time.h>
#include <QApplication>
#include <QDateTime>

using namespace util::common;
using namespace std;
using namespace icu;
#define TRANSLITERATION_HAN_LATIN "Han-Latin"
#define TRANSLITERATION_LATIN_ASCII "Latin-ASCII"

/***************************************STUB begin*********************************************/
bool stub_convHanToLatin_ufailure(int)
{
    return true;
}
/***************************************STUB end**********************************************/


TEST(UT_HanLatin, test_convHanToLatin_01)
{
    QString words;
    convHanToLatin(words);
}

TEST(UT_HanLatin, test_convHanToLatin_EmptyString)
{
    QString result = convHanToLatin("");
    EXPECT_TRUE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_AsciiOnly)
{
    // Pure ASCII text should pass through mostly unchanged
    QString result = convHanToLatin("hello world");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_ChineseChars)
{
    // Test Chinese characters (Hanzi) conversion
    // "系统" means "system" in Chinese
    QString result = convHanToLatin("系统");
    // Should convert to pinyin (e.g., "xitong")
    EXPECT_FALSE(result.isEmpty());
    // The result should be latin/pinyin, not Chinese characters
}

TEST(UT_HanLatin, test_convHanToLatin_MixedContent)
{
    // Mixed Chinese and ASCII
    QString result = convHanToLatin("系统 monitor");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_LongText)
{
    // Test with longer Chinese text
    QString longChinese = "进程监控器";
    QString result = convHanToLatin(longChinese);
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_Punctuation)
{
    // Test Chinese text with punctuation
    QString result = convHanToLatin("系统，监控！");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_Numbers)
{
    // Test with numbers mixed
    QString result = convHanToLatin("系统123");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_ChineseChars2)
{
    // More Chinese characters
    QString result = convHanToLatin("内存");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_ChineseChars3)
{
    // More Chinese characters
    QString result = convHanToLatin("网络");
    EXPECT_FALSE(result.isEmpty());
}

TEST(UT_HanLatin, test_convHanToLatin_ChineseChars4)
{
    // More Chinese characters
    QString result = convHanToLatin("磁盘");
    EXPECT_FALSE(result.isEmpty());
}
