BUILDPATH=../../../build/syncbox
OBJECTS_DIR = $$BUILDPATH/.obj
MOC_DIR = $$BUILDPATH/.moc
TARGET=$$BUILDPATH/syncbox
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += . /usr/include/qca2/QtCrypto/
LIBS += -L/usr/lib64/qca2 -L/usr/lib/qca2 -lqca
QT += xml network

# Input
HEADERS += mainwindow.h webdav/webdav.h webdav/webdav_export.h webdav/webdav_url_info.h mainthread.h \
    archive.h mydirentry.h treemodel.h 
SOURCES += main.cpp mainwindow.cpp webdav/webdav.cpp webdav/webdav_url_info.cpp mainthread.cpp \
    archive.cpp mydirentry.cpp treemodel.cpp mysymlink.cpp 
  