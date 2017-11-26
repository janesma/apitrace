import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import ApiTrace 1.0

Item {
    property QStateModel stateModel

    Item {
        id: searchItem
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 10
        height: textRect.height
        
        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 5
            id: searchText
            text: "Search:"
        }
        Rectangle {
            anchors.top: parent.top
            anchors.left: searchText.right
            anchors.leftMargin: 20
            height: searchText.height * 1.5
            border.width: 1
            width: searchItem.width/2
            id: textRect
            TextInput {
                height: searchText.height
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 4
                id: stateSearch
                text: ""
                onDisplayTextChanged: {
                    stateModel.search(displayText)
                }
            }
        }
    }

    ScrollView {
        anchors.top: searchItem.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        ListView {
            id: stateList
            model: stateModel.state
            anchors.fill: parent
            delegate: Component {
                id: currentDelegate
                Row {
                    visible: modelData.visible
                    height: modelData.visible ? choices.height : 0
                    spacing: 10
                    Rectangle {
                        id: indent
                        width: nameText.height * modelData.indent
                        height: 1
                        opacity: 0.0
                    }
                    Image {
                        id: collapse
                        anchors.verticalCenter: nameText.verticalCenter
                        width: nameText.height * 0.75
                        height: nameText.height * 0.75
                        source: "qrc:///qml/images/if_next_right_82215.png"
                        visible: (modelData.value.length == 0)
                        property var collapsed: false
                        transform: Rotation {
                            origin.x: collapse.width / 2
                            origin.y: collapse.width / 2
                            angle: collapse.collapsed ? 0 : 90
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (collapse.collapsed) {
                                    stateModel.expand(modelData.path);
                                    collapse.collapsed = false
                                } else {
                                    stateModel.collapse(modelData.path);
                                    collapse.collapsed = true
                                }
                            }
                        }
                    }
                    Text {
                        id: nameText
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.name + " : "
                    }
                    Row {
                        id: dataRow
                        spacing: 10
                        property var rowModel: currentDelegate.modelData
                        Repeater {
                            model: modelData.value.length;
                            property var indices: modelData.indices
                            property var value: modelData.value
                            property var path: modelData.path
                            property var name: modelData.name
                            property var choices: modelData.choices
                            Row {
                                property var offset: index
                                spacing: 10
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: indices.length > 0
                                    text: visible ? indices[offset] : ""
                                }
                                TextInput {
                                    anchors.margins: 3
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: choices.length == 0
                                    validator: DoubleValidator{}
                                    text: value[offset]
                                    Keys.onReturnPressed: {
                                        if (!acceptableInput) {
                                            text = modelData.value[0];
                                            return;
                                        }
                                        stateModel.setState(path,
                                                            name,
                                                            offset,
                                                            text);
                                    }
                                }
                                ComboBoxFitContents {
                                    anchors.verticalCenter: parent.verticalCenter
                                    model: choices
                                    currentIndex: parseInt(value[offset])
                                    visible: choices.length > 0
                                    onActivated: {
                                        stateModel.setState(path,
                                                            name,
                                                            offset,
                                                            choices[currentIndex]);
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
    }
}
