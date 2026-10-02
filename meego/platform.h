// The pieces of the Sailfish app that were QML plugins there and are plain
// C++ here: the single-instance D-Bus hook (Nemo.DBus's DBusAdaptor), the
// display state from mce (DBusInterface on the system bus), the clipboard
// (Silica's Clipboard), and where the data lives (SailfishApp::pathTo).
#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QString>
#include <QStringList>

// Exposed to QML as "platform". FirstPage listens to passRequested() instead
// of declaring a DBusAdaptor, reads displayOn instead of a DBusInterface,
// and the start-up argument comes from arguments() instead of
// Qt.application.arguments, which Qt 4.7 does not have.
class Platform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool displayOn READ displayOn NOTIFY displayOnChanged)
    Q_PROPERTY(QStringList arguments READ arguments CONSTANT)
    Q_PROPERTY(QString text READ clipboardText WRITE setClipboardText)
public:
    explicit Platform(const QStringList &arguments, QObject *parent = 0);

    bool displayOn() const { return m_displayOn; }
    QStringList arguments() const { return m_arguments; }
    QString clipboardText() const;
    void setClipboardText(const QString &text);

    // Called over D-Bus by a second instance that was started with a file.
    void requestPass(const QString &origin);

signals:
    void passRequested(const QString &origin);
    void displayOnChanged();

private slots:
    void displayStatusChanged(const QString &status);

private:
    bool m_displayOn;
    QStringList m_arguments;
};

// ch.p2501.harbour_passviewer on /ch/p2501/harbour_passviewer, the same
// interface the Sailfish version registers from QML, so that a second start
// with a pass file reaches the running instance the same way.
class PassViewerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "ch.p2501.harbour_passviewer")
public:
    explicit PassViewerAdaptor(Platform *platform);

public slots:
    void openPass(const QString &origin);

private:
    Platform *m_platform;
};
