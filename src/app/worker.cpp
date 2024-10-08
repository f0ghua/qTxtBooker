#include "worker.h"
#include "httpsession.h"
#include "qcompressor.h"
#include "QAppLogging.h"
#include "htmlentityparser.h"

#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTextCodec>
#include <QCoreApplication>
#include <QTextDocument>
#include <QTime>

static const char * const KEY_LINK_ENCODE       = "encode";
static const char * const KEY_LINK_STR_START    = "link_str_start";
static const char * const KEY_LINK_STR_END      = "link_str_end";
static const char * const KEY_LINK_PATTERN      = "link_pattern";
static const char * const KEY_CONTENT_PATTERN   = "content_pattern";
static const char * const KEY_INTERVAL          = "interval";
static const char * const KEY_LINKTYPE          = "link_type";
static const char * const KEY_PAGECTPATTERN     = "page_ctpattern";
static const char * const KEY_PAGECTSTART       = "page_ctstart";
static const char * const KEY_PAGECTCOUNT       = "page_ctcount";
static const char * const KEY_FMTPAGELINK       = "page_fmtlink";
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

static QString getPageUrl(const SiteInfo &si, const QString &sublink)
{
    QString linkAddr;
    const QUrl &url = si.m_url;
    switch (si.m_linkType) {
        case 0: {
            linkAddr = url.scheme() + "://" + url.host() + sublink;
            break;
        }
        case 1: {
            linkAddr = url.toString() + sublink;
            break;
        }
        case 2: {
            QString strUrl = url.toString();
            int pos = strUrl.lastIndexOf('/');
            linkAddr = strUrl.left(pos) + "/" + sublink;
            break;
        }
        case 3: {
            linkAddr = sublink;
            break;
        }
        case 4: {
            if (!si.m_formatPageLink.isEmpty()) {
                linkAddr = si.m_formatPageLink.arg(sublink);
            } else {
                QLOG_ERROR() << "No page link format was defined";
            }

            break;
        }
        default:
            break;
    }
    return linkAddr;
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
    if (ba.isEmpty()) {
        QLOG_ERROR() << "index file is NULL";
        return false;
    }
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
    re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    QRegularExpressionMatchIterator i = re.globalMatch(indexContent);

    m_pageInfos.clear();

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString sublink = match.captured(1);
        QString title = match.captured(2);
        QString linkAddr;
        const QUrl &url = m_siteInfo.m_url;
        switch (m_siteInfo.m_linkType) {
            case 0: {
                linkAddr = url.scheme() + "://" + url.host() + sublink;
                break;
            }
            case 1: {
                linkAddr = url.toString() + sublink;
                break;
            }
            case 2: {
                QString strUrl = url.toString();
                int pos = strUrl.lastIndexOf('/');
                linkAddr = strUrl.left(pos) + "/" + sublink;
                break;
            }
            case 3: {
                linkAddr = sublink;
                break;
            }
            case 4: {
                if (!m_siteInfo.m_formatPageLink.isEmpty()) {
                    linkAddr = m_siteInfo.m_formatPageLink.arg(sublink);
                } else {
                    QLOG_ERROR() << "No page link format was defined";
                }

                break;
            }
            default:
                break;
        }

        m_pageInfos.append(PageInfo(linkAddr, title));
        //qDebug() << link << title;

//        for (int i = 0; i < m_siteInfo.m_pageContinueCount; i++) {
//            int indexOfDot = linkAddr.lastIndexOf('.');
//            QString part1 = linkAddr.mid(0, indexOfDot);
//            QString part2 = linkAddr.mid(indexOfDot);
//            QString linkAddrPart2 = part1 + QString("_%1").arg(i+1) + part2;

//            m_pageInfos.append(PageInfo(linkAddrPart2, ""));
//        }

    }

    emit indexDownloaded();

    return true;
}

static inline void htmlRemoveEscape(QString &escapedStr)
{
#if 0
    escapedStr.replace("&nbsp;", " ");
    escapedStr.replace("&hellip;", "...");
    escapedStr.replace("&ldquo;", "\"");
    escapedStr.replace("&rdquo;", "\"");
#else

//    HtmlEntityParser ep;
//    escapedStr = ep.parseText(escapedStr);

    QTextDocument doc;
    doc.setHtml(escapedStr);
    escapedStr = doc.toPlainText();

    //qDebug("%s.", qPrintable(escapedStr));

#endif
}

