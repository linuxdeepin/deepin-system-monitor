#ifndef UT_NETIFITEMVIEWWIDGET_H
#define UT_NETIFITEMVIEWWIDGET_H

#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

class Ut_NetifItemViewWidget : public QObject, public::testing::Test
{
    Q_OBJECT
public:
    Ut_NetifItemViewWidget();
};

#endif // UT_NETIFITEMVIEWWIDGET_H
