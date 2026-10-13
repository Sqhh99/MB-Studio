// Left dock: the project's survey lines (multi-selection) and products
// (map layers with visibility, quality grids below their grid).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.panel
    readonly property var project: App.project
    property string selectedProduct: ""
    signal productSelected(string id)
    signal addFolderRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            title: root.project.isOpen ? root.project.name : qsTr("Project")
            iconPath: Icons.folder
            IconButton { iconPath: Icons.add; tip: qsTr("New project…"); onClicked: App.trigger("ui:newProject") }
            IconButton { iconPath: Icons.folderOpen; tip: qsTr("Open project…"); onClicked: App.trigger("ui:openProject") }
        }

        // no project yet
        ColumnLayout {
            visible: !root.project.isOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.panelPadding
            spacing: 10
            Text {
                Layout.fillWidth: true
                text: root.project.available ? qsTr("A project keeps your survey lines, processing settings, products and their history together.")
                                             : qsTr("Projects are not available (the project plugin is not loaded)")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall + 1
                wrapMode: Text.WordWrap
            }
            PrimaryButton {
                Layout.fillWidth: true
                enabled: root.project.available
                text: qsTr("New project…")
                onClicked: App.trigger("ui:newProject")
            }
            Button {
                Layout.fillWidth: true
                enabled: root.project.available
                text: qsTr("Open project…")
                onClicked: App.trigger("ui:openProject")
            }
            Text {
                visible: root.project.recentProjects.length > 0
                Layout.topMargin: 8
                text: qsTr("Recent projects")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.capitalization: Font.AllUppercase
            }
            Repeater {
                model: root.project.recentProjects
                delegate: ItemDelegate {
                    Layout.fillWidth: true
                    implicitHeight: 34
                    onClicked: root.project.openProject(modelData)
                    contentItem: Text {
                        text: modelData.replace(/^.*[\\/]/, "")
                        color: Theme.text
                        font.pixelSize: Theme.fontSmall + 1
                        elide: Text.ElideMiddle
                        verticalAlignment: Text.AlignVCenter
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: modelData
                    ToolTip.delay: 600
                }
            }
            Item { Layout.fillHeight: true }
        }

        // survey lines
        RowLayout {
            visible: root.project.isOpen
            Layout.fillWidth: true
            Layout.leftMargin: Theme.panelPadding
            Layout.rightMargin: 4
            Layout.topMargin: 4
            spacing: 2
            Text {
                text: qsTr("Survey lines")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
            }
            Chip { text: "" + root.project.lineCount; tint: Theme.accentStrong }
            Item { Layout.fillWidth: true }
            IconButton { iconPath: Icons.add; tip: qsTr("Add survey files…"); onClicked: App.trigger("ui:addData") }
            IconButton { iconPath: Icons.folder; tip: qsTr("Add a folder of survey files…"); onClicked: root.addFolderRequested() }
            IconButton { iconPath: Icons.check; tip: qsTr("Select all"); enabled: root.project.lineCount > 0; onClicked: root.project.selectAll() }
            IconButton { iconPath: Icons.close; tip: qsTr("Clear the selection"); enabled: root.project.selectedCount > 0; onClicked: root.project.clearSelection() }
        }
        Text {
            visible: root.project.isOpen
            Layout.fillWidth: true
            Layout.leftMargin: Theme.panelPadding
            Layout.bottomMargin: 4
            text: root.project.selectionText
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
            elide: Text.ElideRight
        }
        ListView {
            id: lineList
            objectName: "lineList"
            visible: root.project.isOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 80
            clip: true
            model: root.project.lines
            ScrollBar.vertical: ScrollBar {}
            Text {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: lineList.count === 0
                text: qsTr("Add survey files or a folder. MB-Studio detects the formats, writes the datalists and reads the statistics and navigation for you.")
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall + 1
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            delegate: Rectangle {
                width: ListView.view.width
                height: 44
                color: selected ? Theme.selection : lineMouse.containsMouse ? Theme.surfaceHover : "transparent"
                MouseArea {
                    id: lineMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            if (!selected) root.project.select(lineId, 0)
                            lineMenu.lineId = lineId
                            lineMenu.linePath = path
                            lineMenu.popup()
                            return
                        }
                        root.project.select(lineId, (mouse.modifiers & Qt.ControlModifier) ? 1 : (mouse.modifiers & Qt.ShiftModifier) ? 2 : 0)
                    }
                    onDoubleClicked: App.inspectLine(lineId)
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: 8
                    spacing: 8
                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: missing ? Theme.danger : model.state === "processed" ? Theme.success : model.state === "preprocessed" ? Theme.info : Theme.textFaint
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: name
                            color: missing ? Theme.danger : Theme.text
                            font.pixelSize: Theme.fontSmall + 1
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        Text {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Theme.textFaint
                            font.pixelSize: Theme.fontSmall
                            text: missing ? qsTr("file not found")
                                  : !inspected ? qsTr("%1 · reading…").arg(formatName)
                                  : qsTr("%1 · %2 pings · %3 km").arg(model.state === "processed" ? qsTr("processed") : model.state === "preprocessed" ? qsTr("preprocessed") : qsTr("raw"))
                                        .arg(records).arg(Number(trackKm).toFixed(1))
                        }
                    }
                }
            }
        }

        // products
        Rectangle { visible: root.project.isOpen; Layout.fillWidth: true; height: 1; color: Theme.border }
        RowLayout {
            visible: root.project.isOpen
            Layout.fillWidth: true
            Layout.leftMargin: Theme.panelPadding
            Layout.rightMargin: 4
            Layout.topMargin: 4
            spacing: 2
            Text {
                text: qsTr("Products and layers")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
            }
            Chip { text: "" + productList.count; tint: Theme.accentStrong }
            Item { Layout.fillWidth: true }
            IconButton { iconPath: Icons.terrain; tip: qsTr("Grid bathymetry…"); onClicked: App.trigger("ui:panel:grid") }
        }
        ListView {
            id: productList
            objectName: "productList"
            visible: root.project.isOpen
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(260, Math.max(70, count * 36 + 8))
            clip: true
            model: root.project.products
            ScrollBar.vertical: ScrollBar {}
            Text {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: productList.count === 0
                text: qsTr("Grids, mosaics and exports appear here.")
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall + 1
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            delegate: Rectangle {
                width: ListView.view.width
                height: 34
                color: root.selectedProduct === assetId ? Theme.selection : productMouse.containsMouse ? Theme.surfaceHover : "transparent"
                MouseArea {
                    id: productMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        root.productSelected(assetId)
                        if (mouse.button === Qt.RightButton) {
                            productMenu.assetId = assetId
                            productMenu.assetPath = path
                            productMenu.mappable = mappable
                            productMenu.popup()
                        }
                    }
                    onDoubleClicked: if (mappable) App.openGrid(path)
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding + (quality ? 16 : 0)
                    anchors.rightMargin: 4
                    spacing: 6
                    IconButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        visible: mappable
                        iconPath: model.layer && model.visible ? Icons.visible : Icons.hidden
                        iconColor: model.layer && model.visible ? Theme.accent : Theme.textFaint
                        tip: qsTr("Show on the map")
                        onClicked: root.project.setLayerVisible(assetId, !(model.layer && model.visible))
                    }
                    Icon {
                        visible: !mappable
                        path: Icons.download
                        size: 16
                        color: Theme.textMuted
                        Layout.leftMargin: 6
                        Layout.rightMargin: 6
                    }
                    Text {
                        text: label
                        color: exists ? Theme.text : Theme.danger
                        font.pixelSize: Theme.fontSmall + 1
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: type === "grid" ? qsTr("grid") : type === "mosaic" ? qsTr("mosaic") : type === "export" ? qsTr("export")
                              : type.indexOf("_num") > 0 ? qsTr("count") : type.indexOf("_sd") > 0 ? qsTr("std. dev.") : type
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }
    }

    Menu {
        id: lineMenu
        property string lineId
        property string linePath
        MenuItem { text: qsTr("Inspect"); onTriggered: App.inspectLine(lineMenu.lineId) }
        MenuItem { text: qsTr("Show in folder"); onTriggered: App.showInFolder(lineMenu.linePath) }
        MenuItem { text: qsTr("Statistics with mbinfo"); onTriggered: App.runToolOn("mbinfo", lineMenu.linePath) }
        MenuSeparator {}
        MenuItem { text: qsTr("Grid the selected lines…"); onTriggered: App.trigger("ui:panel:grid") }
        MenuSeparator {}
        MenuItem { text: qsTr("Remove from the project"); onTriggered: root.project.removeSelectedLines() }
    }
    Menu {
        id: productMenu
        property string assetId
        property string assetPath
        property bool mappable: false
        MenuItem { text: qsTr("Open in the grid view"); enabled: productMenu.mappable; onTriggered: App.openGrid(productMenu.assetPath) }
        MenuItem { text: qsTr("Show in folder"); onTriggered: App.showInFolder(productMenu.assetPath) }
        MenuItem {
            text: qsTr("Export…")
            enabled: productMenu.mappable
            onTriggered: {
                App.processing.kind = "export"
                App.processing.setValue("source", productMenu.assetId)
                App.trigger("ui:panel:export")
            }
        }
        MenuSeparator {}
        MenuItem { text: qsTr("Remove from the project"); onTriggered: root.project.removeProduct(productMenu.assetId, false) }
        MenuItem { text: qsTr("Delete the files"); onTriggered: root.project.removeProduct(productMenu.assetId, true) }
    }
}
