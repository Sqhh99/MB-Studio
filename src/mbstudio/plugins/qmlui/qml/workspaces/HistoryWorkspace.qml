// The processing history of the project: what ran, with which commands,
// what it produced, and its log.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: page
    readonly property var history: App.project.history

    EmptyState {
        anchors.centerIn: parent
        visible: list.count === 0
        iconPath: Icons.history
        title: qsTr("No processing yet")
        message: qsTr("Every import, cleaning, processing, gridding and export job of the project is recorded here with its exact commands, products and log.")
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: 10
        clip: true
        model: page.history
        ScrollBar.vertical: ScrollBar {}
        delegate: Card {
            width: ListView.view.width
            implicitHeight: body.implicitHeight + 36
            ColumnLayout {
                id: body
                anchors.fill: parent
                spacing: 6
                RowLayout {
                    spacing: 10
                    Text { text: "#" + sequence; color: Theme.textFaint; font.family: Theme.monoFont; font.pixelSize: Theme.fontSmall + 1 }
                    Text { text: title; color: Theme.text; font.pixelSize: Theme.fontBody + 1; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                    Chip { text: model.state; tint: Theme.stateColor(model.state) }
                }
                Text {
                    text: (started !== "" ? started : "") + (duration !== "" ? "  ·  " + duration : "")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    visible: text !== ""
                }
                Text { visible: error !== ""; text: error; color: Theme.danger; font.pixelSize: Theme.fontSmall + 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                CommandPreview {
                    Layout.fillWidth: true
                    visible: commands !== ""
                    text: commands.split("\n").map(c => "$ " + c).join("\n")
                }
                Repeater {
                    model: outputs
                    delegate: Text {
                        text: "→ " + modelData
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        font.family: Theme.monoFont
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    spacing: 6
                    Button { text: qsTr("Open log file"); flat: true; enabled: logPath !== ""; onClicked: App.openPath(logPath) }
                    Button { text: qsTr("Show the job folder"); flat: true; enabled: logPath !== ""; onClicked: App.showInFolder(logPath) }
                }
            }
        }
    }
}
