// Qt 4 shim for the one thing the shared sources take from libsailfishapp:
// SailfishApp::pathTo("qml/lib/currencies.json"). On Harmattan the data
// lives under /opt/harbour-passviewer, or beside meego/ when run from a
// source tree; meego/platform.cpp decides which and stores it here.
#pragma once
#include <QString>
#include <QUrl>

namespace SailfishApp {
    // Set once by main() before anything asks.
    QString dataDir();
    void setDataDir(const QString &dir);

    inline QUrl pathTo(const QString &relative)
    {
        return QUrl::fromLocalFile(dataDir() + "/" + relative);
    }
}
