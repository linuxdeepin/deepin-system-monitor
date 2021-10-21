
#include "ut_animation_stackedwidget.h"
#include "animation_stackedwidget.h"
#include <QSignalSpy>
#include <QPainter>

Ut_AnimationStackedWidget::Ut_AnimationStackedWidget()
{

}

TEST(Ut_AnimationStackedWidget_AnimationStackedWidget, Ut_AnimationStackedWidget_AnimationStackedWidget_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    EXPECT_EQ(pAnimationStackedWidget->m_Duration, 250);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setDuration, Ut_AnimationStackedWidget_setDuration_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    pAnimationStackedWidget->setDuration(500);

    EXPECT_EQ(pAnimationStackedWidget->m_Duration, 500);
    EXPECT_EQ(pAnimationStackedWidget->m_Animation->easingCurve(), QEasingCurve::InOutQuad);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_IsRunning, Ut_AnimationStackedWidget_IsRunning_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    pAnimationStackedWidget->m_IsAnimation = false;
    bool  isRunning = pAnimationStackedWidget->IsRunning();

    EXPECT_FALSE(isRunning);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_IsRunning, Ut_AnimationStackedWidget_IsRunning_002)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    pAnimationStackedWidget->m_IsAnimation = true;
    bool isRunning = pAnimationStackedWidget->IsRunning();

    EXPECT_TRUE(isRunning);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_offset, Ut_AnimationStackedWidget_offset_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    double offset = pAnimationStackedWidget->offset();

    EXPECT_EQ(pAnimationStackedWidget->m_offset, offset);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setOffset, Ut_AnimationStackedWidget_setOffset_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    double offset = 10.0;
    pAnimationStackedWidget->setOffset(offset);

    EXPECT_EQ(pAnimationStackedWidget->m_offset, offset);
    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintEvent, Ut_AnimationStackedWidget_paintEvent_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    EXPECT_TRUE(!pAnimationStackedWidget->grab().isNull());
    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_animationFinished, Ut_AnimationStackedWidget_animationFinished_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);
    pAnimationStackedWidget->m_NextIndex = 0;
    QWidget *pAddWidget = new QWidget;
    pAnimationStackedWidget->addWidget(pAddWidget);
    pAnimationStackedWidget->animationFinished();

    EXPECT_FALSE(pAnimationStackedWidget->m_IsAnimation);

    EXPECT_FALSE(pAnimationStackedWidget->widget(pAnimationStackedWidget->currentIndex())->isHidden());

    EXPECT_EQ(pAnimationStackedWidget->currentIndex(), pAnimationStackedWidget->m_NextIndex);

    QSignalSpy signalSpy(pAnimationStackedWidget, &AnimationStackedWidget::signalIsFinished);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pAnimationStackedWidget->signalIsFinished();
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);
    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setCurrent, Ut_AnimationStackedWidget_setCurrent_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);
    pAnimationStackedWidget->setCurrent(0);

    int index = pAnimationStackedWidget->indexOf(pWidget1);

    auto moveOri = pAnimationStackedWidget->getMoveOrientation(pAnimationStackedWidget->currentIndex(), index);
    EXPECT_EQ(pAnimationStackedWidget->m_moveOri, moveOri);

    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setCurrent, Ut_AnimationStackedWidget_setCurrent_002)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    pAnimationStackedWidget->setCurrent(pWidget1);

    int index = pAnimationStackedWidget->indexOf(pWidget1);
    EXPECT_LE(index, 0);


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}


TEST(Ut_AnimationStackedWidget_setCurrent, Ut_AnimationStackedWidget_setCurrent_003)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    pAnimationStackedWidget->setCurrent(pWidget1);

    int index = pAnimationStackedWidget->indexOf(pWidget1);
    EXPECT_GE(index, 0);

    auto moveOri = pAnimationStackedWidget->getMoveOrientation(pAnimationStackedWidget->currentIndex(), index);
    EXPECT_EQ(pAnimationStackedWidget->m_moveOri, moveOri);

    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}


