import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QRenderShadersList renderModel
    property Button compileButton
    property RowLayout compileRow
    SplitView {
        anchors.fill: parent
        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                id: shader_selection
                model: renderModel.renders
                highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
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
