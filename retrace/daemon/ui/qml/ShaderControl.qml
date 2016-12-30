import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property var model
    property Button compile_button
    property RowLayout compile_row
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
                    cursorVisible: true
                    persistentSelection: true
                    selectByMouse: true
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
                        } else if (shader_type == "comp") {
                            compile_button.compText = text;
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
                TextEdit {
                    id: ir
                    readOnly: true
                    selectByMouse: true
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
                TextEdit {
                    id: nirSsa
                    readOnly: true
                    selectByMouse: true
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
                TextEdit {
                    id: nirFinal
                    readOnly: true
                    selectByMouse: true
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
                TextEdit {
                    id: simd8
                    readOnly: true
                    selectByMouse: true
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
                TextEdit {
                    id: simd16
                    readOnly: true
                    selectByMouse: true
                    text: model.simd16
                }
            }
        }
    }

    Component {
        id: simd32Tab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: simd32.width; contentHeight: simd32.height
                clip: true
                TextEdit {
                    id: simd32
                    readOnly: true
                    selectByMouse: true
                    text: model.simd32
                }
            }
        }
    }

    Component {
        id: beforeUnificationTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: beforeUnification.width; contentHeight: beforeUnification.height
                clip: true
                TextEdit {
                    id: beforeUnification
                    readOnly: true
                    selectByMouse: true
                    text: model.beforeUnification
                }
            }
        }
    }

    Component {
        id: afterUnificationTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: afterUnification.width; contentHeight: afterUnification.height
                clip: true
                TextEdit {
                    id: afterUnification
                    readOnly: true
                    selectByMouse: true
                    text: model.afterUnification
                }
            }
        }
    }

    Component {
        id: beforeOptimizationTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: beforeOptimization.width; contentHeight: beforeOptimization.height
                clip: true
                TextEdit {
                    id: beforeOptimization
                    readOnly: true
                    selectByMouse: true
                    text: model.beforeOptimization
                }
            }
        }
    }

    Component {
        id: constCoalescingTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: constCoalescing.width; contentHeight: constCoalescing.height
                clip: true
                TextEdit {
                    id: constCoalescing
                    readOnly: true
                    selectByMouse: true
                    text: model.constCoalescing
                }
            }
        }
    }

    Component {
        id: genIrLoweringTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: genIrLowering.width; contentHeight: genIrLowering.height
                clip: true
                TextEdit {
                    id: genIrLowering
                    readOnly: true
                    selectByMouse: true
                    text: model.genIrLowering
                }
            }
        }
    }

    Component {
        id: layoutTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: layout.width; contentHeight: layout.height
                clip: true
                TextEdit {
                    id: layout
                    readOnly: true
                    selectByMouse: true
                    text: model.layout
                }
            }
        }
    }

    Component {
        id: optimizedTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: optimized.width; contentHeight: optimized.height
                clip: true
                TextEdit {
                    id: optimized
                    readOnly: true
                    selectByMouse: true
                    text: model.optimized
                }
            }
        }
    }

    Component {
        id: pushAnalysisTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: pushAnalysis.width; contentHeight: pushAnalysis.height
                clip: true
                TextEdit {
                    id: pushAnalysis
                    readOnly: true
                    selectByMouse: true
                    text: model.pushAnalysis
                }
            }
        }
    }

    Component {
        id: codeHoistingTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: codeHoisting.width; contentHeight: codeHoisting.height
                clip: true
                TextEdit {
                    id: codeHoisting
                    readOnly: true
                    selectByMouse: true
                    text: model.codeHoisting
                }
            }
        }
    }

    Component {
        id: codeSinkingTab
        ScrollView {
            Flickable {
                anchors.fill: parent
                contentWidth: codeSinking.width; contentHeight: codeSinking.height
                clip: true
                TextEdit {
                    id: codeSinking
                    readOnly: true
                    selectByMouse: true
                    text: model.codeSinking
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
                } else if (shader_type == "comp") {
                    compile_button.compText = model.source;
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
            if (model.simd32 != "") {
                view.addTab("Simd32", simd32Tab);
            }
            if (model.beforeUnification != "") {
                view.addTab("beforeUnification", beforeUnificationTab);
            }
            if (model.afterUnification != "") {
                view.addTab("afterUnification", afterUnificationTab);
            }
            if (model.beforeOptimization != "") {
                view.addTab("beforeOptimization", beforeOptimizationTab);
            }
            if (model.constCoalescing != "") {
                view.addTab("constCoalescing", constCoalescingTab);
            }
            if (model.genIrLowering != "") {
                view.addTab("genIrLowering", genIrLoweringTab);
            }
            if (model.layout != "") {
                view.addTab("layout", layoutTab);
            }
            if (model.optimized != "") {
                view.addTab("optimized", optimizedTab);
            }
            if (model.pushAnalysis != "") {
                view.addTab("pushAnalysis", pushAnalysisTab);
            }
            if (model.codeHoisting != "") {
                view.addTab("codeHoisting", codeHoistingTab);
            }
            if (model.codeSinking != "") {
                view.addTab("codeSinking", codeSinkingTab);
            }
        }
    }
    
    TabView {
        anchors.fill: parent
        id: view
    }
}
