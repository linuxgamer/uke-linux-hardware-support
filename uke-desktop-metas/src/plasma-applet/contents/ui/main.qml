// SPDX-License-Identifier: GPL-3.0-or-later
// Layout adapted from SENEMOS Nabu Tablet Control; see SOURCE.md.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as Plasma5Support
import "Control.js" as Control

PlasmoidItem {
    id: root
    property var panel: ({})
    property var desktop: ({})
    property var usb: ({})
    property var lights: ({})
    property var requests: ({})
    property string errorText: ""
    property int serial: 0
    property int pending: 0
    property string requestedAction: ""

    function execute(action) {
        const command = Control.command(action)
        if (!command) return
        const source = "UKE_TRAY_REQUEST=" + (++serial) + " " + command
        requests[source] = action
        ++pending
        executable.connectSource(source)
    }
    function refresh() {
        if (pending > 0) return
        execute("plasma")
        execute("desktop")
        execute("usb")
        execute("lights")
    }
    function change(action) {
        if (pending > 0) return
        errorText = ""
        execute(action)
    }
    function finish(source, data) {
        const action = requests[source]
        if (!action) return
        delete requests[source]
        --pending
        const query = action === "plasma" || action === "desktop" || action === "usb" || action === "lights"
        if (query) {
            let value = {}
            try {
                if (data["exit code"] !== 0) throw new Error("uke-control failed")
                value = Control.parse(action, String(data.stdout || ""))
            } catch (error) {
                errorText = i18n("Could not read tablet state. Refresh to try again.")
            }
            if (action === "plasma") panel = value
            else if (action === "desktop") desktop = value
            else if (action === "usb") usb = value
            else lights = value
        } else {
            if (data["exit code"] !== 0)
                errorText = i18n("The change failed or authorization was cancelled.")
            refresh()
        }
    }
    function role() {
        const ports = usb.typec || []
        for (let i = 0; i < ports.length; ++i)
            if (ports[i].name === "port0") return String(ports[i].data_role || i18n("Unavailable"))
        return i18n("Unavailable")
    }
    function canRequestRole() {
        const ports = usb.typec || []
        for (let i = 0; i < ports.length; ++i)
            if (ports[i].name === "port0") return String(ports[i].port_type).indexOf("dual") >= 0
        return false
    }
    function confirm(action) {
        requestedAction = action
        confirmation.open()
    }

    Plasmoid.icon: "input-tablet"
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: i18n("Uke Tablet Controls")
    toolTipSubText: i18n("Display, cover and USB")
    onExpandedChanged: { if (expanded) refresh() }

    Timer {
        interval: 2000
        repeat: true
        running: root.expanded && root.lights.flashlight?.on === true
        onTriggered: { if (root.pending === 0) root.execute("lights") }
    }

    Plasma5Support.DataSource {
        id: executable
        engine: "executable"
        onNewData: function(source, data) {
            disconnectSource(source)
            root.finish(source, data)
        }
    }

    compactRepresentation: PC.ToolButton {
        icon.name: "input-tablet"
        Accessible.name: root.toolTipMainText
        onClicked: root.expanded = !root.expanded
    }

    fullRepresentation: PC.ScrollView {
        id: scrollView
        Layout.minimumWidth: Kirigami.Units.gridUnit * 22
        Layout.preferredWidth: Kirigami.Units.gridUnit * 26
        Layout.minimumHeight: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: Kirigami.Units.gridUnit * 30
        contentWidth: availableWidth
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                Kirigami.Heading { Layout.fillWidth: true; text: i18n("Uke Tablet Controls"); level: 2 }
                PC.ToolButton {
                    icon.name: "view-refresh"
                    text: i18n("Refresh")
                    display: PC.AbstractButton.IconOnly
                    Accessible.name: text
                    enabled: root.pending === 0
                    onClicked: root.refresh()
                }
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                visible: root.errorText.length > 0
                text: root.errorText
                type: Kirigami.MessageType.Error
            }
            ControlRow {
                title: i18n("Automatic rotation")
                description: root.panel.rotation === true ? i18n("Always") : i18n("Off or tablet mode only")
                iconName: "object-rotate-right"
                available: typeof root.panel.rotation === "boolean"
                busy: root.pending > 0
                action: Component {
                    PC.Switch {
                        checked: root.panel.rotation === true
                        Accessible.name: i18n("Automatic rotation")
                        onToggled: root.change(checked ? "rotationOn" : "rotationOff")
                    }
                }
            }
            ControlRow {
                title: i18n("Automatic brightness")
                description: root.panel.automatic_brightness === true ? i18n("On") : i18n("Off")
                iconName: "brightness-high"
                available: typeof root.panel.automatic_brightness === "boolean"
                busy: root.pending > 0
                action: Component {
                    PC.Switch {
                        checked: root.panel.automatic_brightness === true
                        Accessible.name: i18n("Automatic brightness")
                        onToggled: root.change(checked ? "brightnessOn" : "brightnessOff")
                    }
                }
            }
            ControlRow {
                title: i18n("Cover")
                iconName: "computer-laptop"
                available: Control.property(root.desktop, "org.freedesktop.UPower", "LidIsPresent") === true
                description: Control.property(root.desktop, "org.freedesktop.UPower", "LidIsClosed") === true
                    ? i18n("Closed") : i18n("Open")
            }
            ControlRow {
                title: i18n("USB data role")
                iconName: "usb-creator"
                description: root.role()
                available: root.canRequestRole()
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Button { text: i18n("Host"); onClicked: root.confirm("host") }
                        PC.Button { text: i18n("Device"); onClicked: root.confirm("device") }
                    }
                }
            }
            ControlRow {
                title: i18n("USB serial at startup")
                iconName: "network-wired"
                description: typeof root.usb.cdc_saved_policy === "string" ? root.usb.cdc_saved_policy : i18n("State unavailable")
                available: root.usb.schema === "org.uke.control.usb.v1"
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Button { text: i18n("Enable"); onClicked: root.confirm("cdcOn") }
                        PC.Button { text: i18n("Disable"); onClicked: root.confirm("cdcOff") }
                    }
                }
            }
            ControlRow {
                title: i18n("Flashlight")
                iconName: "flashlight-off"
                description: i18n("20 mA total; switches off within 60 seconds")
                available: root.lights.flashlight?.available === true
                busy: root.pending > 0
                action: Component {
                    PC.Switch {
                        checked: root.lights.flashlight?.on === true
                        Accessible.name: i18n("Flashlight")
                        onToggled: root.change(checked ? "flashlightOn" : "flashlightOff")
                    }
                }
            }
            ControlRow {
                title: i18n("Notification LED")
                iconName: "color-management"
                description: i18n("Low brightness; administrator authorization required")
                available: root.lights.rgb?.available === true
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Button { text: i18n("Red"); onClicked: root.change("red") }
                        PC.Button { text: i18n("Green"); onClicked: root.change("green") }
                        PC.Button { text: i18n("Blue"); onClicked: root.change("blue") }
                        PC.Button { text: i18n("Off"); onClicked: root.change("ledOff") }
                    }
                }
            }
            PC.Label {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                wrapMode: Text.WordWrap
                text: i18n("Unsupported controls remain unavailable. Refresh reads the current state.")
            }
        }
        QQC2.Dialog {
            id: confirmation
            anchors.centerIn: parent
            width: Kirigami.Units.gridUnit * 22
            modal: true
            title: i18n("Change USB configuration?")
            standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
            contentItem: PC.Label {
                text: i18n("This can disconnect USB accessories or a remote session. Serial startup settings are saved. Administrator authorization is required.")
                wrapMode: Text.WordWrap
            }
            onAccepted: root.change(root.requestedAction)
        }
    }
}
