#ifndef PASSHANDLER_H
#define PASSHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QMap>
#include <QStringList>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileInfo>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QProcess>

#include "zipfile.h"
#include "passdb.h"
#include "passinfo.h"

class PassHandler : public QObject
{
    Q_OBJECT
public:
    explicit PassHandler(QObject *parent = 0);

    Q_INVOKABLE QString getCanonicalPath(QString path);
    Q_INVOKABLE void removePass(QString path);
    Q_INVOKABLE void updatePass(QString path);
    Q_INVOKABLE QMap<QString, QVariant> getFields(QString filename);
    Q_INVOKABLE QStringList getChanges(QString oldfile, QString newfile);
    Q_INVOKABLE void createCalendarEntry(QString subject, QString isoDateTime);

signals:
    void updateFinished(QString state);
    void calendarEntryFinished(QString state);

public slots:
#if QT_VERSION >= 0x050000
    void replyFinished(QNetworkReply* reply);
#else
    void fetchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void fetchFailed(QProcess::ProcessError error);
#endif

private:
    QNetworkAccessManager m_network;
    QMap<QNetworkReply*, QString> m_replies;
#if QT_VERSION < 0x050000
    // One running passviewer-fetch (meego/fetch) per update: the pass it is
    // for, and the file the answer lands in.
    struct Fetch {
        QString path;
        QTemporaryFile* file;
    };
    QMap<QProcess*, Fetch> m_fetches;
#endif

    void m_installUpdate(const QString &path, const QString &downloaded);
    void m_copyFile(QIODevice &to, QIODevice &from);
    QByteArray m_rfc2616(QDateTime date);
};

#endif // PASSHANDLER_H
