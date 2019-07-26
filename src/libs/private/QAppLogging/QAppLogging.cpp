#include "QAppLogging.h"
#include "filerotationstrategy.h"
#include "windows.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QCoreApplication>
#include <QTextCodec>

#define LOG_FILE_SIZE           (256*1024*1024)
#define LOG_INTKEY              "appCore"

QAPP_LOGGING_CATEGORY(AppCore,            LOG_INTKEY)
QAPP_LOGGING_CATEGORY(AppCoreTrace,       LOG_INTKEY QAL_TAG_TRACE)

QAtomicPointer<QAppLogging> QAppLogging::s_instance = 0;

static QtMessageHandler g_oldMsgHandle;

static void msgHandler(QtMsgType type,
                    const QMessageLogContext &context,
                    const QString &message)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    QAppLogging *appLogging = QAppLogging::instance();
    QString logMessage;
    do {
        int destOption = appLogging->outputDest();
        if (destOption == QAppLogging::eDestNone) {
            break;
        }

        logMessage = qFormatLogMessage(type, context, message);
        logMessage.append(QLatin1Char('\n'));

        if (destOption & QAppLogging::eDestSystem) {
            OutputDebugString(reinterpret_cast<const wchar_t *>(logMessage.utf16()));
        }

        if (destOption & QAppLogging::eDestFile) {
            appLogging->writeLogFile(logMessage);
            /*
            QFile *logFile = appLogging->logFile();
            if (logFile) {
                QTextStream stream(logFile);
                stream << qPrintable(logMessage);
                stream.flush();
            }
            */
        }
    } while(0);

    switch (type) {
    case QtFatalMsg:
        abort();
        break;
    case QtWarningMsg:
//        if (logMessage.contains("Cannot create children")) {
//            asm("nop");
//        }
        break;
    default:
        break;
    }
}

/*!
 * \brief QAppLogging::QAppLogging
 *
 * There are several file rotate strategies, we default use size rotate one
 *
 * - NullRotateStrategy: no file rotated and no message rotated, all messages
 * are logged, new file name constructed with each create timestamp
 *
 * - NeverRotateStrategy: no file rotated, message rotated, there is only 1
 * file, old messages are overwritten
 *
 * - SizeRotateStrategy: file rotated and no message rotated, filename
 * construct with the first timestamp and indexes.
 *
 * \return
 */
QAppLogging::QAppLogging()
    : m_outputDest(eDestSystem)
    , m_logFileDir()
    , m_logFileName()
    , m_maxFileSize(LOG_FILE_SIZE)
{
    m_logFile = new QFile();
    m_logStream = new QTextStream();

    FileSizeRotationStrategy *strategy = new FileSizeRotationStrategy();
    strategy->setMaximumSizeInBytes(m_maxFileSize);
    strategy->setBackupCount(3);

    //FileNullRotationStrategy *strategy = new FileNullRotationStrategy();
    m_fileRotationStrategy = strategy;
}

void QAppLogging::installHandler()
{
    g_oldMsgHandle = qInstallMessageHandler(msgHandler);
    qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} - %{message}");
}

bool QAppLogging::createLogFile()
{
    bool ret = false;

    QDateTime dtmCur = QDateTime::currentDateTime();
    QCoreApplication * app = QCoreApplication::instance();
    QString logDir = m_logFileDir;
    QString logFileName = m_logFileName;
    if (logDir.isEmpty()) {
        logDir = app->applicationDirPath() + "/log/" + dtmCur.toString("yyyy_MM");
    }
    QString currLogFilePath = logDir + "/";
    QDir dir;
    dir.mkpath(currLogFilePath);
    if (logFileName.isEmpty()) {
        logFileName += dtmCur.toString("yyyyMMdd_HHmmss_");
        logFileName += app->applicationName() + QString("(%1).txt").arg(app->applicationPid());
    } else {
        logFileName = dtmCur.toString("yyyyMMdd_HHmmss_");
        logFileName += m_logFileName;
    }
    currLogFilePath += logFileName;

    if (m_logFile->isOpen()==true) {
        m_logFile->close();
    }

    m_logFile->setFileName(currLogFilePath);
    if (m_logFile->open(QIODevice::WriteOnly) == false) {
        qDebug() << QObject::tr("open file %1 failed").arg(currLogFilePath);
        m_logStream->setDevice(nullptr);
    } else {
        m_logStream->setDevice(m_logFile);
        m_logStream->setCodec(QTextCodec::codecForName("UTF-8"));

        m_fileRotationStrategy->setInitialInfo(*m_logFile);
        ret = true;
    }

    return ret;
}

