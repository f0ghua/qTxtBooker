#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Worker;
class QThread;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void onIndexDownloaded();
    void onPageDownloaded(int index);

private slots:
    void on_pbReqPages_clicked();
    void on_pbPullPages_clicked();

private:
    void startWorker();
    void stopWorker();

    Ui::MainWindow *ui;
    Worker *m_worker = NULL;
    QThread *m_workThread = NULL;
};

#endif // MAINWINDOW_H
