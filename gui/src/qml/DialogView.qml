import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: dialog
    property alias header: headerLabel.text
    property alias title: titleLabel.text
    property alias buttonText: okButton.text
    property alias buttonEnabled: okButton.enabled
    property alias buttonVisible: okButton.visible
    property Item restoreFocusItem
    default property Item mainItem: null

    signal accepted()
    signal rejected()

    function close() {
        root.closeDialog();
    }

    Keys.onEscapePressed: close()

    Keys.onMenuPressed: {
        if (okButton.enabled)
            okButton.clicked()
    }

    StackView.onDeactivating: {
        restoreFocusItem = Window.window.activeFocusItem;
    }

    StackView.onActivated: {
        if (!restoreFocusItem) {
            let item = mainItem.nextItemInFocusChain();
            if (item)
                item.forceActiveFocus(Qt.TabFocusReason);
        } else {
            restoreFocusItem.forceActiveFocus(Qt.TabFocusReason);
            restoreFocusItem = null;
        }
    }

    onMainItemChanged: {
        if (mainItem) {
            mainItem.parent = contentItem;
            mainItem.anchors.fill = contentItem;
        }
    }

    // DA Remote Play: dialog header
    ToolBar {
        id: toolBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 84
        background: Rectangle {
            color: "#22273A"
            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 1
                color: "#3A4158"
            }
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 16
                rightMargin: 16
            }
            spacing: 14

            Button {
                Layout.alignment: Qt.AlignVCenter
                flat: true
                text: "←  " + qsTr("Back")
                font.pixelSize: 16
                focusPolicy: Qt.NoFocus
                Material.roundedScale: Material.SmallScale
                onClicked: {
                    dialog.rejected();
                    dialog.close();
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                spacing: 2

                Label {
                    id: titleLabel
                    font.bold: true
                    font.pixelSize: 24
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    id: headerLabel
                    visible: text.length > 0
                    font.pixelSize: 13
                    color: "#AEB5C7"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Button {
                id: okButton
                Layout.alignment: Qt.AlignVCenter
                highlighted: true
                font.pixelSize: 17
                font.weight: Font.DemiBold
                leftPadding: 26
                rightPadding: 26
                focusPolicy: Qt.NoFocus
                Material.roundedScale: Material.MediumScale
                onClicked: dialog.accepted()
                icon.source: "qrc:/icons/options.svg";
                icon.width: 28
                icon.height: 28
            }
        }
    }

    Item {
        id: contentItem
        anchors {
            top: toolBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
