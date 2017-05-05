import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0
import Qt.labs.settings 1.0

ApplicationWindow {
    width: 1000
    height: 800
    visible: true
    id: mainWindow

    Selection {
        id: selection
    }

    FrameRetrace {
        id : frameRetrace
        selection: selection
        argvZero: Qt.application.arguments[0]
        onFrameCountChanged: {
            if (frameCount < progressBar.targetFrame) {
                progressBar.frameCount = frameCount;
                return;
            }
            progressBar.visible = false;
            mainUI.visible = true;
        }
        onGeneralErrorChanged: {
            fileError.text = frameRetrace.generalError;
            fileError.detailedText = frameRetrace.generalErrorDetails;
            fileError.visible = true;
        }
    }

    MessageDialog {
        id: fileError
        title: "FrameRetrace Error"
        icon: StandardIcon.Warning
        visible: false
        onAccepted: {
            visible = false;
            if (frameRetrace.errorSeverity == FrameRetrace.Fatal) {
                Qt.quit();
            }
        }
    }

    Item {
        id: openfile
        anchors.fill:parent
        Component.onCompleted: visible = true
        Rectangle {
            id: imageBox
            anchors.top: parent.top
            anchors.topMargin: 100
            anchors.horizontalCenter: parent.horizontalCenter
            width: 800
            height: 200
            border.width: 1
            visible: true
            Image {
                height: 200
                width: 800
                id: appIcon
                source: "qrc:///qml/images/retracer_icon.png"
                visible: true
            }
        }
        Text {
            id: enterText
            width: 800
            anchors.centerIn: parent
            text: "trace file:"
        }
        Text {
            id: textHeight
            visible:false
            text: "ForHeight"
        }
        Rectangle {
            width: 800
            id: textBox
            border.width: 1
            height: textHeight.height * 1.5
            anchors.top: enterText.bottom
            anchors.left: enterText.left
            Flickable {
                id: flickText
                anchors.fill: parent
                contentWidth: textInput.width
                contentHeight: textInput.height
                flickableDirection: Flickable.HorizontalFlick
                clip: true
                TextInput {
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: Text.AlignVCenter
                    height: textBox.height
                    width:500
                    selectByMouse: true
                    activeFocusOnPress : true
                    focus: true
                    id: textInput
                    text: ""

                    Settings {
                        property alias file_name: textInput.text
                    }
                    KeyNavigation.tab: file_rect
                    KeyNavigation.backtab: cancelButton
                    Keys.onReturnPressed: {
                        okButton.clicked()
                    }
                }
            }
        }
        Rectangle {
            id: file_rect
            border.width: 1
            width: textBox.height
            height: textBox.height
            color: activeFocus ? "lightgrey" : "white"
            anchors.left: textBox.right
            anchors.bottom: textBox.bottom
            Text {
                anchors.centerIn: parent
                text: "?"
            }
            MouseArea {
                anchors.fill: parent
                onPressed : {
                    fileDialog.visible = true;
                }
            }
            KeyNavigation.tab: frameInput
            KeyNavigation.backtab: textInput
            Keys.onReturnPressed: {
                fileDialog.visible = true;
            }
        }
        FileDialog {
            id: fileDialog
            visible: false
            title: "Please choose a trace file"
            selectExisting: true
            selectMultiple: false
            nameFilters: [ "trace files (*.trace)", "All files (*)" ]
            onAccepted: {
                var path = fileDialog.fileUrl
                textInput.text = frameRetrace.urlToFilePath(path)
                fileDialog.visible = false
            }
        }
        Text {
            id: frameText
            anchors.top: file_rect.bottom
            anchors.topMargin: 10
            anchors.left: textBox.left
            text: "frame number:"
        }
        Rectangle {
            width: 100
            id: frameBox
            border.width: 1
            height: textHeight.height * 1.5
            anchors.top: frameText.bottom
            anchors.left: frameText.left
            TextInput {
                height: textHeight.height
                anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                anchors.leftMargin: 4
                verticalAlignment: Text.AlignVCenter
                activeFocusOnPress : true
                width: parent.width
                selectByMouse: true
                focus: true
                id: frameInput
                text: ""
                Settings {
                    property alias frame_number: frameInput.text
                }
                KeyNavigation.tab: hostInput
                KeyNavigation.backtab: file_rect
                Keys.onReturnPressed: {
                    okButton.clicked()
                }
            }
        }
        Text {
            id: hostText
            anchors.top: frameBox.bottom
            anchors.topMargin: 10
            anchors.left: frameBox.left
            text: "host:"
        }
        Rectangle {
            width: 500
            id: hostBox
            border.width: 1
            height: textHeight.height * 1.5
            anchors.top: hostText.bottom
            anchors.left: hostText.left
            TextInput {
                height: textHeight.height
                anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                anchors.leftMargin: 4
                verticalAlignment: Text.AlignVCenter
                activeFocusOnPress : true
                width: parent.width
                selectByMouse: true
                focus: true
                id: hostInput
                text: "localhost"
                Settings {
                    property alias host_name: hostInput.text
                }
                KeyNavigation.tab: okButton
                KeyNavigation.backtab: frameInput
                Keys.onReturnPressed: {
                    okButton.clicked()
                }
            }
        }
        Button {
            id: okButton
            anchors.left: hostBox.left
            anchors.top: hostBox.bottom
            anchors.topMargin: 10
            text: "OK"
            onClicked: {
                if (frameRetrace.setFrame(textInput.text, frameInput.text, hostInput.text)) {
                    openfile.visible = false;
                    progressBar.visible = true;
                    progressBar.targetFrame = parseInt(frameInput.text, 10);
                } else {
                    fileError.text = "File not found:\n\t" + textInput.text;
                    fileError.visible = true;
                }
            }
            KeyNavigation.tab: cancelButton
            KeyNavigation.backtab: hostInput
            Keys.onReturnPressed: {
                okButton.clicked();
            }
        }
        Button {
            text: "Cancel"
            id: cancelButton
            anchors.left: okButton.right
            anchors.top: hostBox.bottom
            anchors.topMargin: 10
            anchors.leftMargin: 10
            onClicked: {
                Qt.quit();
            }
            KeyNavigation.tab: textInput
            KeyNavigation.backtab: okButton
            Keys.onReturnPressed: {
                cancelButton.clicked();
            }
        }
    }
    Item {
        id: progressBar
        visible: false
        anchors.fill: parent
        property int frameCount
        property int targetFrame
        onFrameCountChanged: {
            blueBar.width = frameCount / targetFrame * progressBackground.width
        }
        Rectangle {
            id: progressBackground
            anchors.centerIn: parent
            color: "black"
            width: parent.width * 0.8
            height: 20
        }
        Rectangle {
            id: blueBar
            anchors.left: progressBackground.left
            anchors.top: progressBackground.top
            color: "light blue"
            width: 0
            height: 20
            z:1
        }
        Text {
            anchors.left: progressBackground.left
            anchors.top: progressBackground.bottom
            text: "Playing frame: " + progressBar.frameCount.toString()
        }
    }
    ColumnLayout {
        id: mainUI
        anchors.fill: parent
        visible: false

        RefreshControl {
            Layout.minimumHeight: 20
            Layout.maximumHeight: 20
            metricsModel: frameRetrace
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        BarGraphControl {
            selection: selection
            metric_model: frameRetrace
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        TabView {
            Layout.preferredWidth: 400
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
            Tab {
                title: "Shaders"
                anchors.fill: parent
                RenderShadersControl {
                    anchors.fill: parent
                    renderModel: frameRetrace.shaders
                }
            }
            Tab {
                title: "RenderTarget"
                RenderTargetControl {
                    model: frameRetrace.rendertarget
                }
            }
            Tab {
                title: "Api Calls"
                clip: true
                anchors.fill: parent
                ApiControl {
                    anchors.fill: parent
                    apiModel: frameRetrace.api
                }
            }
            Tab {
                title: "Batch"
                clip: true
                anchors.fill: parent
                BatchControl {
                    anchors.fill: parent
                    batchModel: frameRetrace.batch
                }
            }
            Tab {
                title: "Metrics"
                id: metricTab
                anchors.fill: parent
                clip: true
                MetricTabControl {
                    metricsModel: frameRetrace.metricTab
                }
            }
            Tab {
                title: "Experiments"
                id: experimentTab
                anchors.fill: parent
                clip: true
                ExperimentControl {
                    experimentModel: frameRetrace.experimentModel
                }
            }
        }
    }
}
