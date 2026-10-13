// The microkernel at a glance: plugins, the services they provide and the
// commands they register.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

ScrollView {
    id: page
    contentWidth: availableWidth
    clip: true

    function stateColor(state) {
        if (state === "active") return Theme.success
        if (state === "failed") return Theme.danger
        if (state === "disabled") return Theme.textFaint
        return Theme.warning
    }

    ColumnLayout {
        width: page.availableWidth
        spacing: 16

        ColumnLayout {
            Layout.margins: Theme.pagePadding
            Layout.bottomMargin: 0
            spacing: 4
            Text { text: qsTr("Plugins"); color: Theme.text; font.pixelSize: Theme.fontHeadline; font.weight: Font.DemiBold }
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 900
                wrapMode: Text.WordWrap
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
                text: qsTr("MB-Studio is built around a small kernel that only manages plugins, services, events and commands. MB-System support and this user interface are plugins: the same services can be used without the interface, for example with \"mbstudio --command swath.inspect path=…\".")
            }
        }

        Repeater {
            model: App.plugins
            delegate: Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                implicitHeight: pluginColumn.implicitHeight + 36
                ColumnLayout {
                    id: pluginColumn
                    anchors.fill: parent
                    spacing: 6
                    RowLayout {
                        spacing: 10
                        Icon { path: Icons.extension; color: page.stateColor(modelData.state); size: 22 }
                        Text { text: modelData.name; color: Theme.text; font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                        Chip { text: modelData.state; tint: page.stateColor(modelData.state) }
                        Chip { text: modelData.id; tint: Theme.accentStrong }
                        Item { Layout.fillWidth: true }
                        Text { text: modelData.version; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
                    }
                    Text { text: modelData.description; color: Theme.textMuted; font.pixelSize: Theme.fontBody; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Text { visible: modelData.error !== ""; text: modelData.error; color: Theme.danger; font.pixelSize: Theme.fontBody; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Text { visible: modelData.path !== ""; text: modelData.path; color: Theme.textFaint; font.pixelSize: Theme.fontSmall; elide: Text.ElideMiddle; Layout.fillWidth: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: App.services.filter(s => s.provider === modelData.id)
                            delegate: Chip { text: modelData.id; tint: Theme.accent }
                        }
                        Repeater {
                            model: App.commands.filter(c => c.owner === modelData.id)
                            delegate: Chip { text: modelData.id; tint: Theme.warning }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.leftMargin: Theme.pagePadding
            Layout.bottomMargin: Theme.pagePadding
            spacing: 14
            Chip { text: qsTr("service"); tint: Theme.accent }
            Chip { text: qsTr("command"); tint: Theme.warning }
        }
    }
}
