import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QRenderShaders renderModel
    property Button compileButton
    property RowLayout compileRow
    TabView {
        anchors.fill: parent
        Tab {
            title: "Vertex"
            active: true
            anchors.fill: parent
            ShaderControl {
                shader_type: "vs"
                model: renderModel.vsShader
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
                model: renderModel.fsShader
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
                        model: renderModel.tessControlShader
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
                        model: renderModel.tessEvalShader
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
                model: renderModel.geomShader
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
                model: renderModel.compShader
                compile_button: compileButton
                compile_row: compileRow
            }
        }
    }
}
