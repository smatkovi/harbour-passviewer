import QtQuick 1.1
import com.nokia.meego 1.0
import "../lib/utils.js" as Utils

// All fields of a pass as plain text, front and back, in the app's own
// colours -- for a pass whose own colours are unreadable.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait

    property string jsondata: ''
    property string path: ''
    property string barcodeType: "qr"
    property string barcodeEncoding: "iso-8859-1"
    property string barcodeContent: ""
    property string barcodeAltText: ""

    tools: ToolBarLayout {
        ToolIcon {
            platformIconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
        ToolIcon {
            platformIconId: "toolbar-view-menu"
            anchors.right: parent.right
            onClicked: (simpleMenu.status === DialogStatus.Closed) ? simpleMenu.open() : simpleMenu.close()
        }
    }

    Menu {
        id: simpleMenu
        visualParent: pageStack
        MenuLayout {
            MenuItem {
                text: qsTr("Fullscreen Barcode")
                onClicked: pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeContent: barcodeContent, barcodeEncoding: barcodeEncoding, barcodeType: barcodeType })
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent
        pressDelay: 150
        contentHeight: body.height + AppTheme.paddingLarge * 2
        clip: true

        Column {
            id: body
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: AppTheme.horizontalPageMargin
            anchors.rightMargin: AppTheme.horizontalPageMargin
            anchors.topMargin: AppTheme.paddingLarge
            spacing: AppTheme.paddingMedium

            Image {
                source: "image://zipimage" + path + "/logo.png"
                width: sourceSize.height > 0 ? sourceSize.width * AppTheme.iconSizeSmall / sourceSize.height : 0
                height: AppTheme.iconSizeSmall
            }

            Label {
                id: logoText
                text: ''
                textFormat: Text.PlainText
                color: AppTheme.highlightColor

                MouseArea {
                    anchors.fill: parent
                    onPressAndHold: Utils.copyText(logoText.text, Clipboard, notificator);
                }
            }

            Repeater {
                model: ListModel {
                    id: frontFields
                    ListElement { title: ''; value: '' }
                }

                Column {

                    Label {
                        text: title
                        textFormat: Text.PlainText
                        font.pixelSize: AppTheme.fontSizeSmall
                        color: AppTheme.highlightColor
                        x: AppTheme.paddingLarge

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }

                    Label {
                        text: value
                        textFormat: Text.PlainText
                        color: AppTheme.highlightColor
                        width: body.width
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }
                }
            }

            Image {
                source: "image://zipimage" + path + "/footer.png"
                width: sourceSize.width != 0 ? body.width : 0
                height: sourceSize.width != 0 ? sourceSize.height * body.width / sourceSize.width : 0
            }

            Rectangle {
                visible: barcodeImage.width != 0
                width: barcodeImage.width + AppTheme.fontSizeMedium
                height: barcodeImage.height + AppTheme.fontSizeMedium
                color: 'white'

                Image {
                    id: barcodeImage
                    anchors.centerIn: parent
                    width: sourceSize.width !== 0 ? Utils.barcodeSize(sourceSize.width, sourceSize.height, body.width, AppTheme.fontSizeMedium)[0] : 0
                    height: sourceSize.height !== 0 ? Utils.barcodeSize(sourceSize.width, sourceSize.height, body.width, AppTheme.fontSizeMedium)[1] : 0
                    smooth: false
                    fillMode: Image.PreserveAspectFit
                    source: "image://barcode/" + barcodeType + "/" + barcodeEncoding + "/" + barcodeContent;
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: settingsStore.barcodeTap
                    onClicked: pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeContent: barcodeContent, barcodeEncoding: barcodeEncoding, barcodeType: barcodeType })
                }
            }

            Label {
                text: barcodeAltText
                textFormat: Text.PlainText
                color: AppTheme.highlightColor
            }

            Repeater {
                model: ListModel {
                    id: backFields
                    ListElement { title: ''; value: '' }
                }

                Column {

                    Label {
                        text: title
                        textFormat: Text.PlainText
                        font.pixelSize: AppTheme.fontSizeSmall
                        color: AppTheme.highlightColor
                        x: AppTheme.paddingLarge

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }

                    Label {
                        text: value
                        textFormat: Text.PlainText
                        font.pixelSize: AppTheme.fontSizeExtraSmall
                        color: AppTheme.highlightColor
                        width: body.width
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }
                }
            }
        }
    }

    ScrollDecorator { flickableItem: flickable }

    Component.onCompleted: {
        function setFields(pass, style, fieldType, target) {
            // set a field model from json
            if (fieldType in pass[style]) {
                for (var field = 0; field < pass[style][fieldType].length; field++) {
                    var data = pass[style][fieldType][field];
                    target.append({ title: String(data.label), value: String(data.value) });
                }
            }
        }

        // get general pass data
        var pass = JSON.parse(jsondata);
        if ('logoText' in pass)
            logoText.text = pass.logoText
        var styles = ["boardingPass", "coupon", "eventTicket", "storeCard", "generic"];
        var style = '';
        for (var key = 0; key < styles.length; key++) {
            if (styles[key] in pass) {
                style = styles[key];
                break;
            }
        }
        // complete undefined fields
        Utils.checkFields(pass, style);
        // set front field contents
        frontFields.clear();
        setFields(pass, style, 'headerFields', frontFields);
        setFields(pass, style, 'primaryFields', frontFields);
        setFields(pass, style, 'secondaryFields', frontFields);
        setFields(pass, style, 'auxiliaryFields', frontFields);
        // look for barcodes
        if (!('barcodes' in pass)) {
            pass.barcodes = [];
            if ('barcode' in pass) {
                pass.barcodes.push(pass.barcode);
            }
        }
        // paint the first useable barcode
        var validCode = false;
        for (var barcode = 0; barcode < pass.barcodes.length; barcode++) {
            switch(pass.barcodes[barcode].format.substring(15).toLowerCase()) {
            case 'code128':
            case 'qr':
            case 'aztec':
            case 'pdf417':
                barcodeContent = 'message' in pass.barcodes[barcode] ? Qt.btoa(pass.barcodes[barcode].message) : '';
                barcodeEncoding = 'messageEncoding' in pass.barcodes[barcode] ?  pass.barcodes[barcode].messageEncoding: 'iso-8859-1';
                barcodeType = pass.barcodes[barcode].format.substring(15).toLowerCase();
                barcodeAltText = 'altText' in pass.barcodes[barcode] ? pass.barcodes[barcode].altText : '';
                validCode = true;
                break;
            }
            if (validCode)
                break;
        }
        // set back field contents
        backFields.clear();
        setFields(pass, style, 'backFields', backFields);
    }
}
