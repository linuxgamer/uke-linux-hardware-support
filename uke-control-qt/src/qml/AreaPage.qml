// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "areaPage"
    required property var appWindow
    title: backend.areas[backend.section].title
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: backend.simulation ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: backend.simulation ? "Simulation — sample data only. Device commands and changes are disabled." : "Read-only session. Changes are disabled."
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: styleNotice.length > 0
        type: Kirigami.MessageType.Warning
        text: styleNotice
    }
    SafeText {
        text: backend.section === 6 ? "Expert tools" : "Device settings"
        description: backend.section === 6 ? "Diagnostics, recovery and the full action catalog. Each operation keeps its own review and access requirements." : "Choose an item to inspect its current state or review a preference. Opening a page does not run device commands."
    }
    Repeater {
        model: backend.groups
        delegate: ColumnLayout {
            id: groupColumn
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            FormCard.FormHeader { title: groupColumn.modelData.title }
            FormCard.FormCard {
                Layout.fillWidth: true
                Repeater {
                    model: groupColumn.modelData.entries
                    delegate: FormCard.FormButtonDelegate {
                        required property var modelData
                        objectName: "action_" + modelData.id
                        text: modelData.title
                        description: modelData.description
                        icon.name: modelData.icon
                        onClicked: page.appWindow.openGuide(modelData.id)
                    }
                }
            }
        }
    }
    Controls.TextField {
        id: searchField
        objectName: "advancedSearch"
        Layout.fillWidth: true
        visible: backend.section === 6
        placeholderText: "Search all actions…"
        Accessible.name: "Search all actions"
    }
    FormCard.FormCard {
        visible: backend.section === 6
        Repeater {
            id: searchResults
            model: backend.section === 6 ? backend.search(searchField.text) : []
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                text: modelData.title
                description: modelData.id
                icon.name: modelData.icon
                onClicked: page.appWindow.openGuide(modelData.id)
            }
        }
        SafeText {
            visible: searchResults.count === 0
            text: "No matching actions"
            description: "Clear the search or try another word."
        }
    }
}
