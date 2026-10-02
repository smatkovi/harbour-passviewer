// Pass Viewer for MeeGo Harmattan (Nokia N9 / N950).
//
// Upstream's src/harbour-passviewer.cpp builds a Sailfish app around
// SailfishApp and QQuickView. Qt 4.7 has neither, so this starts a
// QDeclarativeView and puts the same objects under the same names into the
// root context; the pages in meego/qml are written for com.nokia.meego and
// read them by those names.
//
// Everything below the QML is the Sailfish code in src/, compiled as it is
// except for what meego/compat catches (QJson*, QStandardPaths,
// QQuickImageProvider, sailfishapp.h) and the two pieces that are platform
// bound: notifications (meego/notificator.cpp) and the D-Bus / clipboard /
// display glue (meego/platform.cpp).
#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeError>
#include <QDeclarativeView>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsObject>
#include <QLocale>
#include <QTextCodec>
#include <QTranslator>
#include <QUrl>
#include <QtDebug>

#include <sailfishapp.h>

#include "src/settingsstore.h"
#include "src/barcodeimageprovider.h"
#include "src/zipfileimageprovider.h"
#include "src/homewatcher.h"
#include "src/datetimeformat.h"
#include "src/currencyformat.h"
#include "src/passhandler.h"
#include "src/passdb.h"
#include "notificator.h"
#include "platform.h"

// Where the QML and data lie after installation; a start from a source tree
// takes the tree instead.
static QString dataDir()
{
    const QString installed = QLatin1String("/opt/harbour-passviewer");
    if (QFileInfo(installed + "/qml/harbour-passviewer.qml").exists())
        return installed;
    const QString here = QCoreApplication::applicationDirPath();
    for (QDir dir(here); !dir.isRoot(); dir.cdUp()) {
        if (QFileInfo(dir.absolutePath() + "/meego/qml/harbour-passviewer.qml").exists())
            return dir.absolutePath() + "/meego";
    }
    return installed;
}

// The theme object the pages read their sizes and colours from. It is called
// AppTheme, not Theme: com.nokia.meego exports a Theme of its own that would
// shadow a context property of that name.
static QObject *instantiate(QDeclarativeEngine *engine, const QString &file)
{
    QDeclarativeComponent *component =
        new QDeclarativeComponent(engine, QUrl::fromLocalFile(file), engine);
    if (component->isError()) {
        const QList<QDeclarativeError> errors = component->errors();
        for (int i = 0; i < errors.size(); ++i)
            qWarning() << file << errors.at(i).toString();
        return 0;
    }
    QObject *object = component->create(engine->rootContext());
    if (object) {
        object->setParent(engine);
        QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);
    }
    return object;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Qt 4 passes tr() sources through Latin-1 unless told otherwise.
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    // The same pair the Sailfish version uses, so the settings file and the
    // pass database land where QStandardPaths of the shim puts them.
    app.setOrganizationName(QLatin1String("ch.p2501"));
    app.setApplicationName(QLatin1String("harbour-passviewer"));

    // One instance: a second start with a pass file hands the file over and
    // leaves, exactly as upstream does it.
    QDBusInterface other("ch.p2501.harbour-passviewer", "/ch/p2501/harbour_passviewer",
                         "ch.p2501.harbour_passviewer");
    if (other.isValid()) {
        QString origin;
        if (argc == 2)
            origin = QString::fromLocal8Bit(argv[1]);
        other.call("openPass", origin);
        return 0;
    }

    const QString data = dataDir();
    SailfishApp::setDataDir(data);

    QTranslator translator;
    if (translator.load(QLatin1String("harbour-passviewer-") + QLocale::system().name(),
                        data + QLatin1String("/translations")))
        app.installTranslator(&translator);

    // The objects the pages call, under upstream's names.
    SettingsStore settingsStore;
    HomeWatcher homeWatcher;
    Notificator notificator;
    DateTimeFormat dateTimeFormat;
    CurrencyFormat currencyFormat;
    PassHandler passHandler;
    PassDB passDB;
    Platform platform(app.arguments());
    new PassViewerAdaptor(&platform);
    QDBusConnection::sessionBus().registerObject("/ch/p2501/harbour_passviewer", &platform);
    QDBusConnection::sessionBus().registerService("ch.p2501.harbour-passviewer");

    QDeclarativeView view;
    view.engine()->addImageProvider(QLatin1String("barcode"), new BarcodeImageProvider());
    view.engine()->addImageProvider(QLatin1String("zipimage"), new ZipFileImageProvider());

    QDeclarativeContext *ctx = view.rootContext();
    ctx->setContextProperty(QLatin1String("settingsStore"), &settingsStore);
    ctx->setContextProperty(QLatin1String("homeWatcher"), &homeWatcher);
    ctx->setContextProperty(QLatin1String("notificator"), &notificator);
    ctx->setContextProperty(QLatin1String("dateTimeFormat"), &dateTimeFormat);
    ctx->setContextProperty(QLatin1String("currencyFormat"), &currencyFormat);
    ctx->setContextProperty(QLatin1String("passHandler"), &passHandler);
    ctx->setContextProperty(QLatin1String("passDB"), &passDB);
    ctx->setContextProperty(QLatin1String("platform"), &platform);
    // Silica's Clipboard singleton: utils.js writes Clipboard.text.
    ctx->setContextProperty(QLatin1String("Clipboard"), &platform);
    ctx->setContextProperty(QLatin1String("AppTheme"),
                            instantiate(view.engine(), data + "/qml/context/Theme.qml"));

    view.engine()->addImportPath(data + "/qml");
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.setSource(QUrl::fromLocalFile(data + "/qml/harbour-passviewer.qml"));
    if (view.status() == QDeclarativeView::Error) {
        const QList<QDeclarativeError> errors = view.errors();
        for (int i = 0; i < errors.size(); ++i)
            qWarning() << "QML:" << errors.at(i).toString();
        return 1;
    }
    ctx->setContextProperty(QLatin1String("appWindow"), static_cast<QObject *>(view.rootObject()));

    view.showFullScreen();
    return app.exec();
}
