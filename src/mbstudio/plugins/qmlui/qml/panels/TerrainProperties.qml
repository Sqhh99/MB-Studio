// Properties of a 3D terrain document: display, the value under the
// cursor, measurement and profile.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ColumnLayout {
    id: root
    property var doc: null
    readonly property var info: doc !== null ? doc.info : ({})
    readonly property var m: doc !== null ? doc.measurement : ({})
    spacing: 6

    function fmt(v, digits) { return v === undefined || v === null || isNaN(v) ? "–" : Number(v).toFixed(digits) }
    function distance(metres) {
        if (metres === undefined) return "–"
        return metres >= 10000 ? (metres / 1000).toFixed(2) + " km" : metres.toFixed(1) + " m"
    }

    Text {
        text: root.doc !== null ? root.doc.fileName : ""
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Size"); value: (root.info.nx || 0) + " × " + (root.info.ny || 0) }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Extent"); value: root.fmt(root.info.widthKm, 2) + " × " + root.fmt(root.info.heightKm, 2) + " km" }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Coordinates"); value: root.info.projection || "" }

    SectionTitle { text: qsTr("Display"); iconPath: Icons.palette; Layout.topMargin: 8 }
    ComboBox {
        Layout.fillWidth: true
        implicitHeight: 30
        model: root.doc !== null ? root.doc.colorMaps : []
        currentIndex: root.doc !== null ? root.doc.colorMap : 0
        onActivated: (index) => root.doc.colorMap = index
    }
    Row {
        id: legendBar
        Layout.fillWidth: true
        height: 12
        readonly property var stops: root.doc !== null ? root.doc.legend : []
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
        Text { text: root.fmt(root.info.zmin, 1); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        Item { Layout.fillWidth: true }
        Text { text: root.fmt(root.info.zmax, 1) + " " + (root.info.zUnits || ""); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    }
    Switch {
        text: qsTr("Values are depths (positive down)")
        checked: root.doc !== null && root.doc.depthsPositive
        onToggled: root.doc.depthsPositive = checked
    }
    Switch {
        visible: root.doc !== null && root.doc.hasQualityGrids
        text: qsTr("Stipple interpolated cells")
        checked: root.doc !== null && root.doc.showInterpolated
        onToggled: root.doc.showInterpolated = checked
    }
    Text {
        Layout.fillWidth: true
        text: root.doc !== null && root.doc.hasQualityGrids
              ? qsTr("Cells without data are holes; stippled cells were filled by interpolation, they are not measured.")
              : qsTr("Cells without data are holes. Grid with quality grids (mbgrid -M) to see interpolated cells.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }

    SectionTitle { text: qsTr("Measurement"); iconPath: Icons.ruler; Layout.topMargin: 8 }
    Text {
        visible: root.doc !== null && root.doc.markerCount < 2
        Layout.fillWidth: true
        text: root.doc !== null && root.doc.tool === 0 ? qsTr("Choose the measure or profile tool and click two points on the surface.")
                                                       : qsTr("Click two points on the surface.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
    PropertyRow { visible: root.m.horizontal !== undefined; Layout.fillWidth: true; label: qsTr("Horizontal"); value: root.distance(root.m.horizontal) }
    PropertyRow { visible: root.m.dz !== undefined; Layout.fillWidth: true; label: qsTr("Height difference"); value: root.fmt(root.m.dz, 2) + " " + (root.info.zUnits || "m") }
    PropertyRow { visible: root.m.slopeDistance !== undefined; Layout.fillWidth: true; label: qsTr("Slope distance"); value: root.distance(root.m.slopeDistance) }
    PropertyRow { visible: root.m.slopeDegrees !== undefined; Layout.fillWidth: true; label: qsTr("Mean slope"); value: root.fmt(root.m.slopeDegrees, 2) + "°" }
    TrackPlot {
        objectName: "terrainProfile"
        visible: root.doc !== null && root.doc.profile.length > 1
        Layout.fillWidth: true
        Layout.preferredHeight: 150
        points: root.doc !== null ? root.doc.profile : []
        geographic: false
        invertY: root.doc !== null && root.doc.depthsPositive
        xLabel: qsTr("distance (m)")
        lineColor: Theme.accent
        axisColor: Theme.border
        textColor: Theme.textMuted
    }
    Button {
        visible: root.doc !== null && root.doc.markerCount > 0
        text: qsTr("Clear the points")
        flat: true
        onClicked: root.doc.clearMarkers()
    }
}
