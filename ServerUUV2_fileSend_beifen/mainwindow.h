#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void selectFolder(QString path);

private slots:
//    void on_selectFile_clicked();
    void on_startListen_clicked();


    void on_selectFolder_clicked();

private:
    Ui::MainWindow *ui;

private:
    QTcpServer* m_s;
    QTcpSocket* m_tcp;

    QHash<QString, int> m_instructions;

private:
    void initWidgets();
};
#endif // MAINWINDOW_H
