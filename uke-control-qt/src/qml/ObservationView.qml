// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: view
    spacing: Kirigami.Units.smallSpacing
    FormCard.FormHeader {
        title: backend.observation.title || "Observation"
    }
    Controls.Label {
        Layout.fillWidth: true
        text: (backend.observation.state || "") + " · " + (backend.observation.stamp || "")
        wrapMode: Text.WordWrap
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: (backend.observation.notice || "").length > 0
        text: backend.observation.notice || ""
        type: backend.observation.failed ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
    }
    RowLayout {
        Layout.fillWidth: true
        Controls.BusyIndicator { running: backend.observation.running || false; visible: running }
        Item { Layout.fillWidth: true }
        Controls.Button {
            objectName: "refreshRead"
            text: "Refresh"
            icon.name: "view-refresh"
            enabled: !backend.observation.running
            onClicked: backend.refresh()
        }
        Controls.Button {
            objectName: "stopRead"
            text: "Stop"
            icon.name: "process-stop"
            enabled: backend.observation.running || false
            onClicked: backend.cancelRead()
        }
    }
    FormCard.FormCard {
        Layout.fillWidth: true
        Repeater {
            model: backend.observation.rows || []
            delegate: SafeText {
                required property var modelData
                text: "  ".repeat(modelData.depth) + modelData.key
                description: modelData.value
            }
        }
    }
    FormCard.FormCard {
        Layout.fillWidth: true
        FormCard.FormButtonDelegate {
            text: "Raw output & errors"
            icon.name: "utilities-terminal"
            onClicked: rawDetails.visible = !rawDetails.visible
        }
        ColumnLayout {
            id: rawDetails
            visible: false
            Layout.fillWidth: true
            Controls.Label { text: "Raw output"; Layout.leftMargin: Kirigami.Units.largeSpacing }
            Controls.TextArea {
                objectName: "rawOutput"
                Layout.fillWidth: true
                text: backend.observation.raw || "No stdout reported."
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
            }
            Controls.Label { text: "Errors · exit " + backend.observation.exitStatus; Layout.leftMargin: Kirigami.Units.largeSpacing }
            Controls.TextArea {
                objectName: "errorOutput"
                Layout.fillWidth: true
                text: backend.observation.error || "No stderr reported."
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
            }
        }
    }
}
