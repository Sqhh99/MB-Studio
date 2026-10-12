// Browser of the swath data formats supported by MB-System.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: page
    readonly property var formats: App.formats

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                spacing: 2
                Text { text: qsTr("Data formats"); color: Theme.text; font.pixelSize: Theme.fontHeadline; font.weight: Font.DemiBold }
                Text {
                    text: qsTr("%1 of %2 formats supported by MB-System. The format id is the MBIO -F option of every program.").arg(formats.count).arg(formats.totalCount)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                Layout.preferredWidth: 320
                placeholderText: qsTr("Search by id, name, sonar or vendor")
                text: formats.filter
                onTextChanged: formats.filter = text
                leftPadding: 34
                Icon { x: 8; anchors.verticalCenter: parent.verticalCenter; path: Icons.search; color: Theme.textFaint; size: 18 }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: formats
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: ListView.view.width - 12
                implicitHeight: row.implicitHeight + 24
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border
                RowLayout {
                    id: row
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 16
                    Rectangle {
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignTop
                        radius: 8
                        color: Theme.accentSoft
                        Text {
                            anchors.centerIn: parent
                            text: formatId
                            color: Theme.accent
                            font.pixelSize: 17
                            font.weight: Font.Bold
                            font.family: Theme.monoFont
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        RowLayout {
                            spacing: 10
                            Text { text: name; color: Theme.text; font.pixelSize: Theme.fontBody + 1; font.weight: Font.DemiBold; font.family: Theme.monoFont }
                            Text { text: description; color: Theme.textMuted; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                        Text {
                            text: attributes
                            color: Theme.textFaint
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    IconButton {
                        iconPath: Icons.code
                        tip: qsTr("Describe with mbformat")
                        Layout.alignment: Qt.AlignTop
                        onClicked: {
                            if (App.tools.selectTool("mbformat")) {
                                App.tools.arguments = "-F" + formatId
                                App.currentPage = AppController.ToolsPage
                                App.tools.run()
                            }
                        }
                    }
                }
            }
        }
    }
}
