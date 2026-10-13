// The project map: grid layers and survey line tracks; click a track to
// select it (Ctrl adds, Shift extends), double click to fit.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: page
    color: Theme.mapBackground
    readonly property var project: App.project

    function fmt(v, digits) { return Number(v).toFixed(digits) }

    MapView {
        id: map
        objectName: "mapView"
        anchors.fill: parent
        project: App.project
        backgroundColor: Theme.mapBackground
        trackColor: Theme.dark ? Qt.rgba(0.89, 0.92, 0.95, 0.75) : Qt.rgba(0.09, 0.13, 0.17, 0.7)
        selectedColor: Theme.accent
        textColor: Theme.textMuted
        gridColor: Theme.dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.08)
        onLineClicked: (id, modifiers) => {
            if (id === "") {
                if (modifiers === 0) page.project.clearSelection()
                return
            }
            page.project.select(id, (modifiers & Qt.ControlModifier) ? 1 : (modifiers & Qt.ShiftModifier) ? 2 : 0)
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !map.hasContent && !page.project.mapBusy
        iconPath: Icons.map
        title: !page.project.isOpen ? qsTr("No project open") : page.project.lineCount === 0 ? qsTr("Add survey data") : qsTr("No navigation yet")
        message: !page.project.isOpen ? qsTr("Create a project, add survey lines and see their tracks and the grids made from them here.")
                 : page.project.lineCount === 0 ? qsTr("Add swath files or a whole folder. Tracks appear as soon as the navigation has been read.")
                 : qsTr("The navigation of the survey lines is read in the background (see Jobs).")
        actionText: !page.project.isOpen ? qsTr("New project…") : page.project.lineCount === 0 ? qsTr("Add survey data…") : ""
        onAction: App.trigger(page.project.isOpen ? "ui:addData" : "ui:newProject")
    }

    BusyIndicator {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 12
        running: page.project.mapBusy
        visible: running
    }

    // tools
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        radius: Theme.radiusSmall
        color: Qt.rgba(0, 0, 0, Theme.dark ? 0.45 : 0.25)
        implicitWidth: tools.implicitWidth + 8
        implicitHeight: tools.implicitHeight + 8
        Column {
            id: tools
            anchors.centerIn: parent
            spacing: 2
            IconButton { iconPath: Icons.fit; iconColor: "#E3EAF1"; tip: qsTr("Fit to the data (double click)"); onClicked: map.fitAll() }
            IconButton { iconPath: Icons.zoomIn; iconColor: "#E3EAF1"; tip: qsTr("Zoom in"); onClicked: map.zoomBy(1.5) }
            IconButton { iconPath: Icons.zoomOut; iconColor: "#E3EAF1"; tip: qsTr("Zoom out"); onClicked: map.zoomBy(1 / 1.5) }
            IconButton {
                iconPath: Icons.route
                iconColor: map.showTracks ? Theme.accent : "#8DA2B5"
                tip: qsTr("Survey lines")
                onClicked: map.showTracks = !map.showTracks
            }
            IconButton {
                iconPath: Icons.layers
                iconColor: map.showLayers ? Theme.accent : "#8DA2B5"
                tip: qsTr("Grid layers")
                onClicked: map.showLayers = !map.showLayers
            }
        }
    }

    // hovered line
    Rectangle {
        visible: map.hoverLine !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        radius: 6
        color: Qt.rgba(0, 0, 0, 0.65)
        implicitWidth: hoverText.implicitWidth + 20
        implicitHeight: hoverText.implicitHeight + 10
        Text { id: hoverText; anchors.centerIn: parent; text: map.hoverLine; color: "white"; font.pixelSize: Theme.fontSmall + 1 }
    }

    // cursor readout
    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        visible: map.cursorInside
        radius: 6
        color: Qt.rgba(0, 0, 0, 0.6)
        implicitWidth: cursorText.implicitWidth + 20
        implicitHeight: cursorText.implicitHeight + 12
        Text {
            id: cursorText
            anchors.centerIn: parent
            color: "white"
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSmall + 1
            text: qsTr("lon %1  lat %2").arg(page.fmt(map.cursorLon, 6)).arg(page.fmt(map.cursorLat, 6))
                  + (map.cursorValue.z !== undefined ? "   z " + page.fmt(map.cursorValue.z, 2) + " " + (map.cursorValue.units || "") : "")
        }
    }
}
