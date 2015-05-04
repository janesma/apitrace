import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

ApplicationWindow {
    width: 600
    height: 500
    visible: true
    id: mainWindow

    FrameRetrace {
        id : frameRetrace
        Component.onCompleted: {
            setFrame("/home/majanes/src/apitrace/retrace/daemon/test/simple.trace", 7);
            //setFrame("/home/majanes/.steam/steam/steamapps/common/dota/dota_linux.2.trace", 1800)
        }
    }

    RowLayout {
        anchors.fill: parent
        ListView {
            id: renderList
            Layout.preferredWidth: 50
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true
            // anchors.fill: parent
            model: frameRetrace.renders
            delegate: Rectangle {
                id: renderDel
                height : renderNum.height * 1.5
                width : renderList.width
                Text {
                    id: renderNum
                    text: model.index
                }
                MouseArea {
                    anchors.fill: parent
                    onPressed : {
                        frameRetrace.retrace(index)
                    }
                }
            }
        }
        TabView {
            Layout.preferredWidth: 600
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
                }
            }
        }
    }
}
