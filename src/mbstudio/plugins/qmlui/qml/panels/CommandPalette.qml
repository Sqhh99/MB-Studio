// Search every menu action, view and plugin command (Ctrl+Shift+P).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Popup {
    id: root
    modal: true
    focus: true
    width: Math.min(parent ? parent.width - 120 : 640, 640)
    height: 420
    x: parent ? (parent.width - width) / 2 : 0
    y: 80
    padding: 0
    property var entries: []
    property var filtered: []

    function rebuild() {
        const out = []
        for (const a of App.actions)
            out.push({ title: a.title.replace("…", ""), detail: a.menu, command: a.command })
        for (const w of App.workspaces.available)
            out.push({ title: w.title, detail: qsTr("view"), command: "ui:workspace:" + w.id })
        for (const c of App.commands)
            out.push({ title: c.id, detail: c.description, command: c.id })
        entries = out
        filter()
    }
    function filter() {
        const q = search.text.toLowerCase()
        filtered = q === "" ? entries : entries.filter(e => e.title.toLowerCase().indexOf(q) >= 0 || e.detail.toLowerCase().indexOf(q) >= 0)
        list.currentIndex = filtered.length > 0 ? 0 : -1
    }
    function run(index) {
        if (index < 0 || index >= filtered.length) return
        const command = filtered[index].command
        close()
        App.trigger(command)
    }
    onOpened: {
        search.text = ""
        rebuild()
        search.forceActiveFocus()
    }

    background: Rectangle { color: Theme.surface; radius: Theme.radius; border.color: Theme.border }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        TextField {
            id: search
            objectName: "paletteSearch"
            Layout.fillWidth: true
            Layout.margins: 10
            placeholderText: qsTr("Type a command, a view or a program step")
            onTextChanged: root.filter()
            Keys.onDownPressed: list.incrementCurrentIndex()
            Keys.onUpPressed: list.decrementCurrentIndex()
            Keys.onReturnPressed: root.run(list.currentIndex)
            Keys.onEnterPressed: root.run(list.currentIndex)
            Keys.onEscapePressed: root.close()
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.filtered
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                width: ListView.view.width
                implicitHeight: 40
                highlighted: ListView.isCurrentItem
                onClicked: root.run(index)
                contentItem: RowLayout {
                    spacing: 10
                    Text { text: modelData.title; color: Theme.text; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
                    Text { text: modelData.detail; color: Theme.textMuted; font.pixelSize: Theme.fontSmall; elide: Text.ElideRight; Layout.maximumWidth: 260 }
                }
            }
        }
    }
}
