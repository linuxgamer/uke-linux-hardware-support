// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    objectName: "kirigamiWindow"
    title: "Uke Control"
    visible: true
    width: Kirigami.Units.gridUnit * 60
    height: Kirigami.Units.gridUnit * 42
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 24
    property bool initialized: false
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.onCurrentIndexChanged: {
        if (initialized && pageStack.currentIndex === 0) backend.cancelRead()
    }
    pageStack.onPageRemoved: page => page.destroy()
    function showArea(index) {
        backend.selectArea(index)
        pageStack.clear()
        pageStack.push(areaPage)
        if (globalDrawer.modal) globalDrawer.close()
    }
    function openGuide(id) {
        while (pageStack.depth > 1) pageStack.pop()
        if (backend.openAction(id)) {
            pageStack.push(guidePage)
        }
    }
    globalDrawer: Kirigami.GlobalDrawer {
        title: "Uke Control"
        titleIcon: "preferences-system"
        isMenu: false
        modal: !root.wideScreen
        drawerOpen: !modal
        actions: [
            Kirigami.Action { text: "Status"; icon.name: "dialog-information"; checked: backend.section === 0; onTriggered: root.showArea(0) },
            Kirigami.Action { text: "Power"; icon.name: "battery"; checked: backend.section === 1; onTriggered: root.showArea(1) },
            Kirigami.Action { text: "Display & lights"; icon.name: "video-display"; checked: backend.section === 2; onTriggered: root.showArea(2) },
            Kirigami.Action { text: "Connections"; icon.name: "network-wired"; checked: backend.section === 3; onTriggered: root.showArea(3) },
            Kirigami.Action { text: "Audio & sensors"; icon.name: "audio-card"; checked: backend.section === 4; onTriggered: root.showArea(4) },
            Kirigami.Action { text: "Updates"; icon.name: "system-software-update"; checked: backend.section === 5; onTriggered: root.showArea(5) },
            Kirigami.Action { text: "Advanced"; icon.name: "preferences-system"; checked: backend.section === 6; onTriggered: root.showArea(6) }
        ]
    }
    pageStack.initialPage: areaPage
    pageStack.onDepthChanged: {
        if (initialized && pageStack.depth === 1) backend.leaveGuide()
    }
    Component { id: areaPage; AreaPage { parent: root.pageStack; appWindow: root } }
    Component { id: guidePage; GuidePage { parent: root.pageStack; appWindow: root } }
    ReviewSheet { id: reviewSheet }
    Connections {
        target: backend
        function onReviewChanged() {
            if (backend.review.title !== undefined) reviewSheet.open()
            else reviewSheet.close()
        }
    }
    Shortcut { sequence: StandardKey.Quit; onActivated: root.close() }
    Shortcut { sequence: "Alt+Left"; enabled: pageStack.depth > 1; onActivated: pageStack.pop() }
    onClosing: backend.cancelRead()
    Component.onCompleted: {
        if (backend.guide.id !== undefined) pageStack.push(guidePage)
        initialized = true
    }
}
