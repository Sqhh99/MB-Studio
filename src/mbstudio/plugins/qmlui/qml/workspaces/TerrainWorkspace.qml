// A 3D terrain document: the grid as a lit surface (TerrainViewModel). The
// tools are in the tool options bar, display and measurement in the
// properties panel.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: page
    property var document: null
    readonly property var terrain: document
    color: Theme.mapBackground

    TerrainView {
        id: view
        objectName: "terrainView"
        anchors.fill: parent
        model: page.terrain
        markerColor: Theme.warning
    }

    EmptyState {
        anchors.centerIn: parent
        visible: page.terrain === null || (!page.terrain.loaded && !page.terrain.busy)
        iconPath: Icons.cube
        title: page.terrain !== null && page.terrain.error !== "" ? qsTr("The grid could not be read") : qsTr("3D terrain view")
        message: page.terrain !== null && page.terrain.error !== "" ? page.terrain.error
                 : qsTr("Open a grid of the project in 3D from its menu or from the grid view.")
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: page.terrain !== null && page.terrain.busy
        visible: running
    }

    // camera and value under the cursor
    Rectangle {
        visible: page.terrain !== null && page.terrain.loaded
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: readout.implicitWidth + 16
        implicitHeight: readout.implicitHeight + 10
        Text {
            id: readout
            anchors.centerIn: parent
            color: Theme.overlayText
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSmall
            text: page.terrain === null ? ""
                  : (page.terrain.statusText !== "" ? page.terrain.statusText + "    " : "")
                    + qsTr("azimuth %1°  elevation %2°  ×%3").arg(Math.round(page.terrain.yaw)).arg(Math.round(page.terrain.pitch)).arg(Number(page.terrain.exaggeration).toFixed(1))
                    + (page.terrain.perspective ? "" : "  " + qsTr("orthographic"))
        }
    }
    // legend
    Rectangle {
        visible: page.terrain !== null && page.terrain.loaded
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: 190
        implicitHeight: legendColumn.implicitHeight + 10
        ColumnLayout {
            id: legendColumn
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2
            Row {
                id: legendBar
                Layout.fillWidth: true
                height: 9
                readonly property var stops: page.terrain !== null ? page.terrain.legend : []
                Repeater {
                    model: Math.max(0, legendBar.stops.length - 1)
                    Rectangle {
                        width: legendBar.width / (legendBar.stops.length - 1)
                        height: legendBar.height
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: legendBar.stops[index].color }
                            GradientStop { position: 1; color: legendBar.stops[index + 1].color }
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: page.terrain !== null ? Number(page.terrain.info.zmin || 0).toFixed(1) : ""; color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
                Item { Layout.fillWidth: true }
                Text { text: page.terrain !== null ? Number(page.terrain.info.zmax || 0).toFixed(1) + " " + (page.terrain.info.zUnits || "") : ""; color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
            }
        }
    }
    // measurement
    Rectangle {
        visible: page.terrain !== null && page.terrain.markerCount === 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.margins: 8
        radius: Theme.radius
        color: Theme.overlay
        implicitWidth: measureText.implicitWidth + 16
        implicitHeight: measureText.implicitHeight + 10
        Text {
            id: measureText
            anchors.centerIn: parent
            color: Theme.overlayText
            font.pixelSize: Theme.fontSmall + 1
            text: page.terrain === null || page.terrain.measurement.horizontal === undefined ? ""
                  : qsTr("A–B  %1 m horizontal  ·  Δz %2 m  ·  slope %3°")
                        .arg(Number(page.terrain.measurement.horizontal).toFixed(1))
                        .arg(Number(page.terrain.measurement.dz).toFixed(2))
                        .arg(Number(page.terrain.measurement.slopeDegrees).toFixed(1))
        }
    }
}
