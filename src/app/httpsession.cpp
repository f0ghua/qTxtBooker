#include "httpsession.h"

#include <QFile>
#include <QEventLoop>

HttpSession::HttpSession(QObject *parent)
    : QObject(parent)
{
    connect(&m_webCtrl, &QNetworkAccessManager::finished,
            this, &HttpSession::fileDownloaded);
}

HttpSession::HttpSession(QUrl url, QObject *parent)
    : QObject(parent)
    , m_reqUrl(url)
{
    connect(&m_webCtrl, &QNetworkAccessManager::finished,
            this, &HttpSession::fileDownloaded);
}

void HttpSession::requestUrl()
{
    QNetworkRequest request(m_reqUrl);
    m_webCtrl.get(request);
}

void HttpSession::fileDownloaded(QNetworkReply *reply)
{
    m_downloadedData = reply->readAll();
    reply->deleteLater();

    emit downloaded();
}

QByteArray HttpSession::requestUrl2File(QUrl url, const QString &fileName)
{
    QNetworkRequest request(url);
    m_webCtrl.get(request);

    QEventLoop loop;
    QObject::connect(this, &HttpSession::downloaded, &loop, &QEventLoop::quit);
    loop.exec();

    QFile localFile(fileName);
    if (localFile.open(QIODevice::WriteOnly)) {
        localFile.write(m_downloadedData);
        localFile.close();
    }

    return m_downloadedData;
}
