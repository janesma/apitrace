import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QRenderShadersList renderModel

    MessageDialog {
        id: compileError
        title: "Shader Compilation Error"
        icon: StandardIcon.Warning
        text: frameRetrace.shaderCompileError
        onTextChanged: {
            if (Boolean(text))
                visible = true;
        }
    }
    
    RowLayout {
        id: compileRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: false
        Button {
            id: compileButton
            visible: false
            text: "Compile"
            property var vsText: ""
            property var fsText: ""
            property var tessControlText: ""
            property var tessEvalText: ""
            property var geomText: ""
            property var compText: ""
            onClicked: {
                visible = false
                compileRow.visible = false;
                renderModel.overrideShaders(shader_selection.currentIndex,
                                            vsText, fsText,
                                            tessControlText,
                                            tessEvalText,
                                            geomText,
                                            compText);
            }
            Component.onCompleted: { visible = false; }
        }
    }
    SplitView {
        anchors.top: compileRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                id: shader_selection
                model: renderModel.renders
                highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
                focus: true
                delegate: Component {
                    Item {
                        height: render_text.height
                        width: render_text.width
                        Text {
                            id: render_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                renderModel.setIndex(index);
                                shader_selection.currentIndex = index;
                                compileButton.visible=false;
                                compileRow.visible=false
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (shader_selection.currentIndex + 1 < shader_selection.count) {
                        shader_selection.currentIndex += 1;
                        renderModel.setIndex(shader_selection.currentIndex);
                        compileButton.visible=false;
                        compileRow.visible=false
                    }
                }
                Keys.onUpPressed: {
                    if (shader_selection.currentIndex > 0) {
                        shader_selection.currentIndex -= 1;
                        renderModel.setIndex(shader_selection.currentIndex);
                        compileButton.visible=false;
                        compileRow.visible=false
                    }
                }
            }
        }
        TabView {
            Layout.preferredWidth: 1000
            Layout.preferredHeight: parent.height
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Tab {
                title: "Vertex"
                active: true
                anchors.fill: parent
                ShaderControl {
                    shader_type: "vs"
                    model: renderModel.shaders.vsShader
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
                    model: renderModel.shaders.fsShader
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
                            model: renderModel.shaders.tessControlShader
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
                            model: renderModel.shaders.tessEvalShader
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
                    model: renderModel.shaders.geomShader
                    compile_button: compileButton
                    compile_row: compileRow
                }
            }
            Tab {
                title: "Compute"
                active: true
                anchors.fill: parent
                ShaderControl {
                    shader_type: "comp"
                    model: renderModel.shaders.compShader
                    compile_button: compileButton
                    compile_row: compileRow
                }
            }
        }
    }
}
