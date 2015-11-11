import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0
import Qt.labs.settings 1.0

ApplicationWindow {
    width: 600
    height: 500
    visible: true
    id: mainWindow

    Selection {
        id: selection
    }

    FrameRetrace {
        id : frameRetrace
        selection: selection
        onOpenPercentChanged: {
            if (openPercent < 100) {
                progressBar.percentComplete = openPercent;
                return;
            }
            progressBar.visible = false;
            mainUI.visible = true;
        }
    }

    Item {
        id: openfile
        anchors.fill:parent
        Component.onCompleted: visible = true
        Text {
            id: enterText
            width: 500
            anchors.centerIn: parent
            text: "trace file:"
        }
        Text {
            id: textHeight
            visible:false
            text: "ForHeight"
        }
        Rectangle {
            width: 500
            id: textBox
            border.width: 1
            height: textHeight.height * 1.5
            anchors.top: enterText.bottom
            anchors.left: enterText.left
            TextInput {
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                height: textHeight.height
                width:500
                selectByMouse: true
                activeFocusOnPress : true
                focus: true
                id: textInput
                text: ""

                Settings {
                    property alias file_name: textInput.text
                }
            }
        }
        Rectangle {
            id: file_rect
            border.width: 1
            width: textBox.height
            height: textBox.height
            anchors.left: textBox.right
            anchors.bottom: textBox.bottom
            Text {
                anchors.centerIn: parent
                text: "?"
            }
            MouseArea {
                anchors.fill: parent
                onPressed : {
                    fileDialog.visible =  true
                }
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
                var path = fileDialog.fileUrl.toString();
                path = path.replace(/^(file:\/{2})/,"");
                textInput.text = path
                fileDialog.visible = false
            }
        }
        Text {
            id: frameText
            anchors.top: file_rect.bottom
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
            }
        }
        Button {
            id: okButton
            anchors.left: frameBox.left
            anchors.top: frameBox.bottom
            text: "OK"
            onClicked: {
                openfile.visible = false
                progressBar.visible = true
                frameRetrace.setFrame(textInput.text, frameInput.text);
            }
        }
        Button {
            text: "Cancel"
            anchors.left: okButton.right
            anchors.top: frameBox.bottom
            onClicked: {
                Qt.quit();
            }
        }
    }
    Item {
        id: progressBar
        visible: false
        anchors.fill: parent
        property int percentComplete
        onPercentCompleteChanged: {
            blueBar.width = percentComplete / 100 * progressBackground.width
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
    }
    ColumnLayout {
        id: mainUI
        anchors.fill: parent
        visible: false
        Component {
            id: highlightBar
            Rectangle {
                clip: false
                width: renderList.width; height: 20 //heightCalculation.height * 1.5
                color: "light blue"
                opacity: .4
                y: renderList.currentItem.y;
                onYChanged: {
                    visible = true
                }
            }
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
                title: "Vertex Shader"
                TabView {
                    Tab {
                        title: "Source"
                        Flickable {
                            contentWidth: vsSource.width; contentHeight: vsSource.height
                            clip: true
                            TextEdit {
                                id: vsSource
                                text: frameRetrace.vsSource
                            }
                        }
                    }
                    Tab {
                        title: "IR"
                        Flickable {
                            anchors.fill: parent
                            contentWidth: vsIR.width; contentHeight: vsIR.height
                            clip: true
                            Text {
                                id: vsIR
                                text: frameRetrace.vsIR
                            }
                        }
                    }
                    Tab {
                        title: "Vec4"
                        Flickable {
                            anchors.fill: parent
                            contentWidth: vsVec4.width; contentHeight: vsVec4.height
                            clip: true
                            Text {
                                id: vsVec4
                                text: frameRetrace.vsVec4
                            }
                        }
                    }
                }
            }
            Tab {
                title: "Fragment Shader"
                TabView {
                    anchors.fill: parent
                    Tab {
                        title: "Source"
                        Flickable {
                            contentWidth: fsSource.width; contentHeight: fsSource.height
                            clip: true
                            TextEdit {
                                id: fsSource
                                text: frameRetrace.fsSource
                            }
                        }
                    }
                    Tab {
                        title: "IR"
                        Flickable {
                            contentWidth: fsIR.width; contentHeight: fsIR.height
                            clip: true
                            Text {
                                id: fsIR
                                text: frameRetrace.fsIR
                            }
                        }
                    }
                    Tab {
                        title: "Simd8"
                        Flickable {
                            contentWidth: fsSimd8.width; contentHeight: fsSimd8.height
                            clip: true
                            Text {
                                id: fsSimd8
                                text: frameRetrace.fsSimd8
                            }
                        }
                    }
                    Tab {
                        title: "Simd16"
                        Flickable {
                            contentWidth: fsSimd16.width; contentHeight: fsSimd16.height
                            clip: true
                            Text {
                                id: fsSimd16
                                text: frameRetrace.fsSimd16
                            }
                        }
                    }
                    Tab {
                        title: "SSA"
                        Flickable {
                            contentWidth: fsSSA.width; contentHeight: fsSSA.height
                            clip: true
                            Text {
                                id: fsSSA
                                text: frameRetrace.fsSSA
                            }
                        }
                    }
                    Tab {
                        title: "NIR"
                        Flickable {
                            contentWidth: fsNIR.width; contentHeight: fsNIR.height
                            clip: true
                            Text {
                                id: fsNIR
                                text: frameRetrace.fsNIR
                            }
                        }
                    }
                }
            }
            Tab {
                title: "RenderTarget"
                Row {
                    Column {
                        id: renderOptions
                        CheckBox {
                            text: "Clear before render"
                            onCheckedChanged: {
                                frameRetrace.clearBeforeRender = checked;
                            }
                        }
                        CheckBox {
                            text: "Stop at render"
                        }
                    }
                    Item {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width - renderOptions.width
                        Image {
                            id: rtDisplayImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: frameRetrace.renderTargetImage
                            cache: false
                        }
                    }
                }
            }
        }
    }
}
