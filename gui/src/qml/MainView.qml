import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

// DA Remote Play home screen
Pane {
    id: consolePane
    padding: 0

    readonly property color headerColor: "#141823"
    readonly property color cardColor: "#171B26"
    readonly property color cardColorActive: "#1E2333"
    readonly property color borderColor: "#262C3A"
    readonly property color mutedColor: "#8A92A6"
    readonly property color goodColor: "#22C55E"
    readonly property color warnColor: "#F59E0B"
    readonly property int visibleHostCount: {
        let n = 0;
        for (let i = 0; i < Chiaki.hosts.length; i++) {
            if (Chiaki.hosts[i].display)
                n++;
        }
        return n;
    }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#161A2A" }
            GradientStop { position: 0.45; color: "#0E1016" }
            GradientStop { position: 1.0; color: "#0E1016" }
        }
    }

    StackView.onActivated: {
        forceActiveFocus(Qt.TabFocusReason);
        if(!Chiaki.autoConnect && !root.initialAsk && !Chiaki.window.directStream)
        {
            root.initialAsk = true;
            if(Chiaki.settings.addSteamShortcutAsk && (typeof Chiaki.createSteamShortcut === "function"))
                root.showRemindDialog(qsTr("Official Steam artwork + controller layout"), qsTr("Would you like to either create a new non-Steam game for DA Remote Play\nor update an existing non-Steam game with the official artwork and controller layout?") + "\n\n" + qsTr("(Note: If you select no now and want to do this later, click the button or press R3 from the main menu.)"), false, () => root.showSteamShortcutDialog(true));
            else if(Chiaki.settings.remotePlayAsk)
            {
                if(!Chiaki.settings.psnRefreshToken || !Chiaki.settings.psnAuthToken || !Chiaki.settings.psnAuthTokenExpiry || !Chiaki.settings.psnAccountId)
                    root.showRemindDialog(qsTr("Remote Play via PSN"), qsTr("Would you like to connect to PSN?\nThis enables:\n- Automatic registration\n- Playing outside of your home network without port forwarding?") + "\n\n" + qsTr("(Note: If you select no now and want to do this later, go to the Config section of the settings.)"), true, () => root.showPSNTokenDialog(false));
                else
                    Chiaki.settings.remotePlayAsk = false;
            }
        }
    }
    Keys.onUpPressed: {
        if(hostsView.currentItem && hostsView.currentItem.visible)
        {
            hostsView.decrementCurrentIndex()
            while(!hostsView.currentItem.visible)
                hostsView.decrementCurrentIndex()
        }
    }
    Keys.onDownPressed: {
        if(hostsView.currentItem && hostsView.currentItem.visible)
        {
            hostsView.incrementCurrentIndex()
            while(!hostsView.currentItem.visible)
                 hostsView.incrementCurrentIndex()
        }
    }
    Keys.onMenuPressed: settingsButton.clicked()
    Keys.onReturnPressed: if (hostsView.currentItem) hostsView.currentItem.connectToHost()
    Keys.onYesPressed: if (hostsView.currentItem) hostsView.currentItem.wakeUpHost()
    Keys.onNoPressed: if (hostsView.currentItem) hostsView.currentItem.deleteHost()
    Keys.onEscapePressed: root.showConfirmDialog(qsTr("Quit"), qsTr("Are you sure you want to quit?"), () => Qt.quit())
    Keys.onPressed: (event) => {
        if (event.modifiers)
            return;
        switch (event.key) {
        case Qt.Key_PageUp:
            if (hostsView.currentItem) hostsView.currentItem.setConsolePin();
            event.accepted = true;
            break;
        case Qt.Key_PageDown:
            if (Chiaki.settings.psnAuthToken) Chiaki.refreshPsnToken();
            event.accepted = true;
            break;
        case Qt.Key_F1:
            if (typeof Chiaki.createSteamShortcut === "function") root.showSteamShortcutDialog(false);
            event.accepted = true;
            break;
        case Qt.Key_F2:
            root.showManualHostDialog();
            event.accepted = true;
            break;
        }
    }

    // ------------------------------------------------------------------ header
    Rectangle {
        id: header
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 84
        color: consolePane.headerColor

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: 1
            color: consolePane.borderColor
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 28
                rightMargin: 20
            }
            spacing: 14

            Image {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 46
                source: "qrc:/icons/chiaking.svg"
                sourceSize: Qt.size(92, 92)
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                spacing: 0
                Label {
                    text: "DA Remote Play"
                    font.pixelSize: 24
                    font.weight: Font.Bold
                }
                Label {
                    text: qsTr("PS5 Remote Play")
                    font.pixelSize: 14
                    color: consolePane.mutedColor
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Steam Shortcut")
                flat: true
                focusPolicy: Qt.NoFocus
                visible: typeof Chiaki.createSteamShortcut === "function"
                onClicked: root.showSteamShortcutDialog(false)
                Material.roundedScale: Material.SmallScale
            }

            Button {
                text: Chiaki.settings.psnAuthToken ? qsTr("Refresh PSN") : qsTr("Connect PSN")
                flat: true
                focusPolicy: Qt.NoFocus
                onClicked: {
                    if (Chiaki.settings.psnAuthToken)
                        Chiaki.refreshPsnToken();
                    else
                        root.showPSNTokenDialog(false);
                }
                Material.roundedScale: Material.SmallScale
            }

            Button {
                text: qsTr("+ Add Console")
                flat: true
                focusPolicy: Qt.NoFocus
                onClicked: root.showManualHostDialog()
                Material.roundedScale: Material.SmallScale
            }

            Button {
                id: settingsButton
                flat: true
                icon.source: "qrc:/icons/settings-20px.svg"
                icon.width: 30
                icon.height: 30
                focusPolicy: Qt.NoFocus
                onClicked: root.showSettingsDialog()
                Material.roundedScale: Material.SmallScale
            }
        }
    }

    // ------------------------------------------------------------------ title row
    Label {
        id: sectionTitle
        anchors {
            top: header.bottom
            left: parent.left
            topMargin: 26
            leftMargin: 32
        }
        text: qsTr("Your consoles")
        font.pixelSize: 21
        font.weight: Font.DemiBold
    }

    Label {
        anchors {
            verticalCenter: sectionTitle.verticalCenter
            right: parent.right
            rightMargin: 32
        }
        text: Chiaki.discoveryEnabled ? qsTr("Searching your network...") : qsTr("Network search is off")
        font.pixelSize: 14
        color: consolePane.mutedColor
    }

    // ------------------------------------------------------------------ console cards
    ListView {
        id: hostsView
        keyNavigationWraps: true
        anchors {
            top: sectionTitle.bottom
            left: parent.left
            right: parent.right
            bottom: quickBar.top
            topMargin: 16
            leftMargin: 32
            rightMargin: 32
            bottomMargin: 16
        }
        spacing: 14
        clip: true
        model: Chiaki.hosts
        ScrollBar.vertical: ScrollBar { }
        onCountChanged: {
            if(!hostsView.currentItem)
                hostsView.incrementCurrentIndex();
            if(!hostsView.currentItem)
                return;
            if(!hostsView.currentItem.visible)
            {
                for(var i = 0; i < hostsView.count; i++)
                {
                    hostsView.incrementCurrentIndex()
                    if(hostsView.currentItem.visible)
                    {
                        break;
                    }
                }
            }
        }
        delegate: ItemDelegate {
            id: delegate
            visible: modelData.display
            width: hostsView.width
            height: modelData.display ? 148 : 0
            highlighted: ListView.isCurrentItem
            onClicked: connectToHost()

            function connectToHost() {
                if(modelData.discovered)
                    Chiaki.connectToHost(index, modelData.name);
                else
                    Chiaki.connectToHost(index);
            }

            function wakeUpHost() {
                if(!modelData.discovered && !modelData.duid)
                    Chiaki.wakeUpHost(index);
            }

            function deleteHost() {
                if (modelData.manual)
                    root.showConfirmDialog(qsTr("Delete Console"), qsTr("Are you sure you want to delete this console?"), () => {Chiaki.deleteHost(index)});
                else if (modelData.discovered && !modelData.registered)
                    root.showConfirmDialog(qsTr("Hide Console"), qsTr("Are you sure you want to hide this console?") + "\n\n" + qsTr("Note: You can unhide from the Consoles section of the Settings under Hidden Consoles"), () => Chiaki.hideHost(modelData.mac, modelData.name));
            }

            function setConsolePin() {
                root.showConsolePinDialog(index);
            }

            function statusText() {
                if (modelData.duid)
                    return modelData.discovered ? qsTr("Ready to pair") : qsTr("Remote via PSN");
                if (modelData.state == "standby")
                    return qsTr("Rest mode");
                if (modelData.state == "ready")
                    return qsTr("Ready");
                return modelData.discovered ? modelData.state : qsTr("Offline / not found");
            }

            function statusColor() {
                if (modelData.state == "ready" || modelData.duid)
                    return consolePane.goodColor;
                if (modelData.state == "standby")
                    return consolePane.warnColor;
                return consolePane.mutedColor;
            }

            background: Rectangle {
                radius: 16
                color: delegate.highlighted || delegate.hovered ? consolePane.cardColorActive : consolePane.cardColor
                border.width: delegate.highlighted ? 2 : 1
                border.color: delegate.highlighted ? Material.accent : consolePane.borderColor
            }

            contentItem: RowLayout {
                spacing: 24

                Rectangle {
                    Layout.preferredWidth: 116
                    Layout.preferredHeight: 116
                    Layout.leftMargin: 8
                    radius: 12
                    color: "#0F121A"
                    Image {
                        anchors.fill: parent
                        anchors.margins: 12
                        fillMode: Image.PreserveAspectFit
                        source: "image://svg/console-ps" + (modelData.ps5 ? "5" : "4") + (modelData.state == "standby" ? "#light_standby" : "#light_on")
                        sourceSize: Qt.size(width, height)
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: modelData.name ? modelData.name : (modelData.ps5 ? "PlayStation 5" : "PlayStation 4")
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 8
                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 5
                            color: delegate.statusColor()
                        }
                        Label {
                            text: delegate.statusText()
                            font.pixelSize: 15
                            color: delegate.statusColor()
                        }
                        Label {
                            visible: !!modelData.app
                            text: "  |  " + qsTr("Playing: %1").arg(modelData.app)
                            font.pixelSize: 15
                            color: consolePane.mutedColor
                        }
                    }

                    Label {
                        text: {
                            let parts = [];
                            if (modelData.address)
                                parts.push(Chiaki.settings.streamerMode ? qsTr("IP hidden") : modelData.address);
                            parts.push(modelData.registered ? qsTr("Paired") : qsTr("Not paired"));
                            if (modelData.manual)
                                parts.push(qsTr("Added by IP"));
                            return parts.join("   |   ");
                        }
                        font.pixelSize: 14
                        color: consolePane.mutedColor
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: 12
                    spacing: 6

                    Button {
                        text: qsTr("Wake")
                        flat: true
                        focusPolicy: Qt.NoFocus
                        visible: modelData.registered && !modelData.duid && !modelData.discovered
                        onClicked: delegate.wakeUpHost()
                        Material.roundedScale: Material.SmallScale
                    }

                    Button {
                        text: qsTr("PIN")
                        flat: true
                        focusPolicy: Qt.NoFocus
                        visible: modelData.registered
                        onClicked: delegate.setConsolePin()
                        Material.roundedScale: Material.SmallScale
                    }

                    Button {
                        text: modelData.manual ? qsTr("Delete") : qsTr("Hide")
                        flat: true
                        focusPolicy: Qt.NoFocus
                        visible: modelData.manual || (modelData.discovered && !modelData.registered)
                        onClicked: delegate.deleteHost()
                        Material.roundedScale: Material.SmallScale
                    }

                    Button {
                        text: (modelData.registered || modelData.duid) ? qsTr("Play") : qsTr("Pair")
                        highlighted: true
                        focusPolicy: Qt.NoFocus
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        leftPadding: 30
                        rightPadding: 30
                        onClicked: delegate.connectToHost()
                        Material.roundedScale: Material.MediumScale
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------ empty state
    ColumnLayout {
        anchors.centerIn: hostsView
        visible: consolePane.visibleHostCount === 0
        spacing: 14

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 260
            Layout.preferredHeight: 155
            source: "qrc:/icons/chiaking-logo-white.svg"
            sourceSize: Qt.size(520, 310)
            fillMode: Image.PreserveAspectFit
            opacity: 0.18
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Looking for your PS5...")
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Turn on your PS5 and keep it on the same network as this PC.\nIf it doesn't show up, add it by its IP address.")
            font.pixelSize: 15
            color: consolePane.mutedColor
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Add console by IP")
            highlighted: true
            focusPolicy: Qt.NoFocus
            onClicked: root.showManualHostDialog()
            Material.roundedScale: Material.MediumScale
        }
    }

    // ------------------------------------------------------------------ quick settings bar
    Rectangle {
        id: quickBar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 76
        color: consolePane.headerColor

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: 1
            color: consolePane.borderColor
        }

        RowLayout {
            anchors {
                fill: parent
                leftMargin: 24
                rightMargin: 24
            }
            spacing: 18

            Switch {
                text: qsTr("Find consoles")
                focusPolicy: Qt.NoFocus
                checked: Chiaki.discoveryEnabled
                onToggled: Chiaki.discoveryEnabled = checked
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 36
                color: consolePane.borderColor
            }

            Label {
                text: qsTr("Stream")
                color: consolePane.mutedColor
                font.pixelSize: 15
            }

            ComboBox {
                Layout.preferredWidth: 190
                focusPolicy: Qt.NoFocus
                model: [qsTr("720p"), qsTr("1080p (max)")]
                currentIndex: Chiaki.settings.resolutionLocalPS5 == 3 ? 0 : 1
                onActivated: (index) => {
                    Chiaki.settings.resolutionLocalPS5 = index == 0 ? 3 : 4;
                }
            }

            Label {
                text: qsTr("Picture")
                color: consolePane.mutedColor
                font.pixelSize: 15
            }

            ComboBox {
                Layout.preferredWidth: 340
                focusPolicy: Qt.NoFocus
                model: [qsTr("Native 1080p"), qsTr("Upscaled to screen - Sharp"), qsTr("Upscaled to screen - Max detail")]
                currentIndex: Chiaki.settings.videoPreset == 4 ? 2 : (Chiaki.settings.videoPreset == 3 ? 1 : 0)
                onActivated: (index) => {
                    switch (index) {
                    case 0:
                        Chiaki.settings.videoPreset = 2;
                        Chiaki.window.videoPreset = ChiakiWindow.VideoPreset.HighQuality;
                        break;
                    case 1:
                        Chiaki.settings.videoPreset = 3;
                        Chiaki.window.videoPreset = ChiakiWindow.VideoPreset.HighQualitySpatial;
                        break;
                    case 2:
                        Chiaki.settings.videoPreset = 4;
                        Chiaki.window.videoPreset = ChiakiWindow.VideoPreset.HighQualityAdvancedSpatial;
                        break;
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "v" + Qt.application.version
                color: consolePane.mutedColor
                font.pixelSize: 14
            }
        }
    }
}
