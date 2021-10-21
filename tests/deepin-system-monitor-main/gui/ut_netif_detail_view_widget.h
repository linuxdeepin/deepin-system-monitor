#ifndef UT_NETIF_DETAIL_VIEW_WIDGET_H
#define UT_NETIF_DETAIL_VIEW_WIDGET_H


#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

class Ut_NetifDetailViewWidget : public QObject, public::testing::Test
{
    Q_OBJECT
public:
    Ut_NetifDetailViewWidget();
};

#endif // UT_NETIF_DETAIL_VIEW_WIDGET_H
