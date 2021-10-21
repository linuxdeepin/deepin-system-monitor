#ifndef UT_BLOCK_DEV_STAT_VIEW_WIDGET_H
#define UT_BLOCK_DEV_STAT_VIEW_WIDGET_H

#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

class Ut_BlockStatViewWidget: public QObject, public::testing::Test
{
    Q_OBJECT
public:
    Ut_BlockStatViewWidget();
};


#endif // UT_BLOCK_DEV_STAT_VIEW_WIDGET_H
