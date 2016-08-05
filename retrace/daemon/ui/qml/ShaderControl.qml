import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property var model
    property var compile_button
    property var compile_row
    property var shader_type
    id: shader_tab

    Component {
        id: sourceTab
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
                            compile_button.tessEvalText = text;
                        } else if (shader_type == "tess_control") {
                            compile_button.tessControlText = text;
                        } else if (shader_type == "geom") {
                            compile_button.geomText = text;
                        } 
                        compile_button.visible=true;
                        compile_row.visible=true
                    }
                }
            }
        }
    }

    Component {
        id: irTab
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
    
    Component {
        id: ssaTab
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

    Component {
        id: nirTab
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

    Component {
        id: simd8Tab
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
    
    Component {
        id: simd16Tab
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

    Connections {
        target: model
        onShadersChanged: {
            // remove all tabs from the view, and re-display them only
            // if content is available.
            while (view.count > 0) {
                view.removeTab(0);
            }
            if (model.source != "") {
                var t = view.addTab("Source", sourceTab);
                t.active = true;
                if (shader_type == "vs") {
                    compile_button.vsText = model.source;
                } else if (shader_type == "fs") {
                    compile_button.fsText = model.source;
                } else if (shader_type == "tess_eval") {
                    compile_button.tessEvalText = model.source;
                } else if (shader_type == "tess_control") {
                    compile_button.tessControlText = model.source;
                } else if (shader_type == "geom") {
                    compile_button.geomText = model.source;
                } 
                compile_button.visible=false;
                compile_row.visible=false
                
            }
            if (model.ir != "") {
                view.addTab("IR", irTab);
            }
            if (model.nirSsa != "") {
                view.addTab("SSA", ssaTab);
            }
            if (model.nirFinal != "") {
                view.addTab("NIR", nirTab);
            }
            if (model.simd8 != "") {
                view.addTab("Simd8", simd8Tab);
            }
            if (model.simd16 != "") {
                view.addTab("Simd16", simd16Tab);
            }
        }
    }
    
    TabView {
        anchors.fill: parent
        id: view
    }
}
