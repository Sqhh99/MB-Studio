// A background job: state, current step, last output line, actions.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."

Rectangle {
    id: root
    property var job: ({})
    signal showLog(var job)
    implicitHeight: content.implicitHeight + 10
    color: mouse.containsMouse ? Theme.surfaceHover : "transparent"
    readonly property bool active: job.state === "running" || job.state === "queued"

    MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

    RowLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.panelPadding
        anchors.rightMargin: 6
        spacing: 10

        Item {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            BusyIndicator { anchors.fill: parent; running: root.job.state === "running"; visible: running }
            Icon {
                anchors.centerIn: parent
                visible: root.job.state !== "running"
                size: 18
                color: Theme.stateColor(root.job.state)
                path: root.job.state === "completed" ? Icons.check
                      : root.job.state === "failed" ? Icons.error
                      : root.job.state === "cancelled" ? Icons.close : Icons.schedule
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1
            RowLayout {
                spacing: 8
                Text {
                    text: root.job.title || ""
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    text: root.job.elapsed || ""
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.family: Theme.monoFont
                }
            }
            Text {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: root.job.state === "failed" ? Theme.danger : Theme.textMuted
                font.pixelSize: Theme.fontSmall
                text: root.job.state === "failed" ? root.job.error
                      : root.job.state === "queued" && root.job.blockedBy > 0 ? qsTr("Waiting for job %1 (it uses the same survey lines or files)").arg(root.job.blockedBy)
                      : root.job.state === "queued" ? qsTr("Waiting for a free worker")
                      : root.job.state === "running" ? qsTr("Step %1 of %2: %3").arg(root.job.step).arg(root.job.stepCount).arg(root.job.stepTitle)
                      : root.job.state === "cancelled" ? qsTr("Cancelled")
                      : qsTr("Completed")
            }
            Text {
                Layout.fillWidth: true
                visible: root.active && (root.job.lastLine || "") !== ""
                text: root.job.lastLine || ""
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall
                font.family: Theme.monoFont
                elide: Text.ElideRight
            }
        }
        IconButton {
            iconPath: Icons.stop
            tip: qsTr("Cancel")
            visible: root.active
            onClicked: App.jobs.cancel(root.job.jobId)
        }
        IconButton {
            iconPath: Icons.refresh
            tip: qsTr("Run again")
            visible: root.job.canRetry === true
            onClicked: App.jobs.retry(root.job.jobId)
        }
        IconButton {
            iconPath: Icons.file
            tip: qsTr("Log and commands")
            onClicked: root.showLog(root.job)
        }
        IconButton {
            iconPath: Icons.folderOpen
            tip: qsTr("Show the job folder")
            onClicked: App.showInFolder(root.job.logPath)
        }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border; opacity: 0.6 }
}
