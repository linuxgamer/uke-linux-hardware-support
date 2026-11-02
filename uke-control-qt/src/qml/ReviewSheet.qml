// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardDialog {
    id: dialog
    objectName: "requestReview"
    title: "Review changes"
    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 32)
    height: Math.min(implicitHeight, parent.height - Kirigami.Units.largeSpacing * 2)
    standardButtons: Controls.Dialog.Cancel
    onOpened: { consent.checked = false; confirmation.text = "" }
    onRejected: backend.cancelReview()
    SafeText {
        text: backend.review.title || ""
        description: backend.review.description || ""
    }
    Repeater {
        model: backend.review.values || []
        delegate: SafeText {
            required property var modelData
            text: modelData.title
            description: modelData.value
        }
    }
    SafeText {
        visible: (backend.review.warning || "").length > 0
        text: "Access & effects"
        description: backend.review.warning || ""
    }
    FormCard.FormTextFieldDelegate {
        id: confirmation
        objectName: "confirmationToken"
        visible: (backend.review.token || "").length > 0
        label: "Type " + (backend.review.token || "") + " to confirm"
        Accessible.name: label
    }
    FormCard.FormCheckDelegate {
        id: consent
        objectName: "reviewConsent"
        text: "I reviewed the request and its effects."
        Accessible.name: text
    }
    SafeText {
        visible: backend.review.blocked || false
        text: "Action disabled"
        description: backend.review.reason || ""
    }
    FormCard.FormButtonDelegate {
        text: "Validated arguments"
        onClicked: exact.visible = !exact.visible
    }
    SafeText {
        id: exact
        visible: false
        text: "Command"
        description: backend.review.command || ""
    }
    FormCard.FormButtonDelegate {
        objectName: "launchTerminal"
        text: "Continue in terminal"
        icon.name: "utilities-terminal"
        enabled: !(backend.review.blocked || false) && consent.checked && confirmation.text === (backend.review.token || "")
        onClicked: backend.confirmReview(consent.checked, confirmation.text)
    }
}
