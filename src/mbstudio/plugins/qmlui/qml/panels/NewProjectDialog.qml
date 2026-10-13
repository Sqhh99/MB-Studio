// Create a project: name and folder, optionally adding a folder of survey data.
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Dialog {
    id: root
    title: qsTr("New project")
    modal: true
    anchors.centerIn: parent
    width: 560
    standardButtons: Dialog.Ok | Dialog.Cancel
    property string folder: ""
    property string dataFolder: ""

    onAboutToShow: {
        nameField.text = qsTr("Survey")
        folder = ""
        dataFolder = ""
    }
    onAccepted: {
        if (folder === "") return
        const path = folder + "/" + nameField.text.trim()
        if (App.project.createProject(path, nameField.text.trim())) {
            App.openWorkspace("map")
            if (dataFolder !== "")
                App.project.addData([dataFolder])
        }
    }

    FolderDialog { id: folderDialog; title: qsTr("Folder for the project"); onAccepted: root.folder = App.toLocalFile(selectedFolder) }
    FolderDialog { id: dataDialog; title: qsTr("Folder with survey data"); onAccepted: root.dataFolder = App.toLocalFile(selectedFolder) }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Text { text: qsTr("Name"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        TextField { id: nameField; objectName: "projectName"; Layout.fillWidth: true; selectByMouse: true }
        Text { text: qsTr("Create in"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
        RowLayout {
            Layout.fillWidth: true
            TextField { Layout.fillWidth: true; text: root.folder; placeholderText: qsTr("Choose a folder"); onEditingFinished: root.folder = text; selectByMouse: true }
            Button { text: qsTr("Browse…"); flat: true; onClicked: folderDialog.open() }
        }
        Text {
            Layout.fillWidth: true
            visible: root.folder !== ""
            text: qsTr("The project folder will be %1").arg(root.folder + "/" + nameField.text.trim())
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WrapAnywhere
        }
        Text { text: qsTr("Survey data (optional)"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall; Layout.topMargin: 6 }
        RowLayout {
            Layout.fillWidth: true
            TextField { Layout.fillWidth: true; text: root.dataFolder; placeholderText: qsTr("A folder with .all, .kmall, .s7k, .gsf, … files"); onEditingFinished: root.dataFolder = text; selectByMouse: true }
            Button { text: qsTr("Browse…"); flat: true; onClicked: dataDialog.open() }
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("The data files stay where they are; the project references them. MB-System cannot read datalists with spaces in the path between the project and the data.")
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
