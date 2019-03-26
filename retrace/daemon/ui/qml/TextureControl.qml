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
        text: "4000"
    }
    Text {
        id: bindingWidth
        visible: false
        text: "GL_TEXTURE_4 GL_TEXTURE_CUBE_MAP_POSITIVE_Y offset 1"
    }
    Text {
        id: detailWidth
        visible: false
        text: "Format: GL_COMPRESSED_RGB_S3TC_DXT1_EXT"
    }
    Text {
        id: levelWidth
        visible: false
        text: "Level:10"
    }
    
    SplitView {
        anchors.fill: parent

        ScrollView {
            width: renderWidth.width * 2
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
            width: bindingWidth.width * 1.3
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
                onModelChanged: {
                    binding_selection.currentIndex = index;
                    textureModel.selectBinding(modelData);
                }
            }
        }
        Column {
            width: detailWidth.width
            height: parent.height
            ComboBox {
                id: levelSelect
                model: textureModel.texture.levels
                width: parent.width
                onCurrentIndexChanged: {
                    textureModel.texture.selectLevel(currentIndex);
                }
            }
            ListView {
                id: details
                model: textureModel.texture.details
                // anchors.top: levelSelect.bottom
                width: parent.width
                height: parent.height - levelSelect.height
                delegate: Component {
                    Item {
                        height: details_text.height
                        Text {
                            id: details_text
                            text: modelData
                        }
                    }
                }
            }
        }
        Image {
            fillMode: Image.PreserveAspectFit
            source: textureModel.texture.image
            smooth: false
        }
    }
}
