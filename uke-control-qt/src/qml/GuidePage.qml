// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "guidePage"
    required property var appWindow
    title: backend.guide.title || "Device settings"
    actions: [
        Kirigami.Action {
            text: backend.guide.read ? (backend.guide.capture ? "Read information" : "Review access…") : "Review changes…"
            icon.name: backend.guide.read ? "view-refresh" : "dialog-ok-apply"
            enabled: !backend.observation.running
            onTriggered: backend.submit()
        }
    ]
    Kirigami.InlineMessage {
        objectName: "sessionNotice"
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: backend.simulation ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: backend.simulation ? "Simulation — sample data only. Device commands and changes are disabled." : "Read-only session. Changes are disabled."
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.message.length > 0
        text: backend.message
    }
    SafeText {
        text: backend.guide.title || ""
        description: backend.guide.description || ""
    }
    FormCard.FormHeader {
        visible: parameters.count > 0
        title: backend.guide.read ? "Options" : "Your preference"
    }
    FormCard.FormCard {
        visible: parameters.count > 0
        SafeText {
            text: "Draft preference"
            description: "Nothing changes until you review and continue."
        }
        Repeater {
            id: parameters
            model: backend.guide.parameters || []
            delegate: Loader {
                id: fieldLoader
                required property var modelData
                Layout.fillWidth: true
                sourceComponent: modelData.choices.length > 0 ? choiceField : modelData.integer ? integerField : textField
                Component {
                    id: integerField
                    FormCard.FormSpinBoxDelegate {
                        objectName: "parameter_" + fieldLoader.modelData.key
                        label: fieldLoader.modelData.title
                        Accessible.name: label
                        from: fieldLoader.modelData.minimum
                        to: fieldLoader.modelData.maximum
                        value: Number(fieldLoader.modelData.value)
                        onValueModified: backend.setDraft(fieldLoader.modelData.key, String(value))
                    }
                }
                Component {
                    id: choiceField
                    FormCard.FormComboBoxDelegate {
                        objectName: "parameter_" + fieldLoader.modelData.key
                        text: fieldLoader.modelData.title
                        Accessible.name: text
                        description: currentIndex >= 0 && model[currentIndex] ? model[currentIndex].description : fieldLoader.modelData.description
                        model: fieldLoader.modelData.choices
                        textRole: "title"
                        valueRole: "value"
                        displayMode: page.width < Kirigami.Units.gridUnit * 35 ? FormCard.FormComboBoxDelegate.Dialog : FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: {
                            for (let i = 0; i < model.length; ++i)
                                if (model[i].value === fieldLoader.modelData.value) return i
                            return 0
                        }
                        onActivated: index => backend.setDraft(fieldLoader.modelData.key, model[index].value)
                    }
                }
                Component {
                    id: textField
                    FormCard.FormTextFieldDelegate {
                        objectName: "parameter_" + fieldLoader.modelData.key
                        label: fieldLoader.modelData.title
                        Accessible.name: label
                        text: fieldLoader.modelData.value
                        maximumLength: 4096
                        onTextEdited: backend.setDraft(fieldLoader.modelData.key, text)
                    }
                }
            }
        }
    }
    FormCard.FormHeader {
        visible: Boolean(backend.guide.inspect || backend.guide.preview)
        title: "Before applying"
    }
    FormCard.FormCard {
        visible: Boolean(backend.guide.inspect || backend.guide.preview)
        FormCard.FormButtonDelegate {
            objectName: "inspectCurrent"
            visible: backend.guide.inspect || false
            text: "Inspect current state"
            icon.name: "view-list-details"
            onClicked: backend.readRelated("inspect")
        }
        FormCard.FormButtonDelegate {
            visible: backend.guide.preview || false
            text: "Preview requested values"
            icon.name: "document-preview"
            onClicked: backend.readRelated("preview")
        }
    }
    ObservationView { Layout.fillWidth: true; visible: backend.observation.title !== undefined }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: "Details & access requirements"
            icon.name: "dialog-information"
            onClicked: details.visible = !details.visible
        }
        SafeText {
            id: details
            visible: false
            text: backend.guide.id || ""
            description: (backend.guide.details || "") + "\n\nEffect: " + (backend.guide.effect || "") + "\nAccess: " + (backend.guide.access || "")
        }
    }
}
