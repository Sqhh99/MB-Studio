// MB-Studio visual design tokens (colours, spacing, typography).
//
// Neutral dark-grey and light-grey themes with compact rows, fine
// separators and small corner radii; the accent marks selection, focus and
// the active tool, and strong status colours are kept for warnings and
// failures, so that the data views carry the colour.
pragma Singleton
import QtQuick

QtObject {
    property bool dark: true

    readonly property color background: dark ? "#1E1F22" : "#F2F3F5"
    readonly property color sidebar: dark ? "#191A1C" : "#E8E9EC"
    readonly property color surface: dark ? "#25272A" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#2B2D31" : "#F5F6F8"
    readonly property color surfaceHover: dark ? "#33363B" : "#E6E8EB"
    readonly property color border: dark ? "#383B40" : "#D4D7DC"
    readonly property color text: dark ? "#DFE1E5" : "#1F2328"
    readonly property color textMuted: dark ? "#9A9EA6" : "#5A616B"
    readonly property color textFaint: dark ? "#6C7078" : "#8E949C"

    readonly property color accent: dark ? "#4C9BE8" : "#2E7BD1"   // selection, focus, active tool
    readonly property color accentStrong: dark ? "#3A84CF" : "#2468B5"
    readonly property color accentSoft: dark ? "#263A50" : "#DCE9F7"
    readonly property color success: "#3FA66B"
    readonly property color warning: "#D99A2B"
    readonly property color danger: "#D9534F"
    readonly property color info: "#4C9BE8"

    // workbench chrome
    readonly property color titleBar: dark ? "#161719" : "#E1E3E6"
    readonly property color titleBarInactive: dark ? "#1C1D20" : "#ECEDEF"
    readonly property color panel: dark ? "#222326" : "#F7F8FA"
    readonly property color panelHeader: dark ? "#26282B" : "#ECEDF0"
    readonly property color tabActive: dark ? "#2C2E32" : "#FFFFFF"
    readonly property color selection: dark ? "#2D4A66" : "#CFE2F7"
    readonly property color mapBackground: dark ? "#141517" : "#E3E5E8"
    readonly property color consoleBackground: dark ? "#141517" : "#1F2328"
    readonly property color consoleText: "#D7DADF"
    readonly property color overlay: dark ? Qt.rgba(0.08, 0.08, 0.09, 0.82) : Qt.rgba(1, 1, 1, 0.88)
    readonly property color overlayText: dark ? "#E6E8EB" : "#1F2328"

    readonly property int radius: 4
    readonly property int radiusSmall: 3
    readonly property int spacing: 8
    readonly property int pagePadding: 12
    readonly property int panelPadding: 10
    readonly property int rowHeight: 26
    readonly property int titleBarHeight: 34
    readonly property int toolBarHeight: 32

    readonly property int fontSmall: 11
    readonly property int fontBody: 12
    readonly property int fontTitle: 15
    readonly property int fontHeadline: 20
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

    // colour of a survey line's processing status
    function lineStatusColor(status, missing) {
        if (missing) return danger
        if (status === "processed") return success
        if (status === "edits-pending" || status === "corrections-pending" || status === "processing-required") return warning
        return textFaint
    }

    // colour of a product's status
    function productStatusColor(status) {
        if (status === "outdated") return danger
        if (status === "pending") return warning
        if (status === "current") return success
        return textFaint
    }
}
