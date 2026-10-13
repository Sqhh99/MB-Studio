// Bottom dock: background jobs (queue, progress, errors, cancel, retry, log).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.surface
    readonly property var jobs: App.jobs
    signal showLog(var job)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        ListView {
            id: list
            objectName: "jobList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.jobs
            ScrollBar.vertical: ScrollBar {}
            delegate: JobRow {
                width: ListView.view.width
                job: model
                onShowLog: (job) => root.showLog(job)
            }
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("Processing runs in the background. Jobs, their progress and their logs appear here.")
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall + 1
            }
        }
    }
}
