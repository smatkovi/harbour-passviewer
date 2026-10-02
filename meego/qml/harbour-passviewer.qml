import QtQuick 1.1
import com.nokia.meego 1.0
import com.nokia.extras 1.1
import "pages"

// The window of the Harmattan edition. Sailfish's ApplicationWindow carries a
// cover and the topmost pass for it; Harmattan has no cover, so what is left
// is the page stack and the two things the pages share: the signal a second
// start sends in, and the banner the notificator asks for.
PageStackWindow {
    id: appWindow
    showStatusBar: true
    showToolBar: true

    signal openPass(string origin)

    initialPage: FirstPage { }

    Component.onCompleted: theme.inverted = true

    function passClicked(origin) {
        openPass(origin);
    }

    // What the Sailfish version hands to a system banner.
    InfoBanner {
        id: banner
        topMargin: 40
    }

    Connections {
        target: notificator
        onBannerRequested: {
            banner.text = body !== "" ? summary + ": " + body : summary;
            banner.show();
        }
    }

    // A second instance started with a pass file, or a tapped notification.
    Connections {
        target: platform
        onPassRequested: openPass(origin)
    }
}
