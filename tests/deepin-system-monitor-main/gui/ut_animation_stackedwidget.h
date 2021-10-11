#ifndef UT_ANIMATION_STACKEDWIDGET_H
#define UT_ANIMATION_STACKEDWIDGET_H

#include "stub.h"

#include <QObject>
#include <QTest>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

class Ut_AnimationStackedWidget : public QObject, public::testing::Test
{
    Q_OBJECT
public:
    Ut_AnimationStackedWidget();
};

#endif // UT_ANIMATION_STACKEDWIDGET_H
