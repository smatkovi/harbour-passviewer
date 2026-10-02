.pragma library

// Qt 4.7's JavaScriptCore does not parse ISO 8601 in new Date(string), so the
// relevant dates of the passes ("2026-10-02T09:23:13+02:00") are taken apart
// here. Returns null for anything else.
function parseIso(text) {
    var m = /^(\d{4})-(\d{2})-(\d{2})(?:T(\d{2}):(\d{2})(?::(\d{2}))?(?:\.\d+)?(Z|[+-]\d{2}:?\d{2})?)?$/.exec(String(text));
    if (!m)
        return null;
    var hours = m[4] !== undefined ? parseInt(m[4], 10) : 0;
    var minutes = m[5] !== undefined ? parseInt(m[5], 10) : 0;
    var seconds = m[6] !== undefined ? parseInt(m[6], 10) : 0;
    var ms = Date.UTC(parseInt(m[1], 10), parseInt(m[2], 10) - 1, parseInt(m[3], 10), hours, minutes, seconds);
    if (m[7] !== undefined && m[7] !== "Z") {
        var sign = m[7].charAt(0) === "-" ? -1 : 1;
        var digits = m[7].substring(1).replace(":", "");
        var offset = parseInt(digits.substring(0, 2), 10) * 60 + parseInt(digits.substring(2, 4), 10);
        ms -= sign * offset * 60000;
    }
    else if (m[4] !== undefined && m[7] === undefined) {
        // no zone given: local time, as the pass author meant it
        var local = new Date(parseInt(m[1], 10), parseInt(m[2], 10) - 1, parseInt(m[3], 10), hours, minutes, seconds);
        ms = local.getTime();
    }
    return new Date(ms);
}

function checkSpecificFields(pass, style, fieldType) {
    // adds undefined entries if necessary
    for (var field = 0; field < pass[style][fieldType].length; field++) {
        if (!('key' in pass[style][fieldType][field]))
            pass[style][fieldType][field].key = '';
        if (!('label' in pass[style][fieldType][field]))
            pass[style][fieldType][field].label = '';
        if (!('value' in pass[style][fieldType][field]))
            pass[style][fieldType][field].value = '';
    }
}

function checkFields(pass, style) {
    // adds undefined entries if necessary
    if (!('primaryFields' in pass[style]))
        pass[style].primaryFields = [];
    if (!('secondaryFields' in pass[style]))
        pass[style].secondaryFields = [];
    if (!('auxiliaryFields' in pass[style]))
        pass[style].auxiliaryFields = [];
    if (!('backFields' in pass[style]))
        pass[style].backFields = [];
    checkSpecificFields(pass, style, 'primaryFields');
    checkSpecificFields(pass, style, 'secondaryFields');
    checkSpecificFields(pass, style, 'auxiliaryFields');
    checkSpecificFields(pass, style, 'backFields');
}

function htmlescape(text) {
    // HTML escaping
    return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}


function setFields(pass, style, fieldType, target, dateTimeFormat, currencyFormat) {
    // sets a field model from json
    target.clear();
    var fields = [];
    if (style in pass && fieldType in pass[style]) {
        for (var field = 0; field < pass[style][fieldType].length; field++) {
            var data = pass[style][fieldType][field];
            // handle datetime fields
            if ("dateStyle" in data && "timeStyle" in data) {
                var dateFormat = data.dateStyle.substring(11).toLowerCase();
                var timeFormat = data.timeStyle.substring(11).toLowerCase();
                var ignoreTimeZone = false;
                if ('ignoresTimeZone' in data)
                    ignoreTimeZone = data.ignoresTimeZone;
                data.value = dateTimeFormat.format(data.value, dateFormat, timeFormat, ignoreTimeZone);
            }
            // handle currency fields
            else if ("currencyCode" in data) {
                var currencyCode = data.currencyCode.substring(0,3).toUpperCase();
                data.value = currencyFormat.format(data.value, currencyCode);
            }

            target.append({ field: String(data.key), title: String(data.label), value: htmlescape(String(data.value)) });
        }
    }
}

function interpretColor(color) {
    // turn passbook color descriptions into QML color descriptions
    if (color.substring(0,3) !== 'rgb')
        return color;
    var components = color.match(/\d+/g);
    var code = '#';
    for (var cur = 0; cur < 3; cur++) {
        var value = parseInt(components[cur]);
        if (isNaN(value) || value < 0)
            value = 0;
        if (value > 0xff)
            value = 0xff;
        if (value < 0x10)
            code += '0';
        code += value.toString(16);
    }
    return code;
}

function barcodeSize(origWidth, origHeight, maxWidth, em) {
    // find the optimal barcode size
    var width = 0;
    var height = 0;
    var scale = 0;
    // leave 0.5em border, but the barcode should be about 4em high
    while (width + em <= maxWidth && height < em * 4) {
        scale++;
        width = origWidth * scale;
        height = origHeight * scale;
    }
    if (width + em > maxWidth && scale > 1) {
        scale--;
        width = origWidth * scale;
        height = origHeight * scale;
    }
    return [width, height];
}

function copyText(text, clipboard, notificator) {
    clipboard.text = text;
    notificator.bannerNotification(qsTr("copied to clipboard"), "");
}