TEST(Ut_AnimationStackedWidget_setCurrentWidget, Ut_AnimationStackedWidget_setCurrentWidget_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    int index = 0;
    int beginWidth = 10;
    pAnimationStackedWidget->m_IsAnimation = true;
    pAnimationStackedWidget->setCurrentWidget(index, beginWidth);

    EXPECT_TRUE(pAnimationStackedWidget->m_IsAnimation);

    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setCurrentWidget, Ut_AnimationStackedWidget_setCurrentWidget_002)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    int index = 0;
    int beginWidth = 10;
    pAnimationStackedWidget->m_IsAnimation = false;
    pAnimationStackedWidget->setCurrentWidget(index, beginWidth);

    EXPECT_FALSE(pAnimationStackedWidget->m_IsAnimation);

    EXPECT_EQ(pAnimationStackedWidget->currentIndex(), index);

    QSignalSpy signalSpy(pAnimationStackedWidget, &AnimationStackedWidget::signalIsFinished);
    EXPECT_TRUE(signalSpy.count() == 0);

    emit pAnimationStackedWidget->signalIsFinished();
    signalSpy.wait(50);
    EXPECT_TRUE(signalSpy.count() == 1);

    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_setCurrentWidget, Ut_AnimationStackedWidget_setCurrentWidget_003)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    int index = 0;
    int beginWidth = 10;
    pAnimationStackedWidget->m_IsAnimation = false;
    pAnimationStackedWidget->setCurrentIndex(2);
    pAnimationStackedWidget->setCurrentWidget(index, beginWidth);

    EXPECT_NE(pAnimationStackedWidget->currentIndex(), index);

    EXPECT_TRUE(pAnimationStackedWidget->m_IsAnimation);

    EXPECT_EQ(pAnimationStackedWidget->m_NextIndex, index);

    EXPECT_FALSE(pAnimationStackedWidget->isVisible());

    EXPECT_EQ(pAnimationStackedWidget->m_Animation->startValue(), beginWidth);

    EXPECT_EQ(pAnimationStackedWidget->m_Animation->endValue(), 0);

    EXPECT_EQ(pAnimationStackedWidget->m_Animation->duration(), pAnimationStackedWidget->m_Duration);

    EXPECT_EQ(pAnimationStackedWidget->m_Animation->state(), QAbstractAnimation::Running);

    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getBeginValue, Ut_AnimationStackedWidget_getBeginValue_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    int value = pAnimationStackedWidget->getBeginValue();
    EXPECT_EQ(pAnimationStackedWidget->rect().width(), value);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getBeginValue, Ut_AnimationStackedWidget_getBeginValue_002)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::TB, pWidget);

    int value = pAnimationStackedWidget->getBeginValue();
    EXPECT_EQ(pAnimationStackedWidget->rect().height(), value);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getMoveOrientation, Ut_AnimationStackedWidget_getMoveOrientation_001)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    // case 1 currIndex < nextIndex
    int currIndex = 2;
    int nextIndex = 1;
    AnimationStackedWidget::MoveOrientation moveOri{AnimationStackedWidget::LeftToRight};

    moveOri = pAnimationStackedWidget->getMoveOrientation(currIndex, nextIndex);

    EXPECT_EQ(pAnimationStackedWidget->m_animationOri, AnimationStackedWidget::AnimationOri::LR);

    EXPECT_EQ(moveOri, AnimationStackedWidget::LeftToRight);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getMoveOrientation, Ut_AnimationStackedWidget_getMoveOrientation_002)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::TB, pWidget);

    // case 1 currIndex < nextIndex
    int currIndex = 2;
    int nextIndex = 1;
    AnimationStackedWidget::MoveOrientation moveOri{AnimationStackedWidget::LeftToRight};

    moveOri = pAnimationStackedWidget->getMoveOrientation(currIndex, nextIndex);

    EXPECT_EQ(pAnimationStackedWidget->m_animationOri, AnimationStackedWidget::AnimationOri::TB);

    EXPECT_EQ(moveOri, AnimationStackedWidget::TopToBottom);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getMoveOrientation, Ut_AnimationStackedWidget_getMoveOrientation_003)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    // case 1 currIndex > nextIndex
    int currIndex = 1;
    int nextIndex = 2;
    AnimationStackedWidget::MoveOrientation moveOri{AnimationStackedWidget::LeftToRight};

    moveOri = pAnimationStackedWidget->getMoveOrientation(currIndex, nextIndex);

    EXPECT_EQ(pAnimationStackedWidget->m_animationOri, AnimationStackedWidget::AnimationOri::LR);

    EXPECT_EQ(moveOri, AnimationStackedWidget::RightToLeft);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_getMoveOrientation, Ut_AnimationStackedWidget_getMoveOrientation_004)
{
    QWidget *pWidget = new QWidget;
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::TB, pWidget);

    // case 1 currIndex > nextIndex
    int currIndex = 1;
    int nextIndex = 2;
    AnimationStackedWidget::MoveOrientation moveOri{AnimationStackedWidget::LeftToRight};

    moveOri = pAnimationStackedWidget->getMoveOrientation(currIndex, nextIndex);

    EXPECT_EQ(pAnimationStackedWidget->m_animationOri, AnimationStackedWidget::AnimationOri::TB);

    EXPECT_EQ(moveOri, AnimationStackedWidget::BottomToTop);

    pWidget->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintCurrentWidget, Ut_AnimationStackedWidget_paintCurrentWidget_001)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::LeftToRight;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintCurrentWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintCurrentWidget, Ut_AnimationStackedWidget_paintCurrentWidget_002)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::RightToLeft;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintCurrentWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintCurrentWidget, Ut_AnimationStackedWidget_paintCurrentWidget_003)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::TopToBottom;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintCurrentWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}


