import QtQuick 1.1

// Silica's Theme for Harmattan. main.cpp puts this object in the root context
// as "AppTheme" -- not as "Theme", because com.nokia.meego exports a Theme of
// its own that beats a context property of the same name, which is what left
// an earlier port of another app completely unstyled.
//
// The numbers are the Sailfish defaults at their 540 px reference width,
// scaled to the N9's 480, so the pages keep the proportions they were drawn
// with. Die Farben: helle Schrift auf Schwarz, Hervorhebung in Blau wie die Harmattan-Oberflaeche.
QtObject {
    property real screenWidth: 480
    property real screenHeight: 854
    property real ratio: screenWidth / 540

    property real paddingSmall: 6 * ratio
    property real paddingMedium: 12 * ratio
    property real paddingLarge: 24 * ratio
    property real horizontalPageMargin: 24 * ratio

    property real itemSizeExtraSmall: 70 * ratio
    property real itemSizeSmall: 80 * ratio
    property real itemSizeMedium: 100 * ratio
    property real itemSizeLarge: 110 * ratio
    property real itemSizeExtraLarge: 135 * ratio
    property real itemSizeHuge: 180 * ratio

    property real iconSizeSmall: 32 * ratio
    property real iconSizeMedium: 48 * ratio
    property real iconSizeLarge: 64 * ratio
    property real iconSizeExtraLarge: 86 * ratio
    property real iconSizeLauncher: 86 * ratio

    property real fontSizeTiny: 16 * ratio
    property real fontSizeExtraSmall: 20 * ratio
    property real fontSizeSmall: 24 * ratio
    property real fontSizeMedium: 28 * ratio
    property real fontSizeLarge: 36 * ratio
    property real fontSizeExtraLarge: 44 * ratio
    property real fontSizeHuge: 60 * ratio

    // Dieselben Farben, die ThemedPage in seine palette legt -- sonst haette
    // der Port zwei Quellen fuer dieselbe Frage, und die Seiten lesen aus
    // beiden: helle Schrift aus dem Thema auf hellem Grund aus der Palette
    // ist genau das, was man dann sieht (naemlich nichts).
    property color primaryColor: "#f2f2f2"
    property color secondaryColor: "#a9a9a9"
    property color highlightColor: "#3d9bff"
    property color secondaryHighlightColor: "#8ac2ff"
    property color highlightBackgroundColor: "#3d9bff"
    property real highlightBackgroundOpacity: 0.3
    property color errorColor: "#ff4d4d"
    property color backgroundColor: "#000000"

    // Silica has light and dark schemes; this port is dark throughout, so the
    // constant is here only so that the pages can name it.
    property int colorScheme: 0
    property int lightOnDark: 0
    property int darkOnLight: 1

    function rgba(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    // On Silica this picks a background tone to go with the given colour and
    // scheme; one dark scheme needs no choice.
    function highlightBackgroundFromColor(color, scheme) {
        return color
    }
}
