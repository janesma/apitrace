import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0


Item {
    property QTextureModel textureModel

    SplitView {
        anchors.fill: parent

        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
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
                                textureModel.setIndex(index);
                                texture_selection.currentIndex = index;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (texture_selection.currentIndex + 1 < texture_selection.count) {
                        texture_selection.currentIndex += 1;
                        textureModel.setIndex(texture_selection.currentIndex);
                    }
                }
                Keys.onUpPressed: {
                    if (texture_selection.currentIndex > 0) {
                        texture_selection.currentIndex -= 1;
                        textureModel.setIndex(texture_selection.currentIndex);
                    }
                }
            }
        }
        SplitView {
            Layout.preferredWidth: 1000
            ScrollView {
                Layout.preferredWidth: 200
                Layout.preferredHeight: parent.height
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                ListView {
                    model: 
                    Rectangle {
                    anchors.fill: parent
                    color: red
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
    }
}
