import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0


Item {
    property QTextureModel textureModel

    // sample text for initial widths
    Text {
        id: renderWidth
        visible: false
        text: "40000"
    }
    Text {
        id: bindingWidth
        visible: false
        text: "GL_TEXTURE_4 GL_TEXTURE_CUBE_MAP_POSITIVE_Y offset 1"
    }
    
    
    SplitView {
        anchors.fill: parent

        ScrollView {
            width: renderWidth.width
            ListView {
                id: texture_selection
                model: textureModel.renders
                focus: true
                highlight: Rectangle { color: "lightsteelblue"; radius: 5; }
                delegate: Component {
                    Item {
                        height: render_text.height
                        width: texture_selection.width
                        Text {
                            id: render_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                textureModel.selectRender(index);
                                texture_selection.currentIndex = index;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (texture_selection.currentIndex + 1 < texture_selection.count) {
                        texture_selection.currentIndex += 1;
                        textureModel.selectRender(texture_selection.currentIndex);
                    }
                }
                Keys.onUpPressed: {
                    if (texture_selection.currentIndex > 0) {
                        texture_selection.currentIndex -= 1;
                        textureModel.selectRender(texture_selection.currentIndex);
                    }
                }
            }
        }
        ScrollView {
            width: bindingWidth.width
            ListView {
                id: binding_selection
                focus: true
                model: textureModel.bindings
                highlight: Rectangle { color: "lightsteelblue"; radius: 5; }
                delegate: Component {
                    Item {
                        height: binding_text.height
                        width: binding_text.width
                        Text {
                            id: binding_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                textureModel.selectBinding(modelData);
                                binding_selection.currentIndex = index;
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height:parent.height
            color: "lightsteelblue"
            radius: 5
        }
            // Flickable {
            //     anchors.fill: parent
            //     contentWidth: api.width; contentHeight: api.height
            //     clip: true
            //     TextEdit {
            //         id: api
            //         readOnly: true
            //         selectByMouse: true
            //         text: apiModel.apiCalls
            //     }
            // }
    }
}
