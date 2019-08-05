#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QVector>
#include <QUrl>

class HttpSession;

struct SiteInfo {
    QUrl m_url;
    QString m_encode;
    QString m_linkStart;
    QString m_linkEnd;
    QString m_linkPattern;
    QString m_bookPattern;
    int m_interval = 0;
};

struct PageInfo {
    PageInfo() {}
    PageInfo(QString link, QString title)
        :m_linkAddr(link), m_title(title) {}
    QString m_linkAddr;
    QString m_title;
};

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

    SiteInfo m_siteInfo;
    QVector<PageInfo> m_pageInfos;

signals:
    void indexDownloaded();
    void pageDownloaded(int index);

public slots:
    void run();
    bool requestBookPages(const QString &urlStr);
    bool pullBookPages(int start, int end);

private:
    void oldparser();
    bool loadSiteConfigs(const QUrl &url);

    HttpSession *m_session = nullptr;

};

#endif // WORKER_H
