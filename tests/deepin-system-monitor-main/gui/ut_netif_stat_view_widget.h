#ifndef UT_NETIFSTATVIEWWIDGET_H
#define UT_NETIFSTATVIEWWIDGET_H

#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

class Ut_NetifStatViewWidget : public QObject, public::testing::Test
{
    Q_OBJECT
public:
    Ut_NetifStatViewWidget();
};

#endif // UT_NETIFSTATVIEWWIDGET_H
