# Pass Viewer for MeeGo Harmattan (Nokia N9 / N950)

The Harmattan edition of Christof Bürgi's Sailfish app lives in this
directory. `src/` is the Sailfish code, compiled as it is except for what
`meego/compat` catches (`QJson*`, `QStandardPaths`, `QQuickImageProvider`,
`sailfishapp.h`); the QML is written anew for `com.nokia.meego` because
QtQuick 1.1 has neither Silica nor its page conventions.

| | |
|---|---|
| `main.cpp` | QDeclarativeView, the context objects under upstream's names, single instance over D-Bus |
| `platform.{h,cpp}` | D-Bus adaptor `ch.p2501.harbour_passviewer`, mce display state, clipboard |
| `notificator.{h,cpp}` | MNotification instead of nemonotifications; banners are an in-app InfoBanner |
| `fetch/` | `passviewer-fetch`, a static Rust helper (reqwest + rustls) that does the HTTPS request of a pass update -- Harmattan's Qt 4 sits on OpenSSL 0.9.8 and reaches no pass server of today; the helper speaks TLS 1.2 and 1.3 |
| `qml/` | the pages for com.nokia.meego; `qml/lib/utils.js` adds `parseIso()` because Qt 4.7's JavaScript does not parse ISO 8601 |
| `tests/` | `check-qml.sh` compiles every page against generated stand-ins of the real components (`make-stubs.sh`) |

## Building

Everything runs on the build machine (`meego/remote-build.sh` pushes the
tree there): the GCC 14 cross toolchain against the MADDE sysroot for the
app, Rust with the `armv7-unknown-linux-musleabi` target for the helper
(`meego/cross.env`, `meego/build-rust.sh`), `meego/build.sh arm` and
`meego/build-deb.sh` for binary and package. Install on the device with
`aegis-dpkg -i`.

## What differs from the Sailfish version

* Passes are looked for under `MyDocs/Documents`, `MyDocs/Downloads` and
  `MyDocs/Passes`, not below the whole home directory -- walking the N9's
  vfat partition takes a minute.
* No cover, no two-column tablet layout; the back of a pass is a menu entry
  of the front.
* Distance highlighting needs the positioning daemon, which refuses
  applications without the `Location` credential; self-built packages get
  none on Harmattan, so the setting has no effect. The "prefer non-satellite"
  choice does not exist in QtMobility's PositionSource either.
* Calendar entries are not offered: there is nothing on Harmattan that opens
  an `.ics` file from `xdg-open`.
