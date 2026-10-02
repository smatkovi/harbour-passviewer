// The Harmattan Notificator: the same three calls the pages make on the
// Sailfish one (src/notificator.h), done with MNotification instead of
// nemonotifications. meego/build.sh compiles this file in place of
// src/notificator.cpp.
#pragma once

#include <QList>
#include <QObject>
#include <QString>

class MNotification;

class Notificator : public QObject
{
    Q_OBJECT
public:
    explicit Notificator(QObject *parent = 0);
    ~Notificator();

    // A standing notification in the event feed, one per pass; tapping it
    // opens the pass through the D-Bus interface of meego/platform.h.
    Q_INVOKABLE void addNotification(QString origin, QString summary, QString body);
    Q_INVOKABLE void removeNotification(QString origin);
    // A short banner, gone by itself.
    Q_INVOKABLE void bannerNotification(QString summary, QString body);

signals:
    void bannerRequested(const QString &summary, const QString &body);

private:
    struct Entry {
        QString origin;
        MNotification *notification;
    };
    QList<Entry> m_notifications;
};
