#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class HttpSession;

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

signals:

public slots:
    void run();

private:
    HttpSession *m_session = nullptr;
};

#endif // WORKER_H
