import QtQuick 1.1
// Only the API surface the pages use; see meego/tests/stubs/QtMobility/location/qmldir.
Item {
    property bool active
    property int updateInterval
    property bool valid
    property variant position
    property string nmeaSource
    signal positionChanged()
    function update() {}
    function start() {}
    function stop() {}
}
