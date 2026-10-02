import QtQuick 1.1
import com.nokia.meego 1.0

// The barcode alone, as large as the screen allows, for the scanner at the
// gate. A tap anywhere goes back.
Page {
    id: page
    orientationLock: PageOrientation.Automatic

    property string barcodeType: "qr"
    property string barcodeEncoding: "iso-8859-1"
    property string barcodeContent: ""

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: AppTheme.horizontalPageMargin
        anchors.rightMargin: AppTheme.horizontalPageMargin
        anchors.topMargin: AppTheme.paddingLarge
        anchors.bottomMargin: AppTheme.paddingLarge
        color: 'white'

        Image {
            anchors.fill: parent
            anchors.margins: AppTheme.paddingLarge
            smooth: false
            fillMode: Image.PreserveAspectFit
            source: "image://barcode/" + barcodeType + "/" + barcodeEncoding + "/" + barcodeContent;
        }

        MouseArea {
            anchors.fill: parent
            onClicked: pageStack.pop()
        }
    }
}
