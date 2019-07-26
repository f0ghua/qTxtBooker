#ifndef FILEROTATESTRAGERY_H
#define FILEROTATESTRAGERY_H

#include <QFile>

class FileRotationStrategy
{
public:
    virtual ~FileRotationStrategy() noexcept;

    virtual void setInitialInfo(const QFile &file) = 0;
    virtual void includeMessageInCalculation(const QString &message) = 0;
    virtual void includeMessageInCalculation(const QByteArray &message) = 0;
    virtual bool shouldRotate() = 0;
    virtual void rotate() = 0;
    virtual QIODevice::OpenMode recommendedOpenModeFlag() = 0;
};

class FileNullRotationStrategy : public FileRotationStrategy
{
public:
    void setInitialInfo(const QFile &) override {}
    void includeMessageInCalculation(const QString &) override {}
    void includeMessageInCalculation(const QByteArray &) override {}
    bool shouldRotate() override
    {
        return false;
    }
    void rotate() override {}
    QIODevice::OpenMode recommendedOpenModeFlag() override
    {
        return QIODevice::Truncate;
    }
};

class FileSizeRotationStrategy : public FileRotationStrategy
{
public:
    FileSizeRotationStrategy() = default;
    static const int MaxBackupCount;

    void setInitialInfo(const QFile &file) override;
    void includeMessageInCalculation(const QString &message) override;
    void includeMessageInCalculation(const QByteArray &message) override;
    bool shouldRotate() override;
    void rotate() override;
    QIODevice::OpenMode recommendedOpenModeFlag() override;

    void setMaximumSizeInBytes(qint64 size);
    void setBackupCount(int backups);

protected:
    // can be overridden for testing
    virtual bool removeFileAtPath(const QString &path);
    virtual bool fileExistsAtPath(const QString &path);
    virtual bool renameFileFromTo(const QString &from, const QString &to);

private:
    QString m_fileName;
    qint64 m_currentSizeInBytes{0};
    qint64 m_maxSizeInBytes{0};
    int m_backupsCount{0};
};

#endif // FILEROTATESTRAGERY_H
