import QtQuick 1.1
import com.nokia.meego 1.0
import QtMobility.location 1.2
import "../lib/utils.js" as Utils

// The pass list. The relevancy, sorting and model bookkeeping below is
// upstream's qml/pages/FirstPage.qml line for line; what differs is the
// chrome -- a toolbar instead of a pull-down menu, a context menu that opens
// on a long press, a query dialog instead of a remorse timer -- and that the
// wide (tablet) layout and the cover are gone. The D-Bus pieces (the
// single-instance hook and mce's display state) are C++ here, see
// meego/platform.h.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    property string uid: "firstPage"
    property bool displayOn: platform.displayOn

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-view-menu"
            anchors.right: parent.right
            onClicked: (mainMenu.status === DialogStatus.Closed) ? mainMenu.open() : mainMenu.close()
        }
    }

    Menu {
        id: mainMenu
        visualParent: pageStack
        MenuLayout {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("Settings.qml"))
            }
            MenuItem {
                text: qsTr("Copyright")
                onClicked: pageStack.push(Qt.resolvedUrl("Copyright.qml"))
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        pressDelay: 150

        model: ListModel {
            id: passList
            ListElement { name: ""; relevantDate: ""; path: ""; points: -1; jsondata: ""; typeId: ""; bundle: false; updateable: false; mtime: "" }
        }

        delegate: Item {
            id: entry
            width: listView.width
            height: passIcon.height + AppTheme.paddingSmall * 2

            Rectangle {
                anchors.fill: parent
                color: AppTheme.highlightColor
                opacity: entryArea.pressed ? 0.3 : 0
            }

            Image {
                id: passIcon
                width: AppTheme.iconSizeLauncher
                height: width
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: AppTheme.horizontalPageMargin
                source: "image://zipimage" + path + "/icon.png"
            }

            Label {
                text: name
                textFormat: Text.PlainText
                width: parent.width - passIcon.width - AppTheme.horizontalPageMargin * 2 - AppTheme.paddingMedium
                elide: Text.ElideRight
                color: points != -1 ? AppTheme.primaryColor : AppTheme.secondaryColor
                anchors.left: passIcon.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: AppTheme.paddingMedium
            }

            Label {
                text: relevantDate
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignRight
                font.pixelSize: AppTheme.fontSizeTiny
                width: parent.width - passIcon.width - AppTheme.horizontalPageMargin * 2 - AppTheme.paddingMedium
                elide: Text.ElideRight
                color: points != -1 ? AppTheme.primaryColor : AppTheme.secondaryColor
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: AppTheme.horizontalPageMargin
                anchors.bottomMargin: AppTheme.paddingSmall
            }

            MouseArea {
                id: entryArea
                anchors.fill: parent
                onClicked: openPass(path, false)
                onPressAndHold: {
                    entryMenu.entryPath = path;
                    entryMenu.entryUpdateable = updateable;
                    entryMenu.entryBundle = bundle;
                    entryMenu.open();
                }
            }
        }

        ScrollDecorator { flickableItem: listView }
    }

    // The context menu of a list entry; one for the whole list, pointed at the
    // entry that was held.
    ContextMenu {
        id: entryMenu
        property string entryPath: ""
        property bool entryUpdateable: false
        property bool entryBundle: false
        visualParent: pageStack

        MenuLayout {
            MenuItem {
                text: qsTr("Show")
                onClicked: {
                    var pass = getPass(entryMenu.entryPath);
                    if (pass !== null)
                        showPass(pass, false);
                }
            }
            MenuItem {
                text: qsTr("Update")
                visible: entryMenu.entryUpdateable
                onClicked: passHandler.updatePass(entryMenu.entryPath)
            }
            MenuItem {
                text: qsTr("Delete")
                visible: !entryMenu.entryBundle
                onClicked: {
                    deleteDialog.delPath = entryMenu.entryPath;
                    deleteDialog.open();
                }
            }
        }
    }

    QueryDialog {
        id: deleteDialog
        property string delPath: ""
        titleText: qsTr("Delete")
        message: qsTr("Deleting")
        acceptButtonText: qsTr("Delete")
        rejectButtonText: qsTr("Cancel")
        onAccepted: {
            passHandler.removePass(delPath);
            for (var entry = 0; entry < passList.count; entry++) {
                if (passList.get(entry).path === delPath) {
                    passList.remove(entry);
                    break;
                }
            }
        }
    }

    BusyIndicator {
        id: busy
        anchors.centerIn: parent
        platformStyle: BusyIndicatorStyle { size: "large" }
        running: true
        visible: running
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("No passes found")
        color: AppTheme.highlightColor
        visible: passList.count == 0 && !busy.running
    }

    Timer {
        id: checkTimer
        interval: 60000
        repeat: true
        onTriggered: checkPassList()
    }

    // QtMobility's PositionSource has no choice of positioning method, so
    // the "prefer non-satellite" setting and the precision switch of the
    // Sailfish version have nothing to act on here; `precise` is kept so
    // that the logic below reads as upstream's.
    PositionSource {
        id: locator
        property bool precise: false
        active: settingsStore.checkDistance && page.displayOn
        updateInterval: 60000
        onPositionChanged: checkPassList()
    }

    // QtMobility has no coordinate factory in JS; the target of a distance
    // check is this element, with its latitude and longitude set each time.
    Coordinate {
        id: target
    }

    Component.onCompleted: {
        // initial pass scan
        passList.clear();
        homeWatcher.scanHome();
    }

    Connections {
        target: homeWatcher
        onPassesFound: {
            // Qt 4.7 hands the QVariantList in as copies: a property written
            // on list[i] is gone with the next access. So the passes are
            // copied into plain objects first, and everything below works on
            // those (upstream writes on the list directly).
            var passes = [];
            for (var i = 0; i < list.length; i++) {
                var p = list[i];
                passes.push({ name: p.name, path: p.path, jsondata: p.jsondata, typeId: p.typeId, bundle: p.bundle, updateable: p.updateable, mtime: p.mtime, points: -1, relevantDate: "", rdate: null });
            }
            // check for vanished passes...
            var removePasses = [];
            for (var oldpass = 0; oldpass < passList.count; oldpass++) {
                var found = false;
                for (var newpass = 0; newpass < passes.length; newpass++) {
                    if (passList.get(oldpass).path === passes[newpass].path) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    removePasses.push(passList.get(oldpass).path);
            }
            // ...and remove them
            for (var toRemove = 0; toRemove < removePasses.length; toRemove++)
                removePass(removePasses[toRemove]);
            // calculate the points sort the list, and update GPS precision
            var close = false;
            for (var pass = 0; pass < passes.length; pass++) {
                if (calcPointsAndTime(passes[pass]))
                    close = true;
            }
            passes.sort(comparePasses);
            if (locator.precise !== close)
                locator.precise = close;
            // update the pass list
            updatePasses(passes);
            // on the first run: stop the busy animation, start the check timer and show the pass called in the CLI (if given)
            if (busy.running) {
                busy.running = false;
                checkTimer.start();
                if (platform.arguments.length === 2)
                    openPass(platform.arguments[1]);
            }
            // report a successful update and redraw the pass, if it's shown
            if (update) {
                notificator.bannerNotification(qsTr("pass update successful"), "");
                if (pageStack.depth > 1 && pageStack.currentPage.path !== undefined)
                    openPass(pageStack.currentPage.path);
            }
        }
    }

    Connections {
        target: settingsStore
        onSortByChanged: checkPassList()
        onCheckTimeChanged: checkPassList()
        onHoursBeforeChanged: checkPassList()
        onHoursAfterChanged: checkPassList()
        onCheckDistanceChanged: checkPassList()
        onMaxDistanceChanged: checkPassList()
        onOverrideDistanceChanged: checkPassList()
    }

    Connections {
        target: passHandler
        onUpdateFinished: {
            switch (state) {
            case "not updateable":
                notificator.bannerNotification(qsTr("pass not updateable"), "");
                break;
            case "no new version":
                notificator.bannerNotification(qsTr("no new version for pass"), "");
                break;
            case "update failed":
                notificator.bannerNotification(qsTr("pass update failed"), "");
                break;
            case "ok":
                homeWatcher.scanHome(true);
            }
        }
        onCalendarEntryFinished: {
            if (state === "format")
                notificator.bannerNotification(qsTr("Format Error"), qsTr("Couldn't recognize date/time format"));
            if (state === "xdg-open")
                notificator.bannerNotification(qsTr("Unsupported"), qsTr("Please update your system or install calendar"));
        }
    }

    Connections {
        target: appWindow
        onOpenPass: {
            openPass(origin);
        }
    }

    function showPass(pass, immediate) {
        var properties = { name: passList.get(pass).name, path: passList.get(pass).path, jsondata: passList.get(pass).jsondata, updateable: passList.get(pass).updateable };
        pageStack.pop(page, true);
        pageStack.push(Qt.resolvedUrl("ShowPass.qml"), properties, immediate);
    }

    function openPass(origin, immediate) {
        if (typeof immediate === 'undefined')
            immediate = true;
        // get the canonical path
        origin = passHandler.getCanonicalPath(origin);
        // look for a matching pass
        var pass = getPass(origin);
        if (pass !== null)
            showPass(pass, immediate);
    }

    // What goes into the model: the same object without rdate, which may be
    // null, and QtQuick 1.1's ListModel refuses a null value.
    function modelEntry(p) {
        return { name: p.name, relevantDate: p.relevantDate, path: p.path, points: p.points, jsondata: p.jsondata, typeId: p.typeId, bundle: p.bundle, updateable: p.updateable, mtime: p.mtime };
    }

    function updatePasses(newpasses) {
        // inserts, updates or moves the passes in the model
        for (var pass = 0; pass < newpasses.length; pass++) {
            var oldpoints = -1;
            if (pass < passList.count && passList.get(pass).path === newpasses[pass].path) {
                // update
                oldpoints = passList.get(pass).points;
                passList.set(pass, modelEntry(newpasses[pass]));
            }
            else {
                // check if it's further down
                var moved = false;
                for(var oldpass = pass + 1; oldpass < passList.count; oldpass++) {
                    if (passList.get(oldpass).path === newpasses[pass].path) {
                        // move and update
                        oldpoints = passList.get(oldpass).points;
                        passList.move(oldpass, pass, 1);
                        passList.set(pass, modelEntry(newpasses[pass]));
                        moved = true;
                        break;
                    }
                }
                if (!moved)
                    passList.insert(pass, modelEntry(newpasses[pass]));  // new pass
            }
            // update pass notifications
            if (oldpoints === -1 && newpasses[pass].points !== -1)
                notificator.addNotification(newpasses[pass].path, newpasses[pass].name, '');
            if (oldpoints !== -1 && newpasses[pass].points === -1)
                notificator.removeNotification(newpasses[pass].path);
        }
    }

    function getPass(path) {
        // gets the pass with the given path
        for (var pass = 0; pass < passList.count; pass++) {
            if (passList.get(pass).path === path)
                return pass;
        }
        return null;
    }

    function removePass(path) {
        // removes the pass with the given path
        var pass = getPass(path);
        if (pass !== null) {
            notificator.removeNotification(passList.get(pass).path);
            passList.remove(pass);
        }
    }

    function calcPointsAndTime(pass) {
        // calculates the relevancy points of a pass and says whether we're close to target coordinates
        // gets the relevant date and time if available
        /* Lower numbers are more relevant, but -1 means "not active".
           This is because "null" is not allowed in models. */
        pass.points = -1;
        var data = JSON.parse(pass.jsondata);
        var close = false;
        if ("relevantDate" in data) {
            pass.relevantDate = dateTimeFormat.format(data.relevantDate, "medium", "short", false);
            pass.rdate = Utils.parseIso(data.relevantDate);
        }
        else {
            pass.relevantDate = "";
            pass.rdate = null;
        }
        if (settingsStore.checkTime && "relevantDate" in data) {
            // close to target time?
            var targetTime = Utils.parseIso(data.relevantDate);
            if (targetTime === null) {
                notificator.removeNotification(pass.path);
                return false;  // faulty pass
            }
            var now = new Date();
            var timeDiff = targetTime - now;  // time difference in milliseconds
            if (timeDiff >= 0 && timeDiff <= settingsStore.hoursBefore * 3600000) {
                pass.points = timeDiff / 1000;
            }
            else if (timeDiff < 0 && Math.abs(timeDiff) <= settingsStore.hoursAfter * 3600000) {
                pass.points = Math.abs(timeDiff) / 1000;
            }
        }
        if (pass.points === -1 && settingsStore.checkDistance && "locations" in data && locator.position.latitudeValid && locator.position.longitudeValid) {
            // close to one of the target destinations?
            var here = locator.position.coordinate;
            try {
                for (var location = 0; location < data.locations.length; location++) {
                    target.latitude = data.locations[location].latitude;
                    target.longitude = data.locations[location].longitude;
                    var posDiff = here.distanceTo(target);  // distance in meter
                    var maxDistance = settingsStore.maxDistance;
                    if (settingsStore.overrideDistance && "maxDistance" in data)
                        maxDistance = data.maxDistance;
                    if (posDiff <= maxDistance && (pass.points === -1 || pass.points > posDiff))
                        pass.points = posDiff;
                    if (posDiff <= maxDistance + 1000)
                        close = true;  // close enough to always check GPS
                }
            }
            catch (e) {
                notificator.removeNotification(pass.path);
                return false;  // faulty pass
            }
            if (pass.points !== -1)
                pass.points += 36000; // close to target time is more relevant than close to destination
        }
        return close;
    }

    function comparePasses(a, b) {
        switch(settingsStore.sortBy) {
        case 0:  // relevancy
            // sort active passes to the top
            // "smaller" passes get sorted upwards
            if (a.points !== -1 && b.points === -1)
                return -1;
            if (a.points === -1 && b.points !== -1)
                return 1;
            // if both are active, check who's more relevant
            if (a.points !== b.points)
                return a.points - b.points;
            // group by pass type ID
            if (a.typeId !== b.typeId)
                return a.typeId.localeCompare(b.typeId);
            // otherwise order by name
            return a.name.localeCompare(b.name);
        case 1:  // event date
            if (a.rdate !== null) {
                if (b.rdate !== null)
                    return b.rdate - a.rdate;
                else
                    return b.mtime - a.rdate;
            }
            else {
                if (b.rdate !== null)
                    return b.rdate - a.mtime;
                else
                    return b.mtime - a.mtime;
            }
        case 2:  // file date
            return b.mtime - a.mtime;
        case 3: // event name
            return a.name.localeCompare(b.name);
        default:
            return 0;
        }
    }

    function checkPassList() {
        // recalculates all relevancy points, reorders the list and updates GPS precision
        var passes = [];
        var close = false;
        for (var pass = 0; pass < passList.count; pass++) {
            // we work with a copy
            var modelPass = passList.get(pass);
            var thisPass = { name: modelPass.name, relevantDate: modelPass.relevantDate, path: modelPass.path, points: modelPass.points, jsondata: modelPass.jsondata, typeId: modelPass.typeId, bundle: modelPass.bundle, updateable: modelPass.updateable, mtime: modelPass.mtime };
            if (calcPointsAndTime(thisPass))
                close = true;
            passes.push(thisPass);
        }
        passes.sort(comparePasses);
        if (locator.precise !== close)
            locator.precise = close;
        updatePasses(passes);
    }
}
