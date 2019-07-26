#include "filerotationstrategy.h"

#include <QDebug>

FileRotationStrategy::~FileRotationStrategy() noexcept = default;

void FileSizeRotationStrategy::setInitialInfo(const QFile &file)
{
    m_fileName = file.fileName();
    m_currentSizeInBytes = file.size();
}

void FileSizeRotationStrategy::includeMessageInCalculation(const QString &message)
{
    includeMessageInCalculation(message.toUtf8());
}

void FileSizeRotationStrategy::includeMessageInCalculation(const QByteArray &message)
{
    m_currentSizeInBytes += message.size();
}

bool FileSizeRotationStrategy::shouldRotate()
{
    return m_currentSizeInBytes > m_maxSizeInBytes;
}

// Algorithm assumes backups will be named filename.X, where 1 <= X <= m_backupCount.
// All X's will be shifted up.
void FileSizeRotationStrategy::rotate()
{
    if (!m_backupsCount) {
        if (!removeFileAtPath(m_fileName)) {
            qDebug() << "QsLog: backup delete failed " << qPrintable(m_fileName);
        }
        return;
    }

     // 1. find the last existing backup than can be shifted up
     const QString logNamePattern = m_fileName + QString::fromUtf8(".%1");
     int lastExistingBackupIndex = 0;
     for (int i = 1;i <= m_backupsCount;++i) {
         const QString backupFileName = logNamePattern.arg(i);
         if (fileExistsAtPath(backupFileName)) {
             lastExistingBackupIndex = qMin(i, m_backupsCount - 1);
         } else {
             break;
         }
     }

     // 2. shift up
     for (int i = lastExistingBackupIndex;i >= 1;--i) {
         const QString oldName = logNamePattern.arg(i);
         const QString newName = logNamePattern.arg(i + 1);
         removeFileAtPath(newName);
         const bool renamed = renameFileFromTo(oldName, newName);
         if (!renamed) {
             qDebug() << "QsLog: could not rename backup " << qPrintable(oldName)
                       << " to " << qPrintable(newName);
         }
     }

     // 3. rename current log file
     const QString newName = logNamePattern.arg(1);
     if (fileExistsAtPath(newName)) {
         removeFileAtPath(newName);
     }
     if (!renameFileFromTo(m_fileName, newName)) {
         qDebug() << "QsLog: could not rename log " << qPrintable(m_fileName)
                   << " to " << qPrintable(newName);
     }
}

QIODevice::OpenMode FileSizeRotationStrategy::recommendedOpenModeFlag()
{
    return QIODevice::Append;
}

void FileSizeRotationStrategy::setMaximumSizeInBytes(qint64 size)
{
    Q_ASSERT(size >= 0);
    m_maxSizeInBytes = size;
}

void FileSizeRotationStrategy::setBackupCount(int backups)
{
    Q_ASSERT(backups >= 0);
    m_backupsCount = backups;
}

bool FileSizeRotationStrategy::removeFileAtPath(const QString &path)
{
    return QFile::remove(path);
}

bool FileSizeRotationStrategy::fileExistsAtPath(const QString &path)
{
    return QFile::exists(path);
}

bool FileSizeRotationStrategy::renameFileFromTo(const QString &from, const QString &to)
{
    return QFile::rename(from, to);
}
