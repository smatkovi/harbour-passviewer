// Loads every QML file of meego/qml through a QDeclarativeEngine and reports
// what does not compile: the typos, the QtQuick 2 spellings, the Silica
// properties com.nokia.meego does not have. The components of
// com.nokia.meego, com.nokia.extras and QtMobility.location are stand-ins
// from meego/tests/stubs with the real API surface, because the real ones
// need the device (or at least a display).
//
// Only compilation is checked, never a running page: the context properties
// the pages read (settingsStore, passDB, ...) are device objects and are not
// needed to compile. meego/tests/check-qml.sh builds and runs this.
#include <QApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeError>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <cstdio>

static QObject *instantiate(QDeclarativeEngine *engine, const QString &file)
{
    QDeclarativeComponent *component =
        new QDeclarativeComponent(engine, QUrl::fromLocalFile(file), engine);
    QObject *object = component->create(engine->rootContext());
    if (object) {
        object->setParent(engine);
        QDeclarativeEngine::setObjectOwnership(object, QDeclarativeEngine::CppOwnership);
    }
    return object;
}

int main(int argc, char *argv[])
{
    // No GUI: the checker never shows a window, and the build machine has
    // no display.
    QApplication app(argc, argv, false);

    const QString qmlDir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString("meego/qml");
    const QString stubs = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QString("meego/tests/stubs");

    QDeclarativeEngine engine;
    engine.addImportPath(stubs);
    engine.addImportPath(qmlDir);

    QDeclarativeContext *context = engine.rootContext();

    QVariantMap pageOrientation;
    pageOrientation.insert("Automatic", 0);
    pageOrientation.insert("LockPortrait", 1);
    pageOrientation.insert("LockLandscape", 2);
    context->setContextProperty("PageOrientation", pageOrientation);

    QVariantMap dialogStatus;
    dialogStatus.insert("Opening", 0);
    dialogStatus.insert("Open", 1);
    dialogStatus.insert("Closing", 2);
    dialogStatus.insert("Closed", 3);
    context->setContextProperty("DialogStatus", dialogStatus);

    QVariantMap theme;
    theme.insert("inverted", true);
    context->setContextProperty("theme", theme);

    context->setContextProperty("AppTheme", instantiate(&engine, qmlDir + "/context/Theme.qml"));

    // Every file, so that a page nobody reaches from the root is checked too.
    QStringList files;
    files << qmlDir + "/harbour-passviewer.qml";
    const char *subdirs[] = {"pages", "lib", 0};
    for (const char **sub = subdirs; *sub; sub++) {
        QDir dir(qmlDir + "/" + *sub);
        const QStringList names = dir.entryList(QStringList() << "*.qml", QDir::Files, QDir::Name);
        for (int i = 0; i < names.size(); ++i)
            files << dir.absoluteFilePath(names.at(i));
    }

    int failed = 0;
    for (int i = 0; i < files.size(); ++i) {
        const QString file = files.at(i);
        QDeclarativeComponent component(&engine, QUrl::fromLocalFile(file));
        if (component.isError()) {
            failed++;
            printf("FAIL   %s\n", qPrintable(QFileInfo(file).fileName()));
            const QList<QDeclarativeError> errors = component.errors();
            for (int e = 0; e < errors.size(); ++e)
                printf("       %s\n", qPrintable(errors.at(e).toString()));
            continue;
        }
        printf("ok     %s\n", qPrintable(QFileInfo(file).fileName()));
    }

    printf("\n%d of %d files failed to load\n", failed, files.size());
    return failed == 0 ? 0 : 1;
}
