#include "homewatcher.h"

HomeWatcher::HomeWatcher(QObject *parent) :
    QFileSystemWatcher(parent),
    m_worker()
{
    HomeScanner* scanner = new HomeScanner;
    scanner->moveToThread(&m_worker);
    connect(&m_worker, SIGNAL(finished()), scanner, SLOT(deleteLater()));
    connect(this, SIGNAL(workerScanHome(bool)), scanner, SLOT(scanHome(bool)));
    connect(this, SIGNAL(directoryChanged(QString)), scanner, SLOT(scanHome(QString)));
    connect(scanner, SIGNAL(passesFound(QVariantList,QStringList,bool)), this, SLOT(passesFoundByWorker(QVariantList,QStringList,bool)));
    m_worker.start();
}

HomeWatcher::~HomeWatcher() {
    m_worker.quit();
    m_worker.wait();
}

void HomeWatcher::updatePaths(const QStringList &paths) {
    // clear the old list
    if (!directories().isEmpty())
        removePaths(directories());
    // add the new ones
    addPaths(paths);
}

void HomeWatcher::scanHome(bool update) {
    emit workerScanHome(update);
}

void HomeWatcher::passesFoundByWorker(QVariantList list, QStringList paths, bool update) {
    // remove vanished passes from the pass DB
    PassDB db;
    QStringList oldids = db.getPassIDs();
    for (auto pass = list.constBegin(); pass != list.constEnd(); ++pass) {
        QJsonDocument json(QJsonDocument::fromJson(pass->toMap().value("jsondata").toString().toUtf8()));
        oldids.removeAll(json.object().value("passTypeIdentifier").toString() + "/" + json.object().value("serialNumber").toString());
    }
    for (auto oldid = oldids.constBegin(); oldid != oldids.constEnd(); ++oldid) {
        db.deletePassInfo(*oldid);
    }
    // make sure we watch all paths
    updatePaths(paths);
    // signal the found passes
    emit passesFound(list, update);
}
