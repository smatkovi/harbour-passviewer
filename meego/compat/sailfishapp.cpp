// The data directory behind the sailfishapp.h shim; set once by main().
#include "sailfishapp.h"

namespace SailfishApp {
    static QString s_dataDir;
    QString dataDir() { return s_dataDir; }
    void setDataDir(const QString &dir) { s_dataDir = dir; }
}
