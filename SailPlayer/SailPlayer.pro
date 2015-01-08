TEMPLATE=app
TARGET = harbour-sail-player

CONFIG += sailfishapp
QT += declarative

TARGETPATH = /usr/bin
target.path = $$TARGETPATH

DEPLOYMENT_PATH = /usr/share/$$TARGET
qml.path = $$DEPLOYMENT_PATH
desktop.path = /usr/share/applications

DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/\"\\\"\"

SOURCES += \
    src/Main.cpp \
    src/Artist.cpp \
    src/Album.cpp \
    src/Track.cpp \
    src/FilesListModel.cpp \
    src/FileInfo.cpp

OTHER_FILES += \
    qml/controls/Playlist.qml \
    qml/cover/CoverPage.qml \
    qml/pages/DefaultPage.qml \
    translations/*.ts \
    harbour-sail-player.desktop \
    qml/CoverPage.qml \
    qml/pages/AddFilesPage.qml \
    qml/Main.qml

TRANSLATIONS += translations/harbour-sail-player-de.ts

HEADERS += \
    include/Artist.h \
    include/Album.h \
    include/Track.h \
    include/FilesListModel.h \
    include/FileInfo.h

INCLUDEPATH += $$PWD/include