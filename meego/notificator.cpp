#include "notificator.h"

#include <MNotification>
#include <MRemoteAction>

#include <QVariant>

Notificator::Notificator(QObject *parent) :
    QObject(parent)
{
    // Notifications of an earlier run are gone with that run: the passes
    // are rescanned and announced again at start-up.
    QList<MNotification *> old = MNotification::notifications();
    for (int i = 0; i < old.size(); ++i) {
        old.at(i)->remove();
        delete old.at(i);
    }
}

Notificator::~Notificator()
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        m_notifications.at(i).notification->remove();
        delete m_notifications.at(i).notification;
    }
}

void Notificator::addNotification(QString origin, QString summary, QString body)
{
    // Replace the standing one for the same pass, if there is one.
    removeNotification(origin);
    MNotification *notification = new MNotification(MNotification::DeviceEvent, summary, body);
    notification->setImage("harbour-passviewer");
    QList<QVariant> arguments;
    arguments << origin;
    notification->setAction(MRemoteAction("ch.p2501.harbour-passviewer", "/ch/p2501/harbour_passviewer",
                                          "ch.p2501.harbour_passviewer", "openPass", arguments));
    notification->publish();
    Entry entry;
    entry.origin = origin;
    entry.notification = notification;
    m_notifications.append(entry);
}

void Notificator::removeNotification(QString origin)
{
    for (int i = m_notifications.size() - 1; i >= 0; --i) {
        if (m_notifications.at(i).origin == origin) {
            m_notifications.at(i).notification->remove();
            delete m_notifications.at(i).notification;
            m_notifications.removeAt(i);
        }
    }
}

void Notificator::bannerNotification(QString summary, QString body)
{
    // Harmattan's system banners are tied to event types the app does not
    // own; the window shows an InfoBanner of its own instead.
    emit bannerRequested(summary, body);
}
