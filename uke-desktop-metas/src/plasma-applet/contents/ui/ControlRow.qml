import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: control

    property string title
    property string description
    property string iconName
    property bool capabilityKnown: true
    property bool available: true
    property bool controlEnabled: available
    property bool busy: false
    property Component action
    property Component details

    Layout.fillWidth: true
    implicitHeight: content.implicitHeight + Kirigami.Units.largeSpacing * 2
    opacity: capabilityKnown && !available ? 0.58 : 1

    ColumnLayout {
        id: content
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: Kirigami.Units.largeSpacing
            rightMargin: Kirigami.Units.largeSpacing
        }
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                source: control.iconName
                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                implicitHeight: implicitWidth
                visible: control.iconName.length > 0
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: control.title
                    textFormat: Text.PlainText
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }

                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: !control.capabilityKnown
                        ? i18n("Checking hardware support…")
                        : control.available
                            ? control.description
                            : i18n("Unavailable through uke-control")
                    textFormat: Text.PlainText
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                }
            }

            PlasmaComponents.BusyIndicator {
                running: control.busy
                visible: running
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: implicitWidth
            }

            Loader {
                sourceComponent: control.action
                enabled: control.capabilityKnown && control.available && control.controlEnabled && !control.busy
                opacity: enabled ? 1 : 0.55
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.leftMargin: control.iconName.length > 0
                ? Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.largeSpacing : 0
            sourceComponent: control.details
            enabled: control.capabilityKnown && control.available && control.controlEnabled && !control.busy
            opacity: enabled ? 1 : 0.55
            visible: sourceComponent !== null
        }
    }

    Kirigami.Separator {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
