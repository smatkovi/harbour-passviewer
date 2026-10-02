#include "homescanner.h"

HomeScanner::HomeScanner(QObject *parent) : QObject(parent)
{

}

// QDir::removeRecursively() is Qt 5; this does the same on both.
static void removeTree(const QString &path) {
    QDir dir(path);
    if (!dir.exists())
        return;
    QFileInfoList entries(dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System));
    for (QFileInfoList::const_iterator entry = entries.constBegin(); entry != entries.constEnd(); ++entry) {
        if (entry->isDir() && !entry->isSymLink())
            removeTree(entry->absoluteFilePath());
        else
            QFile::remove(entry->absoluteFilePath());
    }
    dir.rmdir(path);
}

// Only files that are ZIP archives are worth opening. Qt 5 asks the MIME
// database; Qt 4 has none, so there the file's own signature decides.
static bool looksLikeZip(const QFileInfo &entry) {
#if QT_VERSION >= 0x050000
    QMimeDatabase mime;
    return mime.mimeTypeForFile(entry).name() == "application/zip";
#else
    // Opening every file below the home directory is too slow on the N9's
    // vfat MyDocs; the suffix decides, and only for a plausible one is the
    // signature read.
    const QString suffix = entry.suffix().toLower();
    if (suffix != "zip" && suffix != "pkpasses")
        return false;
    QFile file(entry.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    return file.read(2) == "PK";
#endif
}

HomeScanner::~HomeScanner() {
    // clear and remove temporary directory
    QString tmpdir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    tmpdir += "/harbour-passviewer";
    if (QDir(tmpdir).exists())
        removeTree(tmpdir);
}

void HomeScanner::scanHome(bool update) {
    QStringList inspectPaths;
    QStringList visiblePaths;
    QStringList passPaths;
    QVariantList passes;
    static bool first = true;
#if QT_VERSION >= 0x050000
    inspectPaths.append(QDir::homePath());  // start with the home directory
    if ((!first) && QDir("/media/sdcard").exists())
        inspectPaths.append("/media/sdcard");  // also check the SD-Card (not on the first run)
#else
    // Harmattan: the whole home holds some fifteen thousand files on a vfat
    // partition, and walking them takes the better part of a minute on the
    // N9. Passes arrive by browser or mail, so Documents and Downloads are
    // where they are, plus a folder of one's own.
    const QString docs = QDir::homePath() + "/MyDocs";
    QStringList candidates;
    candidates << docs + "/Documents" << docs + "/Downloads" << docs + "/Passes" << docs + "/passes";
    for (QStringList::const_iterator it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
        if (QDir(*it).exists())
            inspectPaths.append(QDir(*it).canonicalPath());
    }
    if (inspectPaths.isEmpty())
        inspectPaths.append(docs);
#endif
    while (!inspectPaths.isEmpty()) {
        QDir current(inspectPaths.at(0));
        inspectPaths.removeFirst();
        visiblePaths.append(current.path());
        QFileInfoList entries(current.entryInfoList());
        for (auto entry = entries.constBegin(); entry != entries.constEnd(); ++entry) {
            if (entry->isHidden())  // we don't look at hidden directories or files
                continue;
            if (entry->isDir()) {
                QString dpath(entry->fileName().toLower());
                if (dpath == "tmp" || dpath == "temp")  // ignore temporary directories
                    continue;
                dpath = entry->canonicalFilePath();
                if (inspectPaths.contains(dpath) || visiblePaths.contains(dpath))  // avoid double checks
                    continue;
                inspectPaths.append(dpath);
            }
            if (entry->isFile()) {
                // look for obvious pass files and ZIP archives only
                if (entry->suffix() != "pkpass" && !looksLikeZip(*entry))
                    continue;
                QString fpath(entry->canonicalFilePath());
                QVariantMap pass = m_buildPass(fpath);
                if (!pass.isEmpty()) {
                    // singular pass
                    passPaths.append(fpath);
                    passes.append(pass);
                }
                else {
                    // maybe it's a bundle?
                    QString tmppassdir = m_unzipPassBundle(fpath);
                    if (tmppassdir != "") {
                        if (!(inspectPaths.contains(tmppassdir) || visiblePaths.contains(tmppassdir)))
                            inspectPaths.append(tmppassdir);
                    }
                }
            }
        }
    }
    emit passesFound(passes, visiblePaths, update);
    if (first) {
        // after first run, scan once more including the SD card
        first = false;
        scanHome(false);
    }
}

void HomeScanner::scanHome(QString path) {
    scanHome(false);
}

QVariantMap HomeScanner::m_buildPass(QString zipname) {
    // get json data from the file
    ZipFile zip(zipname);
    if (!zip.isValid())
        return QVariantMap();
    QString jsondata = zip.getTextFile("pass.json");
    if (jsondata == "")
        return QVariantMap();
    m_cleanJson(jsondata);
    QJsonDocument json(QJsonDocument::fromJson(jsondata.toUtf8()));
    if (json.isNull())
        return QVariantMap();
    // get the type id
    QString typeId = json.object().value("passTypeIdentifier").toString();
    if (typeId == "")
        return QVariantMap();
    // get the pass style
    QStringList styles;
    styles << "boardingPass" << "coupon" << "eventTicket" << "storeCard" << "generic";
    QString passStyle;
    for (auto style = styles.constBegin(); style != styles.constEnd(); ++style) {
        if (json.object().contains(*style)) {
            passStyle = *style;
            break;
        }
    }
    if (passStyle == "")
        return QVariantMap();
    // look for localization
    if (m_localizePass(json, zip))
        jsondata = QString(json.toJson());
    // create the pass name
    QString name;
    QJsonArray primaries(json.object().value(passStyle).toObject().value("primaryFields").toArray());
    QJsonArray secondaries(json.object().value(passStyle).toObject().value("secondaryFields").toArray());
    QJsonArray auxiliaries(json.object().value(passStyle).toObject().value("auxiliaryFields").toArray());
    if (passStyle == "boardingPass" && primaries.count() > 1)  // use parting point and destination
        name = primaries.at(0).toObject().value("value").toString() + " → " + primaries.at(1).toObject().value("value").toString();
    else if (primaries.count() > 0)
        name = primaries.at(0).toObject().value("label").toString() + " " + primaries.at(0).toObject().value("value").toString();
    else if (secondaries.count() > 0)
        name = secondaries.at(0).toObject().value("label").toString() + " " + secondaries.at(0).toObject().value("value").toString();
    else if (auxiliaries.count() > 0)
        name = auxiliaries.at(0).toObject().value("label").toString() + " " + auxiliaries.at(0).toObject().value("value").toString();
    // use the file basename if everything else fails
    if (name == "" || name == " ")
        name = QFileInfo(zipname).baseName();
    // check if the file is from a bundle
    bool bundle = zipname.startsWith(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    // check if the pass is updateable
    bool updateable = (!bundle) && json.object().contains("webServiceURL") && json.object().contains("serialNumber") && json.object().contains("authenticationToken");
    // construct and return the pass
    QVariantMap pass;
    pass.insert("name", name);
    pass.insert("path", zipname);
    pass.insert("jsondata", jsondata);
    pass.insert("typeId", typeId);
    pass.insert("bundle", bundle);
    pass.insert("updateable", updateable);
    pass.insert("mtime", QFileInfo(zipname).lastModified());
    return pass;
}

QString HomeScanner::m_unzipPassBundle(QString zipname) {
    // only check ZIP files with appropriate suffix
    if (!zipname.endsWith(".pkpasses"))
        return QString();
    ZipFile zip(zipname);
    if (!zip.isValid())
        return QString();
    // temp directory for unzipped passes
    QString tmpdir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    tmpdir += "/harbour-passviewer/" + zipname;
    if (!QDir(tmpdir).exists())
        QDir().mkpath(tmpdir);
    // check file for stored passes
    QStringList entrynames = zip.getFileList();
    bool unzipped = false;
    for (auto entry = entrynames.constBegin(); entry != entrynames.constEnd(); ++entry) {
        if (!entry->endsWith(".pkpass"))  // only check entries with the appropriate suffix
            continue;
        // if it's a pass, unzip it to temp
        QFile passfile(tmpdir + "/" + *entry);
        if (passfile.open(QIODevice::WriteOnly)) {
            passfile.write(zip.getFile(*entry));
            passfile.close();
            unzipped = true;
        }
    }
    // if we don't have files, don't leave the directory
    if (!unzipped) {
        QDir().rmpath(tmpdir);
        return QString();
    }
    // we unzipped files, so that directory has to be watched
    return tmpdir;
}

void HomeScanner::m_cleanJson(QString &data) {
    // this finds and removes commas at the end of arrays and objects (empty elements)
    QRegExp findcomma("[\\]}]\\s*,\\s*[\\]}]");
    bool done = false;
    do {
        int matchpos = findcomma.indexIn(data);
        if (matchpos >= 0) {
            int commapos = data.indexOf(',', matchpos);
            if (commapos >= 0)
                data.remove(commapos, 1);
            else
                done = true; // for safety
        }
        else
            done = true;
    } while(!done);
    // this cleans any garbage at the end
    int objend = data.lastIndexOf('}') + 1;
    if (data.size() > objend)
        data.resize(objend);
}

bool HomeScanner::m_localizePass(QJsonDocument &json, ZipFile &zip) {
    // get local language
#if QT_VERSION >= 0x050000
    QString lang(QLocale().bcp47Name());
#else
    QString lang(QLocale().name());
#endif
    if (lang == "C")
        lang = "en";
    if (lang.contains("_"))
        lang = lang.left(lang.indexOf("_"));
    // get translation file
    QString transFile(zip.getTextFile(lang + ".lproj/pass.strings"));
    if (transFile == "")
        transFile = zip.getTextFile("en.lproj/pass.strings");
    if (transFile == "")
        return false;
    // parse translation file
    QMap<QString, QString> trans;
    bool instr = false;
    bool inkey = true;
    bool escaped = false;
    bool comment = false;
    QString key;
    QString value;
    for (int pos = 0; pos < transFile.size(); pos++) {
        QChar chr = transFile.at(pos);
        if (instr) {
            if (inkey)
                key.append(chr);
            else
                value.append(chr);
            if (escaped)
                escaped = false;
            else {
                if (chr == '\\')
                    escaped = true;
                if (chr == '"')
                    instr = false;
            }
        }
        else {
            if (comment) {
                if (transFile.mid(pos, 2) == "*/")
                    comment = false;
            }
            else {
                if (chr == '"') {
                    instr = true;
                    if (inkey)
                        key = QString("[\"");
                    else
                        value = QString("[\"");
                }
                if (chr == '=')
                    inkey = false;
                if (chr == ';') {
                    key.append("]");
                    key = QJsonDocument::fromJson(key.toUtf8()).array().at(0).toString();
                    value.append("]");
                    value = QJsonDocument::fromJson(value.toUtf8()).array().at(0).toString();
                    trans.insert(key, value);
                    inkey = true;
                }
                if (transFile.mid(pos, 2) == "/*")
                    comment = true;
            }
        }
    }
    // update json
    QStringList styles;
    styles << "boardingPass" << "coupon" << "eventTicket" << "storeCard" << "generic";
    QStringList types;
    types << "headerFields" << "primaryFields" << "secondaryFields" << "auxiliaryFields" << "backFields";
    QStringList subTypes;
    subTypes << "label" << "value";
    QJsonObject docObj(json.object());
    bool docChanged = false;
    for (auto style = styles.constBegin(); style != styles.constEnd(); ++style) {
        if (!docObj.contains(*style))
            continue;
        QJsonObject styleObj(docObj.value(*style).toObject());
        bool styleChanged = false;
        for (auto type = types.constBegin(); type != types.constEnd(); ++type) {
            if (!styleObj.contains(*type))
                continue;
            QJsonArray typeArray(styleObj.value(*type).toArray());
            bool typeChanged = false;
            for (int field = 0; field < typeArray.size(); field++) {
                QJsonObject fieldObj(typeArray.at(field).toObject());
                bool fieldChanged = false;
                for (auto subType = subTypes.constBegin(); subType != subTypes.constEnd(); ++subType) {
                    if (trans.contains(fieldObj.value(*subType).toString())) {
                        fieldObj.insert(*subType, trans.value(fieldObj.value(*subType).toString()));
                        fieldChanged = true;
                    }
                }
                if (fieldChanged) {
                    typeArray.replace(field, fieldObj);
                    typeChanged = true;
                }
            }
            if (typeChanged) {
                styleObj.insert(*type, typeArray);
                styleChanged = true;
            }
        }
        if (styleChanged) {
            docObj.insert(*style, styleObj);
            docChanged = true;
        }
    }
    if (docChanged)
        json.setObject(docObj);
    return docChanged;
}
