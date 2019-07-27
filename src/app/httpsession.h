#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class HttpSession : public QObject
{
     Q_OBJECT
public:
    HttpSession(QObject *parent = 0);
    explicit HttpSession(QUrl url, QObject *parent = 0);
    virtual ~HttpSession() {}

    QByteArray requestUrl2File(QUrl url, const QString &fileName);
    void setUrl(QUrl url) {m_reqUrl = url;}
    void requestUrl();

signals:
    void downloaded();

private slots:
    void fileDownloaded(QNetworkReply* reply);

private:
    QNetworkAccessManager m_webCtrl;
    QByteArray m_downloadedData;
    QUrl m_reqUrl;

};

#endif // HTTPSESSION_H
