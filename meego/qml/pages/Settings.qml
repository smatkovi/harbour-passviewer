import QtQuick 1.1
import com.nokia.meego 1.0

// The settings, one for one as on Sailfish: Silica's ComboBox becomes a
// button that opens a SelectionDialog, its TextSwitch a Switch with a label.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
    }

    SelectionDialog {
        id: sortDialog
        titleText: qsTr("Sort by")
        selectedIndex: settingsStore.sortBy
        model: ListModel {
            ListElement { name: "Relevancy" }
            ListElement { name: "Event Date" }
            ListElement { name: "File Date" }
            ListElement { name: "Event Name" }
        }
        onSelectedIndexChanged: settingsStore.sortBy = selectedIndex
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        pressDelay: 150
        contentHeight: body.height + 2 * AppTheme.paddingLarge
        clip: true

        Column {
            id: body
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: AppTheme.horizontalPageMargin
            anchors.rightMargin: AppTheme.horizontalPageMargin
            anchors.topMargin: AppTheme.paddingLarge
            spacing: AppTheme.paddingMedium

            Label {
                anchors.right: parent.right
                text: qsTr("Sort")
                color: AppTheme.highlightColor
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Sort by")
                }

                Button {
                    text: {
                        switch (settingsStore.sortBy) {
                        case 1: return qsTr("Event Date");
                        case 2: return qsTr("File Date");
                        case 3: return qsTr("Event Name");
                        default: return qsTr("Relevancy");
                        }
                    }
                    onClicked: sortDialog.open()
                }
            }

            Label {
                anchors.right: parent.right
                text: qsTr("Time")
                color: AppTheme.highlightColor
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium
                Switch {
                    id: time
                    anchors.verticalCenter: parent.verticalCenter
                    checked: settingsStore.checkTime
                    onCheckedChanged: settingsStore.checkTime = checked
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - time.width - parent.spacing
                    wrapMode: Text.Wrap
                    text: qsTr("Highlight passes close to event time")
                }
            }

            Label {
                visible: time.checked
                text: qsTr("Time before event") + ": " + hoursBefore.value + "h"
                font.pixelSize: AppTheme.fontSizeSmall
                color: AppTheme.secondaryColor
            }

            Slider {
                id: hoursBefore
                width: parent.width
                enabled: time.checked
                visible: time.checked
                stepSize: 1
                minimumValue: 0
                maximumValue: 8
                value: settingsStore.hoursBefore
                valueIndicatorVisible: true
                onPressedChanged: if (!pressed) settingsStore.hoursBefore = value
            }

            Label {
                visible: time.checked
                text: qsTr("Time after event") + ": " + hoursAfter.value + "h"
                font.pixelSize: AppTheme.fontSizeSmall
                color: AppTheme.secondaryColor
            }

            Slider {
                id: hoursAfter
                width: parent.width
                enabled: time.checked
                visible: time.checked
                stepSize: 1
                minimumValue: 0
                maximumValue: 8
                value: settingsStore.hoursAfter
                valueIndicatorVisible: true
                onPressedChanged: if (!pressed) settingsStore.hoursAfter = value
            }

            Label {
                anchors.right: parent.right
                text: qsTr("Distance")
                color: AppTheme.highlightColor
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium
                Switch {
                    id: distance
                    anchors.verticalCenter: parent.verticalCenter
                    checked: settingsStore.checkDistance
                    onCheckedChanged: settingsStore.checkDistance = checked
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - distance.width - parent.spacing
                    wrapMode: Text.Wrap
                    text: qsTr("Highlight passes close to destination")
                }
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium
                visible: distance.checked
                Switch {
                    id: useHere
                    anchors.verticalCenter: parent.verticalCenter
                    checked: settingsStore.useHere
                    onCheckedChanged: settingsStore.useHere = checked
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - useHere.width - parent.spacing
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: qsTr("Prefer non-satellite position fixing")
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: AppTheme.fontSizeExtraSmall
                        color: AppTheme.secondaryColor
                        textFormat: Text.StyledText
                        text: qsTr("Saves battery. <b>Requires</b> &quot;Faster position fix&quot; switched on in the system settings.")
                    }
                }
            }

            Label {
                visible: distance.checked
                text: qsTr("Distance to destination") + ": " + maxDistance.value + "m"
                font.pixelSize: AppTheme.fontSizeSmall
                color: AppTheme.secondaryColor
            }

            Slider {
                id: maxDistance
                width: parent.width
                enabled: distance.checked
                visible: distance.checked
                stepSize: 50
                minimumValue: 100
                maximumValue: 2000
                value: settingsStore.maxDistance
                valueIndicatorVisible: true
                onPressedChanged: if (!pressed) settingsStore.maxDistance = value
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium
                visible: distance.checked
                Switch {
                    id: override
                    anchors.verticalCenter: parent.verticalCenter
                    checked: settingsStore.overrideDistance
                    onCheckedChanged: settingsStore.overrideDistance = checked
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - override.width - parent.spacing
                    wrapMode: Text.Wrap
                    text: qsTr("Allow passes to override distance")
                }
            }

            Label {
                anchors.right: parent.right
                text: qsTr("Barcode")
                color: AppTheme.highlightColor
            }

            Row {
                width: parent.width
                spacing: AppTheme.paddingMedium
                Switch {
                    id: barcodeTap
                    anchors.verticalCenter: parent.verticalCenter
                    checked: settingsStore.barcodeTap
                    onCheckedChanged: settingsStore.barcodeTap = checked
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - barcodeTap.width - parent.spacing
                    wrapMode: Text.Wrap
                    text: qsTr("Show barcode fullscreen when tapped")
                }
            }
        }
    }

    ScrollDecorator { flickableItem: flickable }
}
