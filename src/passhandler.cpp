#include "passhandler.h"

PassHandler::PassHandler(QObject *parent) :
    QObject(parent),
    m_network(),
    m_replies()
{
#if QT_VERSION >= 0x050000
    QObject::connect(&m_network, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
#endif
}

QString PassHandler::getCanonicalPath(QString path) {
    QFileInfo pass(path);
    if (pass.isFile())
        return pass.canonicalFilePath();
    return path;
}

void PassHandler::removePass(QString path) {
    QFile(path).remove();
}

void PassHandler::updatePass(QString path) {
    // get the infos needed for the update
    ZipFile passFile(path);
    if (!passFile.isValid()) {
        emit updateFinished("not updateable");
        return;
    }
    QJsonDocument pass(QJsonDocument::fromJson(passFile.getTextFile("pass.json").toUtf8()));
    QString baseURL(pass.object().value("webServiceURL").toString());
    QString passID(pass.object().value("passTypeIdentifier").toString());
    QString serial(pass.object().value("serialNumber").toString());
    QString auth(pass.object().value("authenticationToken").toString());
    if (baseURL == "" || passID == "" || serial == "" || auth == "") {
        emit updateFinished("not updateable");
        return;
    }
    QUrl url(baseURL + "/v1/passes/" + passID + "/" + serial);
    PassDB db;
    PassInfo* info = db.getPassInfo(passID + "/" + serial);
    QByteArray since;
    if (!info->updated().isNull())
        since = m_rfc2616(info->updated());
    delete info;
#if QT_VERSION >= 0x050000
    // build and send the HTTP request
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QByteArray("ApplePass ") + auth.toUtf8());
    if (!since.isEmpty())
        request.setRawHeader("If-Modified-Since", since);
    QNetworkReply* reply = m_network.get(request);
    m_replies.insert(reply, path);
#else
    // Qt 4 on Harmattan sits on OpenSSL 0.9.8, which no pass server of
    // today accepts; the request goes through passviewer-fetch (meego/fetch),
    // a static Rust helper with its own TLS, that writes the answer to a file.
    QTemporaryFile* tmp = new QTemporaryFile(QDir::tempPath() + "/passviewer-update-XXXXXX", this);
    if (!tmp->open()) {
        delete tmp;
        emit updateFinished("update failed");
        return;
    }
    tmp->close();
    QProcess* fetch = new QProcess(this);
    Fetch job;
    job.path = path;
    job.file = tmp;
    m_fetches.insert(fetch, job);
    connect(fetch, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(fetchFinished(int,QProcess::ExitStatus)));
    connect(fetch, SIGNAL(error(QProcess::ProcessError)), this, SLOT(fetchFailed(QProcess::ProcessError)));
    QStringList arguments;
    arguments << url.toString() << auth << tmp->fileName() << QString::fromLatin1(since);
    fetch->start(QCoreApplication::applicationDirPath() + "/passviewer-fetch", arguments);
#endif
}

#if QT_VERSION >= 0x050000
void PassHandler::replyFinished(QNetworkReply *reply) {
    // check what this is an answer to
    QString path(m_replies.value(reply));
    if (path == "")
        return;
    m_replies.remove(reply);
    reply->deleteLater();
    // Code 304 -> not modified since last update
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 304) {
        emit updateFinished("no new version");
        return;
    }
    // anything else is a network error
    if (reply->error() != QNetworkReply::NoError) {
        emit updateFinished("update failed");
        return;
    }
    // load the reply into a temporary file
    QTemporaryFile tmp;
    if (!tmp.open()) {
        emit updateFinished("update failed");
        return;
    }
    m_copyFile(tmp, *reply);
    tmp.close();
    m_installUpdate(path, tmp.fileName());
}
#else
void PassHandler::fetchFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess* fetch = qobject_cast<QProcess*>(sender());
    if (!fetch || !m_fetches.contains(fetch))
        return;
    Fetch job = m_fetches.take(fetch);
    fetch->deleteLater();
    QString state("update failed");
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        // {"status": 200} / {"status": 304} / {"error": "..."}
        QJsonDocument report(QJsonDocument::fromJson(fetch->readAllStandardOutput().trimmed()));
        int status = report.object().value("status").toInt();
        if (status == 304)
            state = "no new version";
        else if (status == 200)
            state = "";
        else
            qWarning("passviewer-fetch: %s", qPrintable(report.object().value("error").toString()));
    }
    if (state.isEmpty())
        m_installUpdate(job.path, job.file->fileName());
    else
        emit updateFinished(state);
    delete job.file;
}

void PassHandler::fetchFailed(QProcess::ProcessError error) {
    // The helper could not even be started (missing, not executable).
    QProcess* fetch = qobject_cast<QProcess*>(sender());
    if (!fetch || !m_fetches.contains(fetch) || error != QProcess::FailedToStart)
        return;
    Fetch job = m_fetches.take(fetch);
    fetch->deleteLater();
    delete job.file;
    emit updateFinished("update failed");
}
#endif

