// A processing step on the selected survey lines: its parameters (from the
// schema of the processing view model), the exact commands, the expected
// products, warnings and the Run button.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.panel
    readonly property var vm: App.processing
    signal closeRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PanelHeader {
            Layout.fillWidth: true
            title: root.vm.title
            iconPath: Icons.tune
            IconButton { iconPath: Icons.refresh; tip: qsTr("Default values"); onClicked: root.vm.resetDefaults() }
            IconButton { iconPath: Icons.close; tip: qsTr("Close"); onClicked: root.closeRequested() }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    Layout.margins: Theme.panelPadding
                    Layout.bottomMargin: 0
                    text: root.vm.description
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    wrapMode: Text.WordWrap
                }

                // what the step works on
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.panelPadding
                    Layout.rightMargin: Theme.panelPadding
                    visible: root.vm.revision >= 0 && (root.vm.kind !== "export" || root.vm.value("source") === "soundings")
                    implicitHeight: selectionRow.implicitHeight + 14
                    radius: Theme.radiusSmall
                    color: Theme.accentSoft
                    RowLayout {
                        id: selectionRow
                        anchors.fill: parent
                        anchors.margins: 7
                        spacing: 8
                        Icon { path: Icons.route; size: 16; color: Theme.accent }
                        Text {
                            Layout.fillWidth: true
                            text: App.project.selectionText
                            color: Theme.text
                            font.pixelSize: Theme.fontSmall + 1
                            elide: Text.ElideRight
                        }
                        Text {
                            text: qsTr("select on the map or in the list")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                            visible: App.project.selectedCount === 0 && root.width > 360
                        }
                    }
                }

                // the parameters
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.panelPadding
                    Layout.rightMargin: Theme.panelPadding
                    spacing: 8
                    Repeater {
                        model: root.vm.fields
                        delegate: ParamField {
                            Layout.fillWidth: true
                            field: modelData
                            vm: root.vm
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.panelPadding
                    Layout.rightMargin: Theme.panelPadding
                    visible: root.vm.estimate !== ""
                    text: root.vm.estimate
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: root.vm.warnings
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.panelPadding
                        Layout.rightMargin: Theme.panelPadding
                        spacing: 6
                        Icon { path: Icons.warning; size: 14; color: Theme.warning; Layout.alignment: Qt.AlignTop }
                        Text {
                            Layout.fillWidth: true
                            text: modelData
                            color: Theme.warning
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.panelPadding
                    Layout.rightMargin: Theme.panelPadding
                    visible: root.vm.error !== ""
                    spacing: 6
                    Icon { path: Icons.error; size: 14; color: Theme.danger; Layout.alignment: Qt.AlignTop }
                    Text {
                        objectName: "processingError"
                        Layout.fillWidth: true
                        text: root.vm.error
                        color: Theme.danger
                        font.pixelSize: Theme.fontSmall + 1
                        wrapMode: Text.WordWrap
                    }
                }

                Text {
                    Layout.leftMargin: Theme.panelPadding
                    visible: root.vm.preview !== ""
                    text: qsTr("Commands")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.capitalization: Font.AllUppercase
                }
                CommandPreview {
                    objectName: "commandPreview"
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.panelPadding
                    Layout.rightMargin: Theme.panelPadding
                    visible: root.vm.preview !== ""
                    text: root.vm.preview
                }
                Text {
                    Layout.leftMargin: Theme.panelPadding
                    visible: root.vm.outputs.length > 0
                    text: qsTr("Products")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.capitalization: Font.AllUppercase
                }
                Repeater {
                    model: root.vm.outputs.length > 6 ? root.vm.outputs.slice(0, 6) : root.vm.outputs
                    delegate: Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.panelPadding
                        Layout.rightMargin: Theme.panelPadding
                        text: modelData.replace(/^.*[\\/]/, "")
                        color: Theme.text
                        font.pixelSize: Theme.fontSmall
                        font.family: Theme.monoFont
                        elide: Text.ElideMiddle
                    }
                }
                Text {
                    Layout.leftMargin: Theme.panelPadding
                    visible: root.vm.outputs.length > 6
                    text: qsTr("… and %1 more").arg(root.vm.outputs.length - 6)
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                }
                Item { Layout.preferredHeight: 8 }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.panelPadding
            spacing: 8
            PrimaryButton {
                objectName: "runProcessing"
                Layout.fillWidth: true
                text: qsTr("Run")
                enabled: root.vm.ready
                onClicked: root.vm.run()
            }
        }
    }
}
