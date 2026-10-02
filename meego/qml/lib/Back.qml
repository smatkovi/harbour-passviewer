import QtQuick 1.1
import com.nokia.meego 1.0
import 'utils.js' as Utils

// The back of a pass: its back fields in the pass's colours. Upstream's
// qml/lib/Back.qml with QtQuick 1.1 spellings.
Rectangle {
    id: root
    property string jsondata: ''
    property color backgroundColor: 'white'
    property color labelColor: 'black'
    property color textColor: 'black'
    property alias backFields: backFieldsModel

    height: body.height + AppTheme.paddingMedium * 2
    color: backgroundColor

    Column {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: AppTheme.paddingMedium
        spacing: AppTheme.paddingMedium

        Repeater {
            model: ListModel {
                id: backFieldsModel
                ListElement { title: ''; value: '' }
            }

            Column {

                Label {
                    text: title
                    textFormat: Text.PlainText
                    color: labelColor
                    font.pixelSize: AppTheme.fontSizeExtraSmall

                    MouseArea {
                        anchors.fill: parent
                        onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                    }
                }

                Label {
                    text: value
                    textFormat: Text.StyledText
                    color: textColor
                    font.pixelSize: AppTheme.fontSizeExtraSmall
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

    onJsondataChanged: {
        //clear old data
        backgroundColor = 'white';
        labelColor = 'black';
        textColor = 'black';
        backFields.clear();

        if (jsondata === '')
            return;

        // get general pass info
        var pass = JSON.parse(jsondata);
        if ('backgroundColor' in pass)
            backgroundColor = Utils.interpretColor(pass.backgroundColor);
        if ('labelColor' in pass)
            labelColor = Utils.interpretColor(pass.labelColor);
        if ('foregroundColor' in pass)
            textColor = Utils.interpretColor(pass.foregroundColor);
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
        // set field contents
        Utils.setFields(pass, style, 'backFields', backFields, dateTimeFormat, currencyFormat);
    }
}