void QAppLogging::setLogFilePath(const QString &fileName, const QString &fileDir)
{
    setLogFileDir(fileDir);
    setLogFileName(fileName);
}

void QAppLogging::setLogFileMaxSize(const quint64 fileSize)
{
    m_maxFileSize = fileSize;
    static_cast<FileSizeRotationStrategy *>(m_fileRotationStrategy)->setMaximumSizeInBytes(m_maxFileSize);
}

void QAppLogging::setLogFileBackupCount(const int count)
{
    static_cast<FileSizeRotationStrategy *>(m_fileRotationStrategy)->setBackupCount(count);
}

void QAppLogging::setOutputDest(int value)
{
    m_outputDest = value;
}

void QAppLogging::writeLogFile(const QString &message)
{
    if (!m_logFile->isOpen()) {
        if (false == createLogFile()) {
            return;
        }
    }

    const QByteArray utf8Message = message.toUtf8();
    m_fileRotationStrategy->includeMessageInCalculation(utf8Message);
    if (m_fileRotationStrategy->shouldRotate()) {
        m_logStream->setDevice(nullptr);
        m_logFile->close();
        m_fileRotationStrategy->rotate();
        if (!m_logFile->open(QFile::WriteOnly | QFile::Text | m_fileRotationStrategy->recommendedOpenModeFlag())) {
            qDebug() << "QsLog: could not reopen log file " << qPrintable(m_logFile->fileName());
        }
        m_fileRotationStrategy->setInitialInfo(*m_logFile);
        m_logStream->setDevice(m_logFile);
        m_logStream->setCodec(QTextCodec::codecForName("UTF-8"));
    }

    *m_logStream << utf8Message; //<< endl;
    m_logStream->flush();
}

void QAppLogging::registerCategory(const char *category, QtMsgType severityLevel)
{
    Q_UNUSED(severityLevel);
    QAppCategoryOptions options(category, true);
    _registeredCategories << options;
}

QStringList QAppLogging::registeredCategories()
{
    QStringList sl;
    foreach (auto options, _registeredCategories) {
        sl.append(options.name);
    }
    return sl;
}

void QAppLogging::setCategoryLoggingOn(const QString &category, bool enable)
{
    QList<QAppCategoryOptions>::iterator it;
    QList<QAppCategoryOptions>::iterator end = _registeredCategories.end();
    for (it = _registeredCategories.begin(); it != end; it++) {
        if (it->name == category) {
            it->isEnable = enable;
            break;
        }
    }

    return;
}

bool QAppLogging::categoryLoggingOn(const QString &category)
{
    bool enable = false;
    foreach (auto options, _registeredCategories) {
        if (options.name == category) {
            enable = options.isEnable;
            break;
        }
    }

    return enable;
}

void QAppLogging::setFilterRulesByLevel(LogLevel severityLevel)
{
    QString filterRules;

    filterRules += QString("*") + QAL_TAG_TAIL + ".debug=false\n";
    filterRules += QString("*") + QAL_TAG_TAIL + ".info=false\n";
    filterRules += QString("*") + QAL_TAG_TAIL + ".warning=false\n";
    filterRules += QString("*") + QAL_TAG_TAIL + ".critical=false\n";
    filterRules += QString("*") + QAL_TAG_TAIL + ".fatal=false\n";

    foreach (auto options, _registeredCategories) {
        QString &category = options.name;
        bool isCategoryEnable = options.isEnable;
        if (!isCategoryEnable) {
            continue;
        }

        if (severityLevel <= TraceLevel) {
            filterRules += category;
            filterRules += ".debug=true\n";
        }
        if (severityLevel <= DebugLevel) {
            if (!category.contains(QAL_TAG_TRACE QAL_TAG_TAIL)) {
                filterRules += category;
                filterRules += ".debug=true\n";
            }
        }
        if (severityLevel <= InfoLevel) {
            filterRules += category;
            filterRules += ".info=true\n";
        }
        if (severityLevel <= WarnLevel) {
            filterRules += category;
            filterRules += ".warning=true\n";
        }
        if (severityLevel <= ErrorLevel) {
            filterRules += category;
            filterRules += ".critical=true\n";
        }
        if (severityLevel <= FatalLevel) {
            filterRules += category;
            filterRules += ".fatal=true\n";
        }
    }

    qDebug() << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}

