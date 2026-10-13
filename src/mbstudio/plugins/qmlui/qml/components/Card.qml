// Rounded surface used to group content.
import QtQuick
import ".."

Rectangle {
    id: root
    default property alias content: inner.data
    property int padding: 18
    color: Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    border.width: 1

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
