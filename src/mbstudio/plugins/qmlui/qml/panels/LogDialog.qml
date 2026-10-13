// The commands and the complete log of a job.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Dialog {
    id: root
    property var job: ({})
    property string text: ""
    title: job.title || qsTr("Job log")
    modal: true
    standardButtons: Dialog.Close
    width: Math.min(parent ? parent.width - 80 : 900, 980)
    height: Math.min(parent ? parent.height - 80 : 640, 720)
    anchors.centerIn: parent

    function showJob(j) {
        job = j
        text = App.jobs.log(j.jobId)
        open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        CommandPreview {
            Layout.fillWidth: true
            visible: (root.job.commands || "") !== ""
            text: (root.job.commands || "").split("\n").map(c => "$ " + c).join("\n")
        }
        Text {
            visible: (root.job.error || "") !== ""
            text: root.job.error || ""
            color: Theme.danger
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            TextArea {
                text: root.text
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.NoWrap
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSmall + 1
                color: Theme.consoleText
                background: Rectangle { color: Theme.console; radius: Theme.radiusSmall }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: root.job.logPath || ""
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideMiddle
            }
            Button { text: qsTr("Copy"); flat: true; onClicked: App.copyToClipboard(root.text) }
            Button { text: qsTr("Open log file"); flat: true; enabled: (root.job.logPath || "") !== ""; onClicked: App.openPath(root.job.logPath) }
        }
    }
}
