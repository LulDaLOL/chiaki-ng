import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

// DA Remote Play: brightness panel (screen backlight, display boost, optional stream color)
Popup {
    id: popup
    property bool showColor: false

    width: 420
    padding: 24
    modal: true
    dim: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: {
        DaDisplay.refresh();
        if (DaDisplay.screenBrightnessSupported)
            screenSlider.forceActiveFocus(Qt.TabFocusReason);
        else
            boostSlider.forceActiveFocus(Qt.TabFocusReason);
    }

    background: Rectangle {
        radius: 18
        color: "#272C3D"
        border.width: 1
        border.color: "#3A4156"
    }

    contentItem: ColumnLayout {
        spacing: 6

        Label {
            text: qsTr("Brightness")
            font.pixelSize: 21
            font.weight: Font.DemiBold
        }

        Label {
            Layout.topMargin: 10
            text: qsTr("Screen")
            font.pixelSize: 15
            color: "#B4BBCC"
        }

        RowLayout {
            Layout.fillWidth: true
            Slider {
                id: screenSlider
                Layout.fillWidth: true
                from: 0
                to: 100
                stepSize: 1
                enabled: DaDisplay.screenBrightnessSupported
                value: DaDisplay.screenBrightness
                onMoved: DaDisplay.screenBrightness = Math.round(value)
                KeyNavigation.down: boostSlider
                Keys.onEscapePressed: popup.close()
            }
            Label {
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignRight
                visible: DaDisplay.screenBrightnessSupported
                text: Math.round(screenSlider.value) + "%"
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !DaDisplay.screenBrightnessSupported
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            color: "#B4BBCC"
            text: qsTr("This screen's backlight can't be set by apps (usual for external monitors). Use Boost below, or the monitor's own buttons.")
        }

        Label {
            Layout.topMargin: 12
            text: qsTr("Boost  (lifts dark scenes, whole screen)")
            font.pixelSize: 15
            color: "#B4BBCC"
        }

        RowLayout {
            Layout.fillWidth: true
            Slider {
                id: boostSlider
                Layout.fillWidth: true
                from: 0
                to: 100
                stepSize: 1
                value: DaDisplay.boost
                onMoved: DaDisplay.boost = Math.round(value)
                KeyNavigation.up: screenSlider
                KeyNavigation.down: popup.showColor ? colorSlider : resetButton
                Keys.onEscapePressed: popup.close()
            }
            Label {
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignRight
                text: Math.round(boostSlider.value) + "%"
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !DaDisplay.boostSupported
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            color: "#F5A524"
            text: qsTr("Windows blocked the boost on this screen (it doesn't work while HDR is on).")
        }

        Label {
            Layout.topMargin: 12
            visible: popup.showColor
            text: qsTr("Stream color")
            font.pixelSize: 15
            color: "#B4BBCC"
        }

        RowLayout {
            Layout.fillWidth: true
            visible: popup.showColor
            Slider {
                id: colorSlider
                Layout.fillWidth: true
                from: 0.0
                to: 2.0
                stepSize: 0.01
                value: Chiaki.settings.placeboColorAdjustmentSaturation
                onMoved: {
                    Chiaki.settings.placeboColorAdjustmentSaturation = value;
                    Chiaki.window.setPictureAdjust(0.0, value);
                }
                KeyNavigation.up: boostSlider
                KeyNavigation.down: resetButton
                Keys.onEscapePressed: popup.close()
            }
            Label {
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignRight
                text: Math.round(colorSlider.value * 100) + "%"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 14
            Item { Layout.fillWidth: true }
            Button {
                id: resetButton
                text: qsTr("Reset")
                flat: true
                onClicked: {
                    DaDisplay.boost = 0;
                    boostSlider.value = 0;
                    if (popup.showColor) {
                        Chiaki.settings.placeboColorAdjustmentSaturation = 1.0;
                        colorSlider.value = 1.0;
                        Chiaki.window.setPictureAdjust(0.0, 1.0);
                    }
                }
                Keys.onReturnPressed: clicked()
                Keys.onEscapePressed: popup.close()
            }
            Button {
                text: qsTr("Done")
                highlighted: true
                onClicked: popup.close()
                Keys.onReturnPressed: clicked()
                Keys.onEscapePressed: popup.close()
            }
        }
    }
}
