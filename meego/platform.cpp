#include "platform.h"

#include <QApplication>
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusMessage>

Platform::Platform(const QStringList &arguments, QObject *parent) :
    QObject(parent),
    m_displayOn(true),
    m_arguments(arguments)
{
    // mce announces "on", "dimmed" and "off"; the page stops asking for the
    // position while the display is off, as the Sailfish version does.
    QDBusConnection::systemBus().connect("com.nokia.mce", "/com/nokia/mce/signal",
                                         "com.nokia.mce.signal", "display_status_ind",
                                         this, SLOT(displayStatusChanged(QString)));
}

QString Platform::clipboardText() const
{
    return QApplication::clipboard()->text();
}

void Platform::setClipboardText(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

void Platform::requestPass(const QString &origin)
{
    emit passRequested(origin);
}

void Platform::displayStatusChanged(const QString &status)
{
    const bool on = status != "off";
    if (on == m_displayOn)
        return;
    m_displayOn = on;
    emit displayOnChanged();
}

PassViewerAdaptor::PassViewerAdaptor(Platform *platform) :
    QDBusAbstractAdaptor(platform),
    m_platform(platform)
{
}

void PassViewerAdaptor::openPass(const QString &origin)
{
    m_platform->requestPass(origin);
}
