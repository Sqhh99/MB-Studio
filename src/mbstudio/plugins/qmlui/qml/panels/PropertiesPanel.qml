// Right dock: the properties of the active document (display, tools,
// versions; context sensitive, so the documents need no sidebars of their
// own), then of what is selected (a product with its layer style and state,
// survey lines, or the project with its next step and processing guide).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.panel
    property string product: ""
    readonly property var project: App.project
    // re-read when products or layers change (mapRevision) or lines change (lineCount)
    readonly property var info: product !== "" && project.mapRevision >= 0 ? project.productInfo(product) : ({})
    readonly property var line: project.selectedCount === 1 && project.lineCount >= 0 ? project.lineInfo(project.selectedIds[0]) : ({})
    readonly property var summary: project.summary
    readonly property string workspace: App.workspaces.currentWorkspace
    readonly property string documentPage: workspace === "grid" ? "GridProperties.qml"
                                         : workspace === "terrain" ? "TerrainProperties.qml"
                                         : workspace === "editor" ? "EditorProperties.qml"
                                         : workspace === "inspect" ? "InspectProperties.qml"
                                         : workspace === "figure" ? "FigureProperties.qml" : ""
    signal panelRequested(string kind)
    signal productClosed()

    function fmt(v, digits) { return v === undefined || v === null || isNaN(v) ? "–" : Number(v).toFixed(digits) }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            title: qsTr("Properties")
            iconPath: Icons.info
            IconButton { visible: root.product !== ""; iconPath: Icons.close; tip: qsTr("Close"); onClicked: root.productClosed() }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 6

                // the active document
                Loader {
                    id: documentProperties
                    objectName: "documentProperties"
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
                    visible: active && status === Loader.Ready
                    active: root.documentPage !== ""
                    source: root.documentPage
                    onLoaded: {
                        if (item.doc !== undefined)
                            item.doc = Qt.binding(function() { return App.workspaces.currentDocument })
                        if (item.assetId !== undefined)
                            item.assetId = Qt.binding(function() { return App.workspaces.currentArgument })
                        if (typeof item.panelRequested === "function")
                            item.panelRequested.connect(root.panelRequested)
                    }
                }
                Rectangle {
                    visible: documentProperties.visible && root.project.isOpen
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.border
                }

                // nothing open
                Text {
                    visible: !root.project.isOpen
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
                    text: qsTr("Open or create a project to work with survey lines and their products.")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    wrapMode: Text.WordWrap
                }

                // a product and its layer style
                ColumnLayout {
                    visible: root.product !== "" && root.info.assetId !== undefined
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
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
                    // made from the current data?
                    Rectangle {
                        visible: root.info.status === "outdated" || root.info.status === "pending"
                        Layout.fillWidth: true
                        implicitHeight: stateText.implicitHeight + 12
                        radius: Theme.radiusSmall
                        color: Qt.rgba(Theme.productStatusColor(root.info.status || "").r, Theme.productStatusColor(root.info.status || "").g, Theme.productStatusColor(root.info.status || "").b, 0.15)
                        Text {
                            id: stateText
                            anchors.fill: parent
                            anchors.margins: 6
                            text: root.info.status === "outdated"
                                  ? qsTr("Outdated: %1 of its survey lines changed since it was made.").arg(root.info.staleInputs || 0)
                                  : qsTr("%1 of its survey lines have edits or corrections that are not processed yet.").arg(root.info.staleInputs || 0)
                            color: Theme.text
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.WordWrap
                        }
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
                    ColumnLayout {
                        visible: root.info.mappable === true
                        Layout.fillWidth: true
                        spacing: 4
                        Switch {
                            text: qsTr("Show on the map")
                            checked: root.info.layer === true && root.info.visible === true
                            onToggled: root.project.setLayerVisible(root.product, checked)
                        }
                        Text { text: qsTr("Opacity"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 1; stepSize: 0.05
                            value: root.info.opacity !== undefined ? root.info.opacity : 1
                            onPressedChanged: if (!pressed) root.project.setLayerOpacity(root.product, value)
                        }
                        Text { text: qsTr("Colour table"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                        ComboBox {
                            Layout.fillWidth: true
                            model: App.colorMaps
                            currentIndex: root.info.colorMap !== undefined ? root.info.colorMap : 0
                            onActivated: (index) => root.project.setLayerColorMap(root.product, index)
                        }
                        Switch {
                            text: qsTr("Hillshade")
                            checked: root.info.hillshade === true
                            onToggled: root.project.setLayerHillshade(root.product, checked)
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 4
                        Button { text: qsTr("Open"); visible: root.info.mappable === true; onClicked: App.openGrid(root.project.productPath(root.product)) }
                        Button { text: qsTr("3D"); visible: root.info.mappable === true; onClicked: App.openTerrain(root.project.productPath(root.product)) }
                        Button { text: qsTr("Open"); visible: root.info.type === "figure"; onClicked: App.openFigure(root.product) }
                        Button {
                            text: qsTr("Make again")
                            visible: root.info.regenerable === true && root.info.status === "outdated"
                            onClicked: root.project.regenerateProduct(root.product)
                        }
                        Button {
                            text: qsTr("Export…")
                            visible: root.info.mappable === true
                            onClicked: {
                                App.processing.kind = "export"
                                App.processing.setValue("source", root.product)
                                root.panelRequested("export")
                            }
                        }
                        IconButton { iconPath: Icons.folderOpen; tip: qsTr("Show in folder"); onClicked: App.showInFolder(root.project.productPath(root.product)) }
                    }
                }

                // one survey line
                ColumnLayout {
                    visible: root.product === "" && root.project.isOpen && root.line.lineId !== undefined
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
                    spacing: 6
                    Text {
                        text: root.line.name || ""
                        color: Theme.text
                        font.pixelSize: Theme.fontTitle - 2
                        font.weight: Font.DemiBold
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Format"); value: (root.line.formatName || "") + " (" + (root.line.format || "") + ")" }
                    PropertyRow {
                        Layout.fillWidth: true
                        label: qsTr("State")
                        value: root.line.status === "processed" ? qsTr("processed (up to date)")
                             : root.line.status === "edits-pending" ? qsTr("edits not processed yet")
                             : root.line.status === "corrections-pending" ? qsTr("corrections not processed yet")
                             : root.line.status === "processing-required" ? qsTr("edits or corrections set, not processed yet")
                             : qsTr("unprocessed")
                    }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Start"); value: root.line.timeStart || "" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("End"); value: root.line.timeEnd || "" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Pings"); value: root.line.records !== undefined ? String(root.line.records) : "" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Track length"); value: root.fmt(root.line.trackKm, 2) + " km" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Depth"); value: root.fmt(root.line.depthMin, 1) + " … " + root.fmt(root.line.depthMax, 1) + " m" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Size"); value: root.fmt(root.line.sizeMB, 1) + " MB" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("File"); value: root.line.path || ""; mono: true }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 4
                        Button { text: qsTr("Inspect"); onClicked: App.inspectLine(root.line.lineId, "") }
                        Button { text: qsTr("Edit soundings"); onClicked: App.openEditor([root.line.lineId]) }
                        Button { text: qsTr("mbinfo"); onClicked: App.runToolOn("mbinfo", root.line.path) }
                        IconButton { iconPath: Icons.folderOpen; tip: qsTr("Show in folder"); onClicked: App.showInFolder(root.line.path) }
                    }
                }

                // the project
                ColumnLayout {
                    visible: root.product === "" && root.project.isOpen && root.line.lineId === undefined
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
                    spacing: 6
                    Text {
                        text: root.project.selectedCount > 1 ? root.project.selectionText : root.project.name
                        color: Theme.text
                        font.pixelSize: Theme.fontTitle - 2
                        font.weight: Font.DemiBold
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Survey lines"); value: String(root.summary.lines) }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Data"); value: root.fmt(root.summary.sizeMB, 1) + " MB" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Formats"); value: root.summary.formats || "" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Time span"); value: root.summary.timeStart ? root.summary.timeStart + " … " + root.summary.timeEnd : "" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Track length"); value: root.fmt(root.summary.trackKm, 1) + " km" }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Depth"); value: root.summary.hasDepth ? root.fmt(root.summary.depthMin, 0) + " … " + root.fmt(root.summary.depthMax, 0) + " m" : "" }
                    PropertyRow {
                        Layout.fillWidth: true
                        label: qsTr("Extent")
                        mono: true
                        value: root.summary.hasBounds ? root.fmt(root.summary.west, 4) + " / " + root.fmt(root.summary.east, 4) + "\n" + root.fmt(root.summary.south, 4) + " / " + root.fmt(root.summary.north, 4) : ""
                    }
                    PropertyRow { Layout.fillWidth: true; label: qsTr("Folder"); value: root.project.root; mono: true }
                    // the recommended next step
                    Rectangle {
                        objectName: "nextAction"
                        visible: (root.project.nextAction.kind || "") !== ""
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        implicitHeight: nextColumn.implicitHeight + 16
                        radius: Theme.radius
                        color: Theme.accentSoft
                        ColumnLayout {
                            id: nextColumn
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4
                            Text { text: qsTr("Next step"); color: Theme.accent; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }
                            Text {
                                Layout.fillWidth: true
                                text: root.project.nextAction.reason || ""
                                color: Theme.text
                                font.pixelSize: Theme.fontSmall + 1
                                wrapMode: Text.WordWrap
                            }
                            Button {
                                visible: root.project.nextAction.kind !== "regenerate"
                                text: root.project.nextAction.kind === "import" ? qsTr("Add survey data…")
                                    : root.project.nextAction.kind === "preprocess" ? qsTr("Preprocess…")
                                    : root.project.nextAction.kind === "process" ? qsTr("Process lines…")
                                    : qsTr("Grid bathymetry…")
                                flat: true
                                onClicked: root.panelRequested(root.project.nextAction.kind)
                            }
                            Text {
                                visible: root.project.nextAction.kind === "regenerate"
                                Layout.fillWidth: true
                                text: qsTr("Outdated products are marked in the project panel; make them again from their menu.")
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                    Text {
                        Layout.topMargin: 10
                        text: qsTr("Processing")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                        font.capitalization: Font.AllUppercase
                    }
                    WorkflowGuide {
                        Layout.fillWidth: true
                        onStepClicked: (panel) => root.panelRequested(panel)
                    }
                }
            }
        }
    }
}
