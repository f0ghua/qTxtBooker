#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "worker.h"

#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->leIndexAddr->setText("https://www.mywenxue.com/xiaoshuo/127/127690/Index.htm");

    startWorker();
}

MainWindow::~MainWindow()
{
    stopWorker();
    delete ui;
}

void MainWindow::startWorker()
{
    m_workThread = new QThread();
    m_worker = new Worker();
    m_worker->moveToThread(m_workThread);
    QObject::connect(m_workThread, &QThread::started, m_worker, &Worker::run);
    QObject::connect(m_workThread, &QThread::finished, m_worker, &Worker::deleteLater);
    QObject::connect(m_workThread, &QThread::finished, m_workThread, &QThread::deleteLater);
    //QObject::connect(this, &MainWindow::workStop, m_worker, &Worker::onWorkStop);
    QObject::connect(m_worker, &Worker::indexDownloaded, this, &MainWindow::onIndexDownloaded);
    QObject::connect(m_worker, &Worker::pageDownloaded, this, &MainWindow::onPageDownloaded);

    m_workThread->start(QThread::HighPriority);
    qDebug() << "Worker thread started.";
}

void MainWindow::stopWorker()
{
    //emit workStop();

    if(m_workThread && (!m_workThread->isFinished())) {
        m_workThread->quit();
        if(!m_workThread->wait()) {
            qDebug() << "can't stop thread";
        }
        qDebug() << "Worker thread finished.";
    }
}

void MainWindow::onIndexDownloaded()
{
    ui->cbPageStart->clear();
    ui->cbPageEnd->clear();
    for (int i = 0; i < m_worker->m_pageInfos.size(); i++) {
        const PageInfo &pi = m_worker->m_pageInfos[i];
        ui->cbPageStart->insertItem(i, pi.m_title);
        ui->cbPageEnd->insertItem(i, pi.m_title);
    }
}

void MainWindow::onPageDownloaded(int index)
{
    const PageInfo &pi = m_worker->m_pageInfos[index];
    QString msg = QString("GET PAGE: %1").arg(pi.m_title);
    ui->plainTextEdit->appendPlainText(msg);
}

void MainWindow::on_pbReqPages_clicked()
{
    const QString &urlStr = ui->leIndexAddr->text();
    QMetaObject::invokeMethod(m_worker,
                              "requestBookPages",
                              Qt::QueuedConnection,
                              //Q_RETURN_ARG(bool, ret),
                              Q_ARG(const QString&, urlStr));

    return;
}

void MainWindow::on_pbPullPages_clicked()
{
    int start = ui->cbPageStart->currentIndex();
    int end = ui->cbPageEnd->currentIndex();

    QMetaObject::invokeMethod(m_worker,
                              "pullBookPages",
                              Qt::QueuedConnection,
                              Q_ARG(int, start),
                              Q_ARG(int, end)
                              );

    return;
}