bool Worker::pullBookPage(QTextStream &out, int index)
{
    const PageInfo &pi = m_pageInfos.at(index);
    //const QString &urlStr = pi.m_linkAddr;

    auto getCtPageUrlStr = [](const QString &linkAddr, int index) {
        int indexOfDot = linkAddr.lastIndexOf('.');
        QString part1 = linkAddr.mid(0, indexOfDot);
        QString part2 = linkAddr.mid(indexOfDot);
        QString ctPageUrlStr = part1 + QString("_%1").arg(index) + part2;
        return ctPageUrlStr;
    };

    auto pullPageContentByUrl = [&](const QString &urlStr, QString &ctPageUrl){
        QLOG_DEBUG() << "request page url =" << urlStr;

        QUrl url(urlStr);
        QByteArray ba = m_session->requestUrl2File(url, BOOK_PAGE_FNAME);
        QString content;
        if (m_siteInfo.m_encode == "UTF8") {
            content = QString::fromStdString(ba.toStdString());
        } else {
            content = GBK2UTF8(ba);
        }

        ctPageUrl.clear();
        // check if there continue page exist
        if (!m_siteInfo.m_pageCtPattern.isEmpty()) {
            QRegularExpression re(m_siteInfo.m_pageCtPattern);
            re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
            QRegularExpressionMatch match = re.match(content);
            if (match.hasMatch()) {
                ctPageUrl = match.captured(1);
            }
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
            QLOG_DEBUG() << "no content match of link" << urlStr;
            return QString();
        }

        QString matchedContent = match.captured(1);

        htmlRemoveEscape(matchedContent);
        matchedContent.replace(" ", "");
        matchedContent.replace("@@", "@");
        matchedContent.replace("@", "\r\n");

        return matchedContent;
    };

    QString ctPageUrl;
    auto matchedContent = pullPageContentByUrl(pi.m_linkAddr, ctPageUrl);

    if (!pi.m_title.isEmpty()) {
        out << pi.m_title;
        out << "\r\n";
    }
    out << matchedContent;
    out << "\r\n";

    qDebug() << "ctPageUrl = " << ctPageUrl;
    //int ctIndex = m_siteInfo.m_pageCtStart;
    while (!ctPageUrl.isEmpty()) { // there must be continue pages
        QString urlStr = getPageUrl(m_siteInfo, ctPageUrl);
        //getCtPageUrlStr(pi.m_linkAddr, ctIndex);
        auto matchedContent = pullPageContentByUrl(urlStr, ctPageUrl);

        out << matchedContent;
        out << "\r\n";

        if (m_siteInfo.m_interval) {
            msleep(m_siteInfo.m_interval);
        }
        //ctIndex++;
    }

    emit pageDownloaded(index);
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
//        const PageInfo &pi = m_pageInfos.at(index);
//        const QString &urlStr = pi.m_linkAddr;

//        QLOG_DEBUG() << "request page url =" << urlStr;

//        QUrl url(urlStr);
//        QByteArray ba = m_session->requestUrl2File(url, BOOK_PAGE_FNAME);
//        QString content;
//        if (m_siteInfo.m_encode == "UTF8") {
//            content = QString::fromStdString(ba.toStdString());
//        } else {
//            content = GBK2UTF8(ba);
//        }

//        //content.replace("\r\n", "");
//        content.replace("<br />", "@");
//        //content.replace("<br/>", "@");
//        //content.replace("</br>", "@");
//        //content.replace("<br>", "@");
//        content.replace(QRegularExpression("\r|\n|\t"), "");
//        content.replace(QRegularExpression("</*br/*>"), "@");

//        const QString &pattern = m_siteInfo.m_bookPattern;
//        //qDebug() << pattern;
//        //qDebug() << content;
//        QRegularExpression re(pattern);
//        re.setPatternOptions(QRegularExpression::DotMatchesEverythingOption | QRegularExpression::InvertedGreedinessOption);
//        QRegularExpressionMatch match = re.match(content);
//        bool hasMatch = match.hasMatch(); // true
//        if (!hasMatch) {
//            QLOG_DEBUG() << "no content match of link" << pi.m_linkAddr;
//            break;
//        }

//        QString matchedContent = match.captured(1);

//        htmlRemoveEscape(matchedContent);
//        matchedContent.replace(" ", "");
//        matchedContent.replace("@@", "@");
//        matchedContent.replace("@", "\r\n");

//        if (!pi.m_title.isEmpty()) {
//            out << pi.m_title;
//            out << "\r\n";
//        }
//        out << matchedContent;
//        out << "\r\n";

//        emit pageDownloaded(index);

        pullBookPage(out, index);

        if (m_siteInfo.m_interval) {
            msleep(m_siteInfo.m_interval);
        }

        QCoreApplication::processEvents();
        if (m_isQuit) {
            break;
        }
    }

    outFile->close();

    return true;
}

void Worker::onWorkStop()
{
    m_isQuit = true;
}

bool Worker::loadSiteConfigs(const QUrl &url)
{
    const QString &domain = url.host();
    QSettings settings("./config.ini", QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    QLOG_DEBUG() << "try to load config for host " << domain;

    QStringList groups = settings.childGroups();
    bool isSupport = false;
    foreach (const QString &group, groups) {
        if (group == domain) {
            settings.beginGroup(group);
            m_siteInfo.m_encode = settings.value(KEY_LINK_ENCODE, "UTF8").toString();
            m_siteInfo.m_linkStart = settings.value(KEY_LINK_STR_START).toString();
            m_siteInfo.m_linkEnd = settings.value(KEY_LINK_STR_END).toString();
            m_siteInfo.m_linkPattern = settings.value(KEY_LINK_PATTERN).toString();
            m_siteInfo.m_bookPattern = settings.value(KEY_CONTENT_PATTERN).toString();
            m_siteInfo.m_interval = settings.value(KEY_INTERVAL, 0).toInt();
            m_siteInfo.m_linkType = settings.value(KEY_LINKTYPE, 0).toInt();
            m_siteInfo.m_pageCtPattern = settings.value(KEY_PAGECTPATTERN).toString();
            m_siteInfo.m_pageCtStart = settings.value(KEY_PAGECTSTART, 0).toInt();
            m_siteInfo.m_pageContinueCount = settings.value(KEY_PAGECTCOUNT, 0).toInt();
            m_siteInfo.m_formatPageLink = settings.value(KEY_FMTPAGELINK).toString();
            settings.endGroup();

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
