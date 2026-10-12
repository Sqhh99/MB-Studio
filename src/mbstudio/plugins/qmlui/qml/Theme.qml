// MB-Studio visual design tokens (colours, spacing, typography).
pragma Singleton
import QtQuick

QtObject {
    property bool dark: true

    readonly property color background: dark ? "#0E151D" : "#F3F5F8"
    readonly property color sidebar: dark ? "#0A1118" : "#E8EDF2"
    readonly property color surface: dark ? "#151F2A" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#1B2836" : "#F0F3F7"
    readonly property color surfaceHover: dark ? "#223244" : "#E6EBF1"
    readonly property color border: dark ? "#243344" : "#DCE2E9"
    readonly property color text: dark ? "#E3EAF1" : "#17212B"
    readonly property color textMuted: dark ? "#8497AA" : "#5E6E7E"
    readonly property color textFaint: dark ? "#5D6F81" : "#93A1AF"

    readonly property color accent: "#1FB6C9"        // ocean teal
    readonly property color accentStrong: "#1E88E5"  // sea blue
    readonly property color accentSoft: dark ? "#163845" : "#DDF4F7"
    readonly property color success: "#2EBD85"
    readonly property color warning: "#F2A93B"
    readonly property color danger: "#E5534B"

    readonly property int radius: 10
    readonly property int radiusSmall: 6
    readonly property int spacing: 12
    readonly property int pagePadding: 24

    readonly property int fontSmall: 11
    readonly property int fontBody: 13
    readonly property int fontTitle: 17
    readonly property int fontHeadline: 24
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas"
                                     : Qt.platform.os === "osx" ? "Menlo" : "DejaVu Sans Mono"

    function levelColor(level) {
        if (level === "error") return danger
        if (level === "warning") return warning
        if (level === "debug") return textFaint
        return accent
    }
}
