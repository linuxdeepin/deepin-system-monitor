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

#ifndef HAN_LATIN_H
#define HAN_LATIN_H

class QString;

/**
* @brief namespace util::common
*/
namespace util {
namespace common {

/**
* @brief convHanToLatin Convert chinese hanzi to ascii pinyin format
* @param words Chinese hanzi unicode string
* @return Converted ascii pinyin string
*/
QString convHanToLatin(const QString &words);

} // namespace common
} // namespace util

#endif  // HAN_LATIN_H
