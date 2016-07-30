import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property var model
    property var compile_button
    property var shader_type
    id: shader_tab
    TabView {
        anchors.fill: parent
        Tab {
            title: "Source"
            ScrollView {
                Flickable {
                    contentWidth: model.source.width; contentHeight: model.source.height
                    clip: true
                    TextEdit {
                        id: source
                        text: model.source
                        onTextChanged: {
                            if (shader_type == "vs") {
                                compile_button.vsText = text;
                            } else if (shader_type == "fs") {
                                compile_button.fsText = text;
                            } else if (shader_type == "tess_eval") {
                                compile_button.fsText = text;
                            } else if (shader_type == "tess_control") {
                                compile_button.fsText = text;
                            } 
                            compile_button.visible=true;
                        }
                    }
                }
            }
        }
        Tab {
            title: "IR"
            ScrollView {
                Flickable {
                    anchors.fill: parent
                    contentWidth: model.ir.width; contentHeight: model.ir.height
                    clip: true
                    Text {
                        id: ir
                        text: model.ir
                    }
                }
            }
        }
        Tab {
            title: "SSA"
            ScrollView {
                Flickable {
                    anchors.fill: parent
                    contentWidth: model.nirSsa.width; contentHeight: model.nirSsa.height
                    clip: true
                    Text {
                        id: ssa
                        text: model.nirSsa
                    }
                }
            }
        }
        Tab {
            title: "NIR"
            ScrollView {
                Flickable {
                    anchors.fill: parent
                    contentWidth: model.nirFinal.width; contentHeight: model.nirFinal.height
                    clip: true
                    Text {
                        id: vsNIR
                        text: model.nirFinal
                    }
                }
            }
        }
        Tab {
            title: "Simd8"
            ScrollView {
                Flickable {
                    anchors.fill: parent
                    contentWidth: model.simd8.width; contentHeight: model.simd8.height
                    clip: true
                    Text {
                        id: vec4
                        text: model.simd8
                    }
                }
            }
        }
        Tab {
            title: "Simd16"
            ScrollView {
                Flickable {
                    anchors.fill: parent
                    contentWidth: model.simd16.width; contentHeight: model.simd16.height
                    clip: true
                    Text {
                        id: simd16
                        text: model.simd16
                    }
                }
            }
        }
    }
}
