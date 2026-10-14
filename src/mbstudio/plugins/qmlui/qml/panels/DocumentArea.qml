// The central document area: the open documents (map, grids, 3D views,
// survey lines, sounding editors, figures, ...) in tabs, in one group or in
// two groups side by side. Every document is created once and keeps its
// state while it is hidden or moved to the other group.
import QtQuick
import QtQuick.Controls
import MBStudio.Backend
import ".."

Item {
    id: root
    readonly property var workspaces: App.workspaces

    SplitView {
        id: split
        anchors.fill: parent
        orientation: Qt.Horizontal
        DocumentGroup {
            id: left
            group: 0
            SplitView.fillWidth: true
            SplitView.minimumWidth: 220
        }
        DocumentGroup {
            id: right
            group: 1
            visible: root.workspaces.split
            SplitView.preferredWidth: root.width / 2
            SplitView.minimumWidth: 220
        }
    }

    Repeater {
        model: root.workspaces
        delegate: Item {
            id: documentHost
            required property string url
            required property var document
            required property string argument
            required property int group
            required property bool shown
            // Repeater keeps its delegates as siblings; only the contained
            // document moves between the visual hosts of the split groups.
            Loader {
                parent: (documentHost.group === 1 ? right.host : left.host) || null
                anchors.fill: parent
                source: documentHost.url
                visible: documentHost.shown
                onLoaded: {
                    if (item !== null && item.document !== undefined)
                        item.document = Qt.binding(function() { return documentHost.document })
                    if (item !== null && item.argument !== undefined)
                        item.argument = Qt.binding(function() { return documentHost.argument })
                }
            }
        }
    }
}
