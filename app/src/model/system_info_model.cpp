#include <QCoreApplication>

#include "system_info_model.h"

#define TR_CONTEXT          "System.Info.Dialog"
#define _tr(m)              QCoreApplication::translate(TR_CONTEXT, (m))

// kernel log level
#define TR_TEXT_EMERGENCY   QT_TRANSLATE_NOOP("System.Info.Dialog", "Emergency")
#define TR_TEXT_ALERT       QT_TRANSLATE_NOOP("System.Info.Dialog", "Alert")
#define TR_TEXT_CRITICAL    QT_TRANSLATE_NOOP("System.Info.Dialog", "Critical")
#define TR_TEXT_ERROR       QT_TRANSLATE_NOOP("System.Info.Dialog", "Error")
#define TR_TEXT_WARNING     QT_TRANSLATE_NOOP("System.Info.Dialog", "Warning")
#define TR_TEXT_NOTICE      QT_TRANSLATE_NOOP("System.Info.Dialog", "Notice")
#define TR_TEXT_INFO        QT_TRANSLATE_NOOP("System.Info.Dialog", "Info")
#define TR_TEXT_DEBUG       QT_TRANSLATE_NOOP("System.Info.Dialog", "Debug")

// clear cache depth
#define TR_TEXT_NO_CLEAR            QT_TRANSLATE_NOOP("System.Info.Dialog", "Do not clear")
#define TR_TEXT_CLEAR_PAGE_CACHE    QT_TRANSLATE_NOOP("System.Info.Dialog", "Clear page cache")
#define TR_TEXT_CLEAR_INODES        QT_TRANSLATE_NOOP("System.Info.Dialog", "Clear dentries and inodes")
#define TR_TEXT_CLEAR_ALL           QT_TRANSLATE_NOOP("System.Info.Dialog", "Clear all caches")

SystemInfoModel::SystemInfoModel(QObject *parent) : QObject(parent)
{

    m_logLevelModel = new QStandardItemModel(this);
    auto *emergencyItem = new QStandardItem(_tr(TR_TEXT_EMERGENCY));
    m_logLevelModel->appendRow(emergencyItem);
    auto *alertItem = new QStandardItem(_tr(TR_TEXT_ALERT));
    m_logLevelModel->appendRow(alertItem);
    auto *criticalItem = new QStandardItem(_tr(TR_TEXT_CRITICAL));
    m_logLevelModel->appendRow(criticalItem);
    auto *errorItem = new QStandardItem(_tr(TR_TEXT_ERROR));
    m_logLevelModel->appendRow(errorItem);
    auto *warningItem = new QStandardItem(_tr(TR_TEXT_WARNING));
    m_logLevelModel->appendRow(warningItem);
    auto *noticeItem = new QStandardItem(_tr(TR_TEXT_NOTICE));
    m_logLevelModel->appendRow(noticeItem);
    auto *infoItem = new QStandardItem(_tr(TR_TEXT_INFO));
    m_logLevelModel->appendRow(infoItem);
    auto *debugItem = new QStandardItem(_tr(TR_TEXT_DEBUG));
    m_logLevelModel->appendRow(debugItem);

    m_dropCacheModel = new QStandardItemModel(this);
    auto *noClearItem = new QStandardItem(_tr(TR_TEXT_NO_CLEAR));
    m_dropCacheModel->appendRow(noClearItem);
    auto *clearPageCacheItem = new QStandardItem(_tr(TR_TEXT_CLEAR_PAGE_CACHE));
    m_dropCacheModel->appendRow(clearPageCacheItem);
    auto *clearInodesItem = new QStandardItem(_tr(TR_TEXT_CLEAR_INODES));
    m_dropCacheModel->appendRow(clearInodesItem);
    auto *clearAllItem = new QStandardItem(_tr(TR_TEXT_CLEAR_ALL));
    m_dropCacheModel->appendRow(clearAllItem);
}
