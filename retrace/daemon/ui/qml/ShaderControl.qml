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
                    contentWidth: source.width; contentHeight: source.height
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
                    contentWidth: ir.width; contentHeight: ir.height
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
                    contentWidth: nirSsa.width; contentHeight: nirSsa.height
                    clip: true
                    Text {
                        id: nirSsa
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
                    contentWidth: nirFinal.width; contentHeight: nirFinal.height
                    clip: true
                    Text {
                        id: nirFinal
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
                    contentWidth: simd8.width; contentHeight: simd8.height
                    clip: true
                    Text {
                        id: simd8
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
                    contentWidth: simd16.width; contentHeight: simd16.height
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
