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
    readonly property color info: "#5B9BF0"

    // workbench chrome
    readonly property color panel: dark ? "#111A23" : "#F8FAFC"
    readonly property color panelHeader: dark ? "#16212C" : "#EEF2F6"
    readonly property color tabActive: dark ? "#1B2836" : "#FFFFFF"
    readonly property color selection: dark ? "#1A4152" : "#D3EEF3"
    readonly property color mapBackground: dark ? "#081018" : "#DFE6EC"
    readonly property color console: dark ? "#0A1016" : "#1D2630"
    readonly property color consoleText: "#D5E2EE"

    readonly property int radius: 10
    readonly property int radiusSmall: 6
    readonly property int spacing: 12
    readonly property int pagePadding: 24
    readonly property int panelPadding: 12
    readonly property int rowHeight: 30

    readonly property int fontSmall: 11
    readonly property int fontBody: 13
    readonly property int fontTitle: 17
    readonly property int fontHeadline: 24
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas"
                                     : Qt.platform.os === "osx" ? "Menlo" : "DejaVu Sans Mono"

    // colour of a job or history state
    function stateColor(state) {
        if (state === "completed") return success
        if (state === "failed") return danger
        if (state === "cancelled") return textFaint
        if (state === "running") return accent
        return warning
    }

    function levelColor(level) {
        if (level === "error") return danger
        if (level === "warning") return warning
        if (level === "debug") return textFaint
        return accent
    }
}
