#include "worker.h"
#include "httpsession.h"
#include "qcompressor.h"
#include "QAppLogging.h"

#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTextCodec>
#include <QCoreApplication>

static const char * const KEY_LINK_ENCODE       = "encode";
static const char * const KEY_LINK_STR_START    = "link_str_start";
static const char * const KEY_LINK_STR_END      = "link_str_end";
static const char * const KEY_LINK_PATTERN      = "link_pattern";
static const char * const KEY_CONTENT_PATTERN   = "content_pattern";
static const char * const KEY_INTERVAL          = "interval";
static const char * const INDEX_PAGE_FNAME      = "./index.html";
static const char * const BOOK_PAGE_FNAME       = "./page.html";
static const char * const BOOK_SAVE_FNAME       = "./out.txt";

static void msleep(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);

    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

static inline QString GBK2UTF8(const QByteArray &ba)
{
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec *gbk = QTextCodec::codecForName("GBK");

    // gbk -> unicode
    QString strUnicode = gbk->toUnicode(ba.data());
    // unicode -> utf-8
    QByteArray baUtf8 =utf8->fromUnicode(strUnicode);
    return QString::fromStdString(baUtf8.toStdString());
}

Worker::Worker(QObject *parent) : QObject(parent)
{

}

void Worker::run()
{
    m_session = new HttpSession(this);
}

bool Worker::requestBookPages(const QString &urlStr)
{
    QUrl url(urlStr);
    //qDebug() << url.scheme() << url.path();

    if (!loadSiteConfigs(url))
        return false;

    QByteArray ba = m_session->requestUrl2File(url, INDEX_PAGE_FNAME);
    if ((static_cast<uint8_t>(ba.at(0)) == 0x1F)
            && (static_cast<uint8_t>(ba.at(1)) == 0x8B)) {
        // gzip file is detected
        QByteArray compressed = ba;
        if (!QCompressor::gzipDecompress(compressed, ba)) {
            QLOG_ERROR() << "gzip file decompress fail";
            return false;
        }
    }

    QString indexContent;
    if (m_siteInfo.m_encode == "UTF8") {
        indexContent = QString::fromStdString(ba.toStdString());
    } else {
        indexContent = GBK2UTF8(ba);
    }

    QRegularExpression re(m_siteInfo.m_linkPattern);
    QRegularExpressionMatchIterator i = re.globalMatch(indexContent);

    m_pageInfos.clear();

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString sublink = match.captured(1);
        QString title = match.captured(2);
        QString linkAddr;
        const QUrl &url = m_siteInfo.m_url;
        if (!urlStr.endsWith('/')) {
            // there is a file name exist
            linkAddr = url.scheme() + "://" + url.host() + sublink;
        } else {
            linkAddr = url.toString() + sublink;
        }
        m_pageInfos.append(PageInfo(linkAddr, title));
        //qDebug() << link << title;
    }

    emit indexDownloaded();

    return true;
}

bool Worker::pullBookPages(int start, int end)
{
    QFile *outFile = new QFile(BOOK_SAVE_FNAME);
    if (!outFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        delete outFile;
        return false;
    }
    QTextStream out(outFile);

    for (int index = start; index <= end; index++) {
        const PageInfo &pi = m_pageInfos.at(index);
        const QString &urlStr = pi.m_linkAddr;
        QUrl url(urlStr);
        QByteArray ba = m_session->requestUrl2File(url, BOOK_PAGE_FNAME);
        QString content;
        if (m_siteInfo.m_encode == "UTF8") {
            content = QString::fromStdString(ba.toStdString());
        } else {
            content = GBK2UTF8(ba);
        }

        //content.replace("\r\n", "");
        content.replace("<br />", "@");
        //content.replace("<br/>", "@");
        //content.replace("</br>", "@");
        //content.replace("<br>", "@");
        content.replace(QRegularExpression("\r|\n|\t"), "");
        content.replace(QRegularExpression("</*br/*>"), "@");

        const QString &pattern = m_siteInfo.m_bookPattern;
        //qDebug() << pattern;
        //qDebug() << content;
        QRegularExpression re(pattern);
        re.setPatternOptions(QRegularExpression::DotMatchesEverythingOption | QRegularExpression::InvertedGreedinessOption);
        QRegularExpressionMatch match = re.match(content);
        bool hasMatch = match.hasMatch(); // true
        if (!hasMatch) {
            QLOG_DEBUG() << "no content match of link" << pi.m_linkAddr;
            break;
        }

        QString matchedContent = match.captured(1);
        matchedContent.replace("&nbsp;", " ");
        matchedContent.replace(" ", "");
        matchedContent.replace("@@", "@");
        matchedContent.replace("@", "\r\n");

        out << pi.m_title;
        out << "\r\n";
        out << matchedContent;
        out << "\r\n";

        emit pageDownloaded(index);

        if (m_siteInfo.m_interval) {
            msleep(m_siteInfo.m_interval);
        }
    }

    outFile->close();

    return true;
}

bool Worker::loadSiteConfigs(const QUrl &url)
{
    const QString &domain = url.host();
    QSettings *cfg = new QSettings("./config.ini", QSettings::IniFormat);
    cfg->setIniCodec("UTF-8");

    QLOG_DEBUG() << "try to load config for host " << domain;

    QStringList groups = cfg->childGroups();
    bool isSupport = false;
    foreach (const QString &group, groups) {
        if (group == domain) {
            cfg->beginGroup(group);
            m_siteInfo.m_encode = cfg->value(KEY_LINK_ENCODE, "UTF8").toString();
            m_siteInfo.m_linkStart = cfg->value(KEY_LINK_STR_START).toString();
            m_siteInfo.m_linkEnd = cfg->value(KEY_LINK_STR_END).toString();
            m_siteInfo.m_linkPattern = cfg->value(KEY_LINK_PATTERN).toString();
            m_siteInfo.m_bookPattern = cfg->value(KEY_CONTENT_PATTERN).toString();
            m_siteInfo.m_interval = cfg->value(KEY_INTERVAL, 0).toInt();
            cfg->endGroup();

            isSupport = true;
            break;
        }
    }
    if (!isSupport) {
        QLOG_ERROR() << QObject::tr("site %1 has not been supported").arg(domain);
        return false;
    }

    m_siteInfo.m_url = url;

    return true;
}
