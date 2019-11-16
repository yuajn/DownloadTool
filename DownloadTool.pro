QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

DESTDIR =../bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = ui

RC_ICONS = download.ico

# 1.0.0.0 下载功能
# 1.0.0.1 添加读取文件大小、已下载文件大小功能
# 1.0.1.0 添加下载网速功能
# 1.0.1.1 添加剩余下载时间功能
# 1.0.2.0 多功能升级
VERSION = 1.0.2.0

SOURCES += \
    main.cpp \
    downloadtool.cpp

HEADERS += \
    downloadtool.h \
    general.h

FORMS += \
    downloadtool.ui

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc

win32: LIBS += -liphlpapi
