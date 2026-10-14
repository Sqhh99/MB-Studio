// Properties of a figure document: the recipe, its state and making it
// again or changing its settings.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ColumnLayout {
    id: root
    property string assetId: ""
    signal panelRequested(string kind)
    readonly property var info: assetId !== "" && App.project.mapRevision >= 0 ? App.project.productInfo(assetId) : ({})
    spacing: 6

    Text {
        text: root.info.label || ""
        color: Theme.text
        font.pixelSize: Theme.fontTitle - 2
        font.weight: Font.DemiBold
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
    }
    PropertyRow { Layout.fillWidth: true; label: qsTr("File"); value: root.info.fileName || ""; mono: true }
    PropertyRow { Layout.fillWidth: true; label: qsTr("Made"); value: root.info.created || "" }
    PropertyRow {
        Layout.fillWidth: true
        label: qsTr("State")
        value: root.info.status === "outdated" ? qsTr("outdated: its data changed since")
             : root.info.status === "pending" ? qsTr("current, but edits or corrections wait for processing")
             : root.info.status === "current" ? qsTr("current") : ""
    }
    Repeater {
        model: root.info.parameters ? Object.keys(root.info.parameters) : []
        delegate: PropertyRow {
            Layout.fillWidth: true
            label: modelData
            value: String(root.info.parameters[modelData])
            mono: true
        }
    }
    Flow {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 4
        Button {
            text: qsTr("Make again")
            flat: true
            enabled: root.info.regenerable === true
            onClicked: App.project.regenerateProduct(root.assetId)
        }
        Button {
            text: qsTr("Change settings…")
            flat: true
            enabled: (root.info.job || 0) > 0
            onClicked: {
                if (App.processing.loadRequest(root.info.job)) root.panelRequested("figure")
            }
        }
        Button { text: qsTr("Open the file"); flat: true; onClicked: App.openPath(App.project.productPath(root.assetId)) }
        Button { text: qsTr("Show in folder"); flat: true; onClicked: App.showInFolder(App.project.productPath(root.assetId)) }
    }
}
