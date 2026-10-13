import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."

Dialog {
    id: root
    title: qsTr("About MB-Studio")
    modal: true
    anchors.centerIn: parent
    width: 460
    standardButtons: Dialog.Close
    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/mbstudio/images/mbstudio.png"
            sourceSize: Qt.size(128, 128)
            Layout.preferredWidth: 72
            Layout.preferredHeight: 72
        }
        Text { Layout.alignment: Qt.AlignHCenter; text: "MB-Studio " + App.version; color: Theme.text; font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
        Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("MB-System %1").arg(App.mbsystemVersion); color: Theme.textMuted }
        Text { Layout.alignment: Qt.AlignHCenter; text: App.platformName; color: Theme.textFaint; font.pixelSize: Theme.fontSmall }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Processing and visualisation of swath sonar and seafloor mapping data with MB-System. Free software under the GNU General Public License version 3.")
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
