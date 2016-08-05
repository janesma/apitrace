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

        BarGraphControl {
            selection: selection
            metric_model: frameRetrace
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            id: compileRow
            visible: false
            Button {
                id: compileButton
                visible: false
                text: "Compile"
                property var vsText
                property var fsText
                property var tessControlText
                property var tessEvalText
                property var geomText
                onClicked: {
                    visible = false
                    compileRow.visible = false;
                    frameRetrace.overrideShaders(vsText, fsText,
                                                 tessControlText,
                                                 tessEvalText,
                                                 geomText);
                }
                Component.onCompleted: { visible = false; }
            }
            Text {
                id: compileError
                text: frameRetrace.shaderCompileError
                visible: false
                onTextChanged: {
                    if (text == "") {
                        visible = false;
                        compileRow.visible = false;
                    } else {
                        visible = true;
                        compileRow.visible = true;
                    }
                }
            }
            Component.onCompleted: { visible = false; }
        }

        TabView {
            Layout.preferredWidth: 400
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
            Tab {
                title: "Shaders"
                TabView {
                    Tab {
                        title: "Vertex"
                        active: true
                        anchors.fill: parent
                        ShaderControl {
                            shader_type: "vs"
                            model: frameRetrace.vsShader
                            compile_button: compileButton
                            compile_row: compileRow
                        }
                    }
                    Tab {
                        title: "Fragment"
                        active: true
                        anchors.fill: parent
                        ShaderControl {
                            shader_type: "fs"
                            model: frameRetrace.fsShader
                            compile_button: compileButton
                            compile_row: compileRow
                        }
                    }
                    Tab {
                        title: "Tesselation"
                        active: true
                        anchors.fill: parent
                        TabView {
                            anchors.fill: parent
                            Tab {
                                title: "Control"
                                active: true
                                anchors.fill: parent
                                ShaderControl {
                                    shader_type: "tess_control"
                                    model: frameRetrace.tessControlShader
                                    compile_button: compileButton
                                    compile_row: compileRow
                                }
                            }
                            Tab {
                                title: "Evaluation"
                                active: true
                                anchors.fill: parent
                                ShaderControl {
                                    shader_type: "tess_eval"
                                    model: frameRetrace.tessEvalShader
                                    compile_button: compileButton
                                    compile_row: compileRow
                                }
                            }
                        }
                    }
                    Tab {
                        title: "Geometry"
                        active: true
                        anchors.fill: parent
                        ShaderControl {
                            shader_type: "geom"
                            model: frameRetrace.geomShader
                            compile_button: compileButton
                            compile_row: compileRow
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
                            onCheckedChanged: {
                                frameRetrace.stopAtRender = checked;
                            }
                        }
                        CheckBox {
                            text: "Highlight selected render"
                            onCheckedChanged: {
                                frameRetrace.highlightRender = checked;
                            }
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
            Tab {
                title: "Api Calls"
                id: apiTab
                clip: true
                ScrollView {
                    ListView {
                        model: frameRetrace.apiCalls
                        anchors.fill: apiTab
                        delegate: Text {
                            text: modelData
                        }
                    }
                }
            }
        }
    }
}
