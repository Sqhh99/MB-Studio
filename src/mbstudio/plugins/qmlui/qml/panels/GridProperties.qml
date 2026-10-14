// Properties of a grid document: display (colour table, range, shading),
// coverage and quality, coordinate system and extent.
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
    spacing: 6

    function fmt(v, digits) { return v === undefined || v === null || isNaN(v) ? "–" : Number(v).toFixed(digits) }

    Text {
        text: root.doc !== null ? root.doc.fileName : ""
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }
    Text {
        visible: text !== ""
        text: root.info.title || ""
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Size"); value: (root.info.nx || 0) + " × " + (root.info.ny || 0) }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Coordinates"); value: root.info.projectionName || "" }

    SectionTitle { text: qsTr("Display"); iconPath: Icons.palette; Layout.topMargin: 8 }
    ComboBox {
        objectName: "gridColorMap"
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
        Text { text: root.fmt(root.doc !== null && !root.doc.autoRange ? root.doc.rangeMin : root.info.zmin, 1); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        Item { Layout.fillWidth: true }
        Text { text: root.fmt(root.doc !== null && !root.doc.autoRange ? root.doc.rangeMax : root.info.zmax, 1) + " " + (root.info.zUnits || ""); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    }
    Switch {
        text: qsTr("Colour range from the data")
        checked: root.doc !== null && root.doc.autoRange
        onToggled: root.doc.autoRange = checked
    }
    RowLayout {
        visible: root.doc !== null && !root.doc.autoRange
        Layout.fillWidth: true
        spacing: 6
        TextField {
            Layout.fillWidth: true
            text: root.doc !== null ? root.fmt(root.doc.rangeMin, 1) : ""
            selectByMouse: true
            validator: RegularExpressionValidator { regularExpression: /-?\d{0,7}([.,]\d{0,4})?/ }
            onEditingFinished: root.doc.rangeMin = Number(text.replace(",", "."))
        }
        Text { text: "…"; color: Theme.textMuted }
        TextField {
            Layout.fillWidth: true
            text: root.doc !== null ? root.fmt(root.doc.rangeMax, 1) : ""
            selectByMouse: true
            validator: RegularExpressionValidator { regularExpression: /-?\d{0,7}([.,]\d{0,4})?/ }
            onEditingFinished: root.doc.rangeMax = Number(text.replace(",", "."))
        }
    }
    Switch {
        text: qsTr("Hillshade")
        checked: root.doc !== null && root.doc.hillshade
        onToggled: root.doc.hillshade = checked
    }
    Text { text: qsTr("Light azimuth  %1°").arg(root.doc !== null ? Math.round(root.doc.azimuth) : 0); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; visible: root.doc !== null && root.doc.hillshade }
    Slider {
        Layout.fillWidth: true
        visible: root.doc !== null && root.doc.hillshade
        from: 0; to: 359; stepSize: 1
        value: root.doc !== null ? root.doc.azimuth : 315
        onMoved: root.doc.azimuth = value
    }
    Text { text: qsTr("Light elevation  %1°").arg(root.doc !== null ? Math.round(root.doc.elevation) : 0); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; visible: root.doc !== null && root.doc.hillshade }
    Slider {
        Layout.fillWidth: true
        visible: root.doc !== null && root.doc.hillshade
        from: 5; to: 90; stepSize: 1
        value: root.doc !== null ? root.doc.elevation : 45
        onMoved: root.doc.elevation = value
    }
    Text { text: qsTr("Vertical exaggeration  ×%1").arg(root.doc !== null ? root.doc.exaggeration.toFixed(1) : "1"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; visible: root.doc !== null && root.doc.hillshade }
    Slider {
        Layout.fillWidth: true
        visible: root.doc !== null && root.doc.hillshade
        from: 0.5; to: 20; stepSize: 0.5
        value: root.doc !== null ? root.doc.exaggeration : 1
        onMoved: root.doc.exaggeration = value
    }

    // coverage and quality: measured, interpolated and empty cells
    SectionTitle { text: qsTr("Coverage and quality"); iconPath: Icons.check; Layout.topMargin: 8 }
    PropertyRow {
        Layout.fillWidth: true
        label: qsTr("Empty cells")
        value: root.doc !== null && root.doc.statistics.emptyPercent !== undefined ? root.fmt(root.doc.statistics.emptyPercent, 1) + " %" : ""
    }
    PropertyRow {
        Layout.fillWidth: true
        visible: root.doc !== null && root.doc.hasQualityGrids
        label: qsTr("Interpolated")
        value: root.doc !== null ? root.fmt(root.doc.interpolatedPercent, 1) + " %" : ""
    }
    PropertyRow {
        Layout.fillWidth: true
        label: qsTr("Mean ± σ")
        value: root.doc !== null && root.doc.statistics.mean !== undefined ? root.fmt(root.doc.statistics.mean, 2) + " ± " + root.fmt(root.doc.statistics.stddev, 2) : ""
    }
    Text {
        visible: root.doc !== null && !root.doc.hasQualityGrids
        Layout.fillWidth: true
        text: qsTr("Grid with quality grids (mbgrid -M) to see data counts, standard deviations and interpolated cells.")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }

    SectionTitle { text: qsTr("Extent"); iconPath: Icons.info; Layout.topMargin: 8 }
    PropertyRow { Layout.fillWidth: true; label: root.info.xName || "x"; value: root.fmt(root.info.xmin, 6) + " … " + root.fmt(root.info.xmax, 6); mono: true }
    PropertyRow { Layout.fillWidth: true; label: root.info.yName || "y"; value: root.fmt(root.info.ymin, 6) + " … " + root.fmt(root.info.ymax, 6); mono: true }
    PropertyRow { Layout.fillWidth: true; label: "z"; value: root.fmt(root.info.zmin, 2) + " … " + root.fmt(root.info.zmax, 2); mono: true }
    PropertyRow { Layout.fillWidth: true; label: qsTr("spacing"); value: root.fmt(root.info.dx, 6) + " / " + root.fmt(root.info.dy, 6); mono: true }
    Flow {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 4
        Button { text: qsTr("Open in 3D"); flat: true; onClicked: App.openTerrain(root.doc.path, 1) }
        Button {
            text: qsTr("Add to project")
            flat: true
            visible: App.project.isOpen
            onClicked: App.project.addGrid(root.doc.path)
        }
        Button { text: qsTr("Show in folder"); flat: true; onClicked: App.showInFolder(root.doc.path) }
    }
}
