import QtQuick 1.1
import com.nokia.meego 1.0

// The front of a pass. Sailfish attaches the back as a page to the right;
// here the back is a menu entry, as are the fullscreen barcode, the simple
// view and the update.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    property string name: ''
    property string jsondata: ''
    property string path: ''
    property bool updateable: false

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
        ToolIcon {
            platformIconId: "toolbar-view-menu"
            anchors.right: parent.right
            onClicked: (passMenu.status === DialogStatus.Closed) ? passMenu.open() : passMenu.close()
        }
    }

    Menu {
        id: passMenu
        visualParent: pageStack
        MenuLayout {
            MenuItem {
                text: qsTr("Fullscreen Barcode")
                onClicked: pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeContent: pass.item.barcodeContent, barcodeEncoding: pass.item.barcodeEncoding, barcodeType: pass.item.barcodeType })
            }
            MenuItem {
                text: qsTr("Back Side")
                onClicked: pageStack.push(Qt.resolvedUrl("ShowBack.qml"), { path: page.path, jsondata: page.jsondata })
            }
            MenuItem {
                text: qsTr("Simple View")
                onClicked: pageStack.push(Qt.resolvedUrl("ShowSimple.qml"), { path: pass.item.path, jsondata: pass.item.jsondata })
            }
            MenuItem {
                text: qsTr("Update")
                visible: updateable
                onClicked: passHandler.updatePass(page.path)
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
            source: Qt.resolvedUrl("../lib/Pass.qml")
            onLoaded: {
                item.path = path
                item.jsondata = jsondata
            }
        }
    }

    ScrollDecorator { flickableItem: flickable }
}
