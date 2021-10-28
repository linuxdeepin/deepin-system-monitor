//#include "application.h"
//#include "stub.h"

//#include <QObject>
//#include <QTest>

//#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>

////实现内存对齐
//class Ut_application : public QObject, public::testing::Test
//{
//public:
//    Ut_application() {}
//};

//TEST(UT_Application_setCurrentTaskState,UT_Application_setCurrentTaskState_001)
//{
//    int argc = 1;
//    char *argv[2] = {"one", "two"};
//    Application *pApplication = new Application(argc,argv);

//    Application::TaskState pTaskState = Application::kTaskStarted;
//    pApplication->setCurrentTaskState(pTaskState);
//    EXPECT_EQ(pApplication->m_currentTaskStat,pTaskState);

//    pApplication->deleteLater();
//}

//TEST(UT_Application_getCurrentTaskState,UT_Application_getCurrentTaskState_001)
//{
//    int argc = 1;
//    char *argv[2] = {"one", "two"};
//    Application *pApplication = new Application(argc,argv);

//    EXPECT_EQ(pApplication->getCurrentTaskState(),pApplication->m_currentTaskStat);

//    pApplication->deleteLater();
//}

//TEST(UT_Application_event,UT_Application_event_001)
//{
//    int argc = 1;
//    char *argv[2] = {"one", "two"};
//    Application *pApplication = new Application(argc,argv);

//    EXPECT_TRUE(kMonitorStartEventType);
//    pApplication->deleteLater();
//}

//TEST(UT_Application_event,UT_Application_event_002)
//{
//    int argc = 1;
//    char *argv[2] = {"one", "two"};
//    Application *pApplication = new Application(argc,argv);

//    EXPECT_TRUE(kNetifStartEventType);
//    pApplication->deleteLater();
//}