TEST(Ut_AnimationStackedWidget_paintCurrentWidget, Ut_AnimationStackedWidget_paintCurrentWidget_004)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::BottomToTop;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintCurrentWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}


TEST(Ut_AnimationStackedWidget_paintNextWidget, Ut_AnimationStackedWidget_paintNextWidget_001)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::LeftToRight;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintNextWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintNextWidget, Ut_AnimationStackedWidget_paintNextWidget_002)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::RightToLeft;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintNextWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintNextWidget, Ut_AnimationStackedWidget_paintNextWidget_003)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::TopToBottom;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintNextWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}

TEST(Ut_AnimationStackedWidget_paintNextWidget, Ut_AnimationStackedWidget_paintNextWidget_004)
{
    QWidget *pWidget = new QWidget;
    pWidget->setFixedSize(100, 100);
    AnimationStackedWidget *pAnimationStackedWidget = new AnimationStackedWidget(AnimationStackedWidget::AnimationOri::LR, pWidget);

    QWidget *pWidget1 = new QWidget;
    QWidget *pWidget2 = new QWidget;
    QWidget *pWidget3 = new QWidget;
    pAnimationStackedWidget->addWidget(pWidget1);
    pAnimationStackedWidget->addWidget(pWidget2);
    pAnimationStackedWidget->addWidget(pWidget3);

    QPixmap pix;
    pix.fill(Qt::red);
    QPainter painter;
    painter.drawPixmap(0, 0, pix);
    int currIndex = 0;
    pAnimationStackedWidget->m_moveOri = AnimationStackedWidget::BottomToTop;
    pAnimationStackedWidget->m_offset = 1;
    pAnimationStackedWidget->widget(currIndex);

    pAnimationStackedWidget->paintNextWidget(painter, currIndex);

    EXPECT_TRUE(pAnimationStackedWidget->widget(currIndex));


    pWidget->deleteLater();
    pWidget1->deleteLater();
    pWidget2->deleteLater();
    pWidget3->deleteLater();
    pAnimationStackedWidget->deleteLater();
}
