#include "archive.h"
#include "mainwindow.h"
#include "mainthread.h"
#include "treemodel.h"
//#include "webdav/webdav.h"
//#include "webdav/webdav_url_info.h"

#include <QApplication>
#include <QAuthenticator>
//#include <QBuffer>
//#include <QDir>
#include <QHttp>
#include <QInputDialog>
//#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressBar>
#include <QSettings>
#include <QStatusBar>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>


MainWindow::MainWindow()
{
    textEdit = new QTextEdit;
    quitButton = new QPushButton(tr("Quit"));
 
    connect(quitButton, SIGNAL(clicked()), this, SLOT(quit()));
    
    
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(textEdit);
    layout->addWidget(quitButton);

    setCentralWidget(centralWidget);


    setWindowTitle(tr("SyncBox"));
    resize(1000,400);

  progress = new QProgressBar(this);

  statusBar()->insertPermanentWidget(0, progress);

  thread = new MainThread(this);
  connect(thread, SIGNAL(logSignal(const QString&)), this, SLOT(log(const QString&)));
  connect(thread, SIGNAL(dataProgress(int, int)), this, SLOT(dataProgress(int,int)));
    
  QSettings settings;
  QString webdavPath = settings.value("webdavPath").toString();
  bool ok=true;
  while (webdavPath.isEmpty() || !ok) 
    webdavPath = QInputDialog::getText(this, "SyncBox", "WebDAV Path", QLineEdit::Normal, "/syncbox", &ok);  

  settings.setValue("webdavPath", webdavPath);
  settings.sync();
  
  archive = new Archive(thread, QDir("/home/mark/dokumente"), webdavPath);
  archive->initializeFolders();
  
  QMenu* menu = menuBar()->addMenu("&Menu");
  menu->addAction("&Browser", this, SLOT(getDataAndOpenBrowser()));
  menu->addAction("&Start Updates", archive, SLOT(startUpdates()));
  menu->addSeparator();
  menu->addAction("E&xit", this, SLOT(quit()));
  
}



void MainWindow::getDataAndOpenBrowser() {
  openBrowser();
  //watcher.setFuture(QtConcurrent::run(archive, &Archive::getRoot));
}
void MainWindow::openBrowser() {
  qDebug("openBrowser");
  QTreeView *tree = new QTreeView;
  TreeModel *model = new TreeModel(archive->getRoot(), archive, tree);
  tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tree, SIGNAL(customContextMenuRequested(const QPoint &)),
            model, SLOT(showContextMenu(const QPoint &)));
  
  tree->setModel(model);
  tree->showMaximized();
}

void MainWindow::dataProgress(int done, int total)
{
  progress->setMaximum(total);
  progress->setValue(done);
}


void MainWindow::stateChanged(int state)                                                                                                                                                           
{                                                                                                                                                                                             
  QString text = "Unknown";                                                                                                                                                                   
                                                                                                                                                                                              
  if (state == QHttp::Unconnected)                                                                                                                                                            
    text = "Unconnected";                                                                                                                                                                     
  if (state == QHttp::HostLookup)                                                                                                                                                             
    text = "HostLookup";                                                                                                                                                                      
  if (state == QHttp::Connecting)                                                                                                                                                             
    text = "Connecting";                                                                                                                                                                      
  if (state == QHttp::Sending)                                                                                                                                                                
    text = "Sending";                                                                                                                                                                         
  if (state == QHttp::Reading)                                                                                                                                                                
    text = "Reading";                                                                                                                                                                         
  if (state == QHttp::Connected)                                                                                                                                                              
    text = "Connected";                                                                                                                                                                       
  if (state == QHttp::Closing)                                                                                                                                                                
    text = "Closing";                                                                                                                                                                         
  statusBar()->showMessage(text, 10000);
}                                                                                                                                                                                             


void MainWindow::authenticationRequired(const QString &hostname, quint16 port,                                                                                                                     
                                   QAuthenticator *authenticator)                                                                                                                             
{                                                                                                                                                                                             
  bool ok;                                                                                                                                                                                    
  QString user;                                                                                                                                                                               
  QString pass;                                                                                                                                                                               
                                                                                                                                                                                              
    user = QInputDialog::getText(this, tr("Authentication %1:%2").arg(hostname).arg(port),
                                 tr("User name:"), QLineEdit::Normal,
                                 "mark.reiche", &ok);
    pass = QInputDialog::getText(this, tr("Authentication"),
                                 tr("Password:"), QLineEdit::Password,
                                 tr(""), &ok);
  authenticator->setUser(user);                                                                                                                                                               
  authenticator->setPassword(pass);                                                                                                                                                           
}                                                                                                                                                                                             

void MainWindow::quit()
{
    QApplication::quit();
}

void MainWindow::log(const QString& string) {
  textEdit->append(string);
}