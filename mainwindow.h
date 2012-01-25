#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTextEdit>
#include <QMainWindow>



class QPushButton;
class QAuthenticator;
class QHttpResponseHeader;
class QBuffer;
class MainThread;
class QProgressBar;
class MyRoot;
class Archive;

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    MainWindow();

public slots:
  void log(const QString& string);
  void dataProgress(int done, int total);
  void getDataAndOpenBrowser();
  void openBrowser();


private slots:
  void quit();
  void authenticationRequired(const QString &hostname, quint16 port,                                                                                                                          
                              QAuthenticator *authenticator);                                                                                                                                 
  void stateChanged(int state);                                                                                                                                                               
private:
  QTextEdit *textEdit;
  QPushButton *quitButton;
  MainThread *thread;
  QProgressBar *progress;
  Archive *archive;

};

#endif