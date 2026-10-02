import QtQuick 1.1
import com.nokia.meego 1.0

// The back of a pass: its back fields.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    property string jsondata: ''
    property string path: ''

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
        ToolIcon {
            platformIconId: "toolbar-view-menu"
            anchors.right: parent.right
            onClicked: (backMenu.status === DialogStatus.Closed) ? backMenu.open() : backMenu.close()
        }
    }

    Menu {
        id: backMenu
        visualParent: pageStack
        MenuLayout {
            MenuItem {
                text: qsTr("Simple View")
                onClicked: pageStack.push(Qt.resolvedUrl("ShowSimple.qml"), { path: path, jsondata: jsondata })
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        pressDelay: 150
        contentHeight: pass.item ? pass.item.height + AppTheme.paddingLarge * 2 : 0
        clip: true

        Loader {
            id: pass
            width: parent.width - 2 * AppTheme.horizontalPageMargin
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: AppTheme.paddingLarge
            source: Qt.resolvedUrl("../lib/Back.qml")
            onLoaded: {
                item.jsondata = jsondata
            }
        }
    }

    ScrollDecorator { flickableItem: flickable }
}
