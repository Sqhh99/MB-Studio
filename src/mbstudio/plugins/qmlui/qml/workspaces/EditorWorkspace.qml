// A sounding editor document: the soundings of survey lines in 3D (the
// selection tools are in the tool options bar) above the ping view, which
// shows the beams of the current ping across the track. The 3D view and the
// ping view are linked: a click on a sounding (navigate tool) shows its
// ping, and the beams of that ping are highlighted in 3D.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: page
    property var document: null
    readonly property var editor: document
    readonly property bool ready: editor !== null && editor.loaded
    color: Theme.mapBackground

    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical
        visible: page.ready

        Item {
            SplitView.fillHeight: true
            SplitView.minimumHeight: 150
            PointCloudView {
                id: cloud
                objectName: "pointCloudView"
                anchors.fill: parent
                model: page.editor
                toolColor: Theme.warning
            }
            // the sounding under the cursor: what an edit of it refers to
            Rectangle {
                visible: page.ready && page.editor.statusText !== ""
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 8
                radius: Theme.radius
                color: Theme.overlay
                implicitWidth: probeText.implicitWidth + 16
                implicitHeight: probeText.implicitHeight + 10
                Text {
                    id: probeText
                    anchors.centerIn: parent
                    color: Theme.overlayText
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.fontSmall
                    text: page.ready ? page.editor.statusText : ""
                }
            }
            Rectangle {
                visible: page.ready && page.editor.colorMode === 0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                radius: Theme.radius
                color: Theme.overlay
                implicitWidth: 180
                implicitHeight: depthLegend.implicitHeight + 10
                ColumnLayout {
                    id: depthLegend
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 2
                    Row {
                        id: legendBar
                        Layout.fillWidth: true
                        height: 9
                        readonly property var stops: page.ready ? page.editor.legend : []
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
                        Text { text: page.ready ? Number(page.editor.depthMin).toFixed(0) + " m" : ""; color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
                        Item { Layout.fillWidth: true }
                        Text { text: page.ready ? qsTr("depth %1 m").arg(Number(page.editor.depthMax).toFixed(0)) : ""; color: Theme.overlayText; font.pixelSize: Theme.fontSmall - 1 }
                    }
                }
            }
        }

        // the current ping across the track
        Rectangle {
            SplitView.preferredHeight: 190
            SplitView.minimumHeight: 110
            color: Theme.surface
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: Theme.panelHeader
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 4
                        spacing: 6
                        Text { text: qsTr("Ping"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold }
                        IconButton { iconPath: Icons.chevronLeft; tip: qsTr("Previous ping (Page Up)"); implicitWidth: 22; implicitHeight: 22; padding: 2; onClicked: page.editor.stepPing(-1) }
                        IconButton { iconPath: Icons.chevronRight; tip: qsTr("Next ping (Page Down)"); implicitWidth: 22; implicitHeight: 22; padding: 2; onClicked: page.editor.stepPing(1) }
                        Text {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Theme.text
                            font.pixelSize: Theme.fontSmall
                            text: page.ready && page.editor.pingInfo.index !== undefined
                                  ? qsTr("%1 · ping %2 of %3 · %4").arg(page.editor.pingInfo.line).arg(page.editor.pingInfo.number)
                                        .arg(page.editor.pingInfo.pings).arg(page.editor.pingInfo.time)
                                  : ""
                        }
                        Text { text: qsTr("click a beam to flag or unflag it"); color: Theme.textFaint; font.pixelSize: Theme.fontSmall }
                    }
                }
                PingView {
                    objectName: "pingView"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: page.editor
                    goodColor: Theme.accent
                    flaggedColor: Theme.danger
                    axisColor: Theme.border
                    textColor: Theme.textMuted
                }
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: page.editor === null || (!page.editor.loaded && !page.editor.busy)
        iconPath: Icons.edit
        title: page.editor !== null && page.editor.error !== "" ? qsTr("The soundings could not be read") : qsTr("Sounding editor")
        message: page.editor !== null && page.editor.error !== "" ? page.editor.error
                 : qsTr("Select survey lines and open them in the sounding editor.")
    }
    ColumnLayout {
        anchors.centerIn: parent
        visible: page.editor !== null && page.editor.busy
        spacing: 8
        BusyIndicator { running: parent.visible; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Reading the soundings and their saved edits…"); color: Theme.textMuted; font.pixelSize: Theme.fontBody }
    }
}
