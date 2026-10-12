// Swath file inspector: summary statistics, navigation and depth profile.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: page
    signal openRequested()
    readonly property var swath: App.swath
    readonly property var s: swath.summary

    function fmt(v, digits) { return v === undefined ? "–" : Number(v).toFixed(digits) }
    function duration(hours) {
        if (hours === undefined) return "–"
        const seconds = hours * 3600
        if (seconds < 120) return seconds.toFixed(1) + " s"
        if (hours < 2) return (seconds / 60).toFixed(1) + " min"
        return hours.toFixed(2) + " h"
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !swath.loaded && !swath.busy
        iconPath: Icons.waves
        title: swath.error !== "" ? qsTr("The file could not be read") : qsTr("Inspect a swath sonar file")
        message: swath.error !== "" ? swath.error
                 : qsTr("Open a file in any of the formats supported by MB-System (for example .all, .kmall, .s7k, .gsf, .mb88) to see its statistics, navigation and depth profile.")
        actionText: qsTr("Open swath file…")
        onAction: page.openRequested()
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: swath.busy
        spacing: 12
        BusyIndicator { running: swath.busy; Layout.alignment: Qt.AlignHCenter }
        Text { text: qsTr("Reading %1…").arg(swath.fileName); color: Theme.textMuted; font.pixelSize: Theme.fontBody }
        Button { text: qsTr("Cancel"); flat: true; Layout.alignment: Qt.AlignHCenter; onClicked: swath.cancel() }
    }

    ScrollView {
        anchors.fill: parent
        visible: swath.loaded && !swath.busy
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 16

            // header
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.pagePadding
                Layout.bottomMargin: 0
                spacing: 14
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    RowLayout {
                        spacing: 10
                        Text {
                            text: swath.fileName
                            color: Theme.text
                            font.pixelSize: Theme.fontHeadline
                            font.weight: Font.DemiBold
                            elide: Text.ElideMiddle
                            Layout.maximumWidth: page.width * 0.5
                        }
                        Chip { text: "MBIO " + (s.format || ""); tint: Theme.accent }
                        Chip { text: s.formatName || ""; tint: Theme.accentStrong }
                    }
                    Text {
                        text: swath.path
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }
                Button { text: qsTr("mbinfo"); onClicked: App.runToolOn("mbinfo", swath.path) }
                Button { text: qsTr("mblist"); onClicked: App.runToolOn("mblist", swath.path) }
                IconButton { iconPath: Icons.refresh; tip: qsTr("Read again"); onClicked: swath.inspect(swath.path) }
                IconButton { iconPath: Icons.folderOpen; tip: qsTr("Show in folder"); onClicked: App.showInFolder(swath.path) }
            }

            // statistics
            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                columns: page.width > 1150 ? 4 : 2
                columnSpacing: 12
                rowSpacing: 12
                StatTile { Layout.fillWidth: true; label: qsTr("Survey records"); value: s.records !== undefined ? s.records.toLocaleString(Qt.locale(), 'f', 0) : "–"; iconPath: Icons.layers }
                StatTile { Layout.fillWidth: true; label: qsTr("Good beams"); value: fmt(s.goodPercent, 1); unit: "%"; iconPath: Icons.check; tint: Theme.success }
                StatTile { Layout.fillWidth: true; label: qsTr("Duration"); value: duration(s.durationHours); iconPath: Icons.route; tint: Theme.accentStrong }
                StatTile { Layout.fillWidth: true; label: qsTr("Track length"); value: fmt(s.trackLengthKm, 3); unit: "km"; iconPath: Icons.route; tint: Theme.warning }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                spacing: 12

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 3
                    Layout.preferredHeight: 420
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6
                        SectionTitle { text: qsTr("Navigation"); iconPath: Icons.route }
                        TrackPlot {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            points: swath.track
                            geographic: true
                            lineColor: Theme.accent
                            axisColor: Theme.border
                            textColor: Theme.textMuted
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 2
                    Layout.preferredHeight: 420
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8
                        SectionTitle { text: qsTr("Details"); iconPath: Icons.info }
                        Repeater {
                            model: [
                                [qsTr("Start"), s.timeStart || "–"],
                                [qsTr("End"), s.timeEnd || "–"],
                                [qsTr("Longitude"), fmt(s.lonMin, 6) + " … " + fmt(s.lonMax, 6)],
                                [qsTr("Latitude"), fmt(s.latMin, 6) + " … " + fmt(s.latMax, 6)],
                                [qsTr("Depth"), fmt(s.depthMin, 1) + " … " + fmt(s.depthMax, 1) + " m"],
                                [qsTr("Beams per ping"), s.maxBeams !== undefined ? s.maxBeams : "–"],
                                [qsTr("Beams total / good"), (s.beamsTotal || 0) + " / " + (s.beamsGood || 0)],
                                [qsTr("Flagged beams"), s.beamsFlagged !== undefined ? s.beamsFlagged : "–"]
                            ]
                            delegate: RowLayout {
                                Layout.fillWidth: true
                                Text { text: modelData[0]; color: Theme.textMuted; font.pixelSize: Theme.fontBody; Layout.preferredWidth: 130 }
                                Text {
                                    text: modelData[1]
                                    color: Theme.text
                                    font.pixelSize: Theme.fontBody
                                    font.family: Theme.monoFont
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideLeft
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pagePadding
                Layout.rightMargin: Theme.pagePadding
                Layout.bottomMargin: Theme.pagePadding
                Layout.preferredHeight: 260
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    SectionTitle { text: qsTr("Centre beam depth"); iconPath: Icons.terrain }
                    TrackPlot {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        points: swath.profile
                        geographic: false
                        invertY: true
                        xLabel: qsTr("seconds since start of file")
                        lineColor: Theme.accentStrong
                        axisColor: Theme.border
                        textColor: Theme.textMuted
                    }
                }
            }
        }
    }
}