// The downloaded pass is in `downloaded`: check it, note what changed, and
// put it in place of the old file. Shared by both ways of fetching.
void PassHandler::m_installUpdate(const QString &path, const QString &downloaded) {
    ZipFile zip(downloaded);
    if (!zip.isValid()) {
        emit updateFinished("update failed");
        return;
    }
    // get the ID for the pass DB
    QJsonDocument json(QJsonDocument::fromJson(zip.getTextFile("pass.json").toUtf8()));
    QString id = json.object().value("passTypeIdentifier").toString() + "/" + json.object().value("serialNumber").toString();
    // get changes to former version
    QFile passFile(path);
    QStringList changes;
    if (passFile.exists())
        changes = getChanges(path, downloaded);
    // overwrite former version
    QFile source(downloaded);
    if (source.open(QFile::ReadOnly) && passFile.open(QFile::WriteOnly)) {
        m_copyFile(passFile, source);
        passFile.close();
        // update pass DB
        PassDB db;
        PassInfo* info = new PassInfo(id, QDateTime::currentDateTime(), changes);
        db.setPassInfo(info);
        delete info;
        emit updateFinished("ok");
    }
    else {
        emit updateFinished("update failed");
    }
}

QMap<QString, QVariant> PassHandler::getFields(QString filename) {
    // get all header, primary, secondary and auxiliary fields from a pass
    QMap<QString, QVariant> fields;
    ZipFile zip(filename);
    if (!zip.isValid())
        return fields;
    QJsonDocument pass(QJsonDocument::fromJson(zip.getTextFile("pass.json").toUtf8()));
    QStringList styles;
    styles << "boardingPass" << "coupon" << "eventTicket" << "storeCard" << "generic";
    QStringList types;
    types << "headerFields" << "primaryFields" << "secondaryFields" << "auxiliaryFields";
    for (auto style = styles.constBegin(); style != styles.constEnd(); ++style) {
        for (auto type = types.constBegin(); type != types.constEnd(); ++type) {
            QJsonArray thisFields(pass.object().value(*style).toObject().value(*type).toArray());
            for (int entry = 0; entry < thisFields.size(); entry++) {
                QJsonObject field(thisFields.at(entry).toObject());
                if (field.contains("key") && field.contains("value"))
                    fields.insert(field.value("key").toString(), field.value("value").toVariant());
            }
        }
    }
    return fields;
}

QStringList PassHandler::getChanges(QString oldfile, QString newfile) {
    // compare the fields from two passes
    QStringList changed;
    QMap<QString, QVariant> oldfields(getFields(oldfile));
    QMap<QString, QVariant> newfields(getFields(newfile));
    for (auto newfield = newfields.constBegin(); newfield != newfields.constEnd(); ++newfield) {
        if (!oldfields.contains(newfield.key()) || oldfields.value(newfield.key()) != newfield.value())
            changed.append(newfield.key());
    }
    return changed;
}

void PassHandler::createCalendarEntry(QString subject, QString isoDateTime) {
    // get the datetime
    QDateTime dateTime(QDateTime::fromString(isoDateTime, Qt::ISODate));
    if (!dateTime.isValid()) {
        emit calendarEntryFinished("format");
        return;
    }
    // create a temporary iCal file
    QString icaldir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    if (!QDir(icaldir).exists())
        QDir().mkpath(icaldir);
    QFile ical(icaldir + "/passbook.ics");
    if (ical.open(QFile::WriteOnly)) {
        QString w_subject(subject);
        ical.write("BEGIN:VCALENDAR\r\n");
        ical.write("VERSION:2.0\r\n");
        ical.write("PRODID:-//p2501.ch//Pass Viewer 0.10//EN\r\n");
        ical.write("BEGIN:VEVENT\r\n");
        ical.write("UID:" + w_subject.replace(' ', '_').toUtf8() + "/" + isoDateTime.toUtf8() + "/harbour-passviewer\r\n");
        ical.write("DTSTAMP:" + QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ").toUtf8() + "\r\n");
        ical.write("DTSTART:" + dateTime.toUTC().toString("yyyyMMddTHHmmssZ").toUtf8() + "\r\n");
        ical.write("SUMMARY:" + subject.toUtf8() + "\r\n");
        ical.write("TRANSP:TRANSPARENT\r\n");
        ical.write("END:VEVENT\r\n");
        ical.write("END:VCALENDAR\r\n");
        ical.close();
        QDesktopServices::openUrl(QUrl("file:/" + ical.fileName()));
    }
}

void PassHandler::m_copyFile(QIODevice &to, QIODevice &from) {
    for (QByteArray data(from.read(4096)); data.size() > 0; data = from.read(4096))
        to.write(data);
}

QByteArray PassHandler::m_rfc2616(QDateTime datetime) {
    // RFC2616 is similar to RFC2822, but requires the weekday, requires UTC, and the timezone must be written as "GMT"
    // "Sun, 06 Nov 1994 08:49:37 GMT"; spelt out so that the month names are
    // English whatever the locale, on Qt 4 as on Qt 5.
    QDateTime utc(datetime.toUTC());
    QStringList weekdays;
    weekdays << "NULL" << "Mon" << "Tue" << "Wed" << "Thu" << "Fri" << "Sat" << "Sun";
    QStringList months;
    months << "NULL" << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
    QString dateString = weekdays.at(utc.date().dayOfWeek()) + ", "
        + utc.toString("dd") + " " + months.at(utc.date().month()) + " " + utc.toString("yyyy HH:mm:ss") + " GMT";
    return dateString.toUtf8();
}
