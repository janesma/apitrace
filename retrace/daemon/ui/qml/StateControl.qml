import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import ApiTrace 1.0

Item {
    property QStateModel stateModel

    ScrollView {
        anchors.fill: parent
        ListView {
            id: stateList
            model: stateModel.state
            anchors.fill: parent
            delegate: Component {
                Row {
                    visible: modelData.visible
                    height: modelData.visible ? combo.height : 0
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
                        visible: (modelData.valueType == QStateValue.KglDirectory)
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
                    TextInput {
                        anchors.margins: 3
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (modelData.valueType == QStateValue.KglFloat)
                        validator: DoubleValidator{}
                        text: (modelData.valueType == QStateValue.KglFloat) ? modelData.value : ""
                        Keys.onReturnPressed: {
                            if (!acceptableInput) {
                                text = modelData.value;
                                return;
                            }
                            stateModel.setState(modelData.group,
                                                modelData.path,
                                                modelData.name,
                                                0,
                                                text);
                        }
                    }
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10
                        visible: (modelData.valueType == QStateValue.KglColor)
                        Text{
                            text: "Red: "
                        }
                        TextInput {
                            anchors.margins: 3
                            validator: DoubleValidator{}
                            text: (modelData.valueType == QStateValue.KglColor) ? modelData.value[0] : ""
                            Keys.onReturnPressed: {
                                if (!acceptableInput) {
                                    text = modelData.value[0];
                                    return;
                                }
                                stateModel.setState(modelData.group,
                                                    modelData.path,
                                                    modelData.name,
                                                    0,
                                                    text);
                            }
                        }
                        Text{
                            text: "Blue: "
                        }
                        TextInput {
                            anchors.margins: 3
                            validator: DoubleValidator{}
                            text: (modelData.valueType == QStateValue.KglColor) ? modelData.value[1] : ""
                            Keys.onReturnPressed: {
                                if (!acceptableInput) {
                                    text = modelData.value[1];
                                    return;
                                }
                                stateModel.setState(modelData.group,
                                                    modelData.path,
                                                    modelData.name,
                                                    1,
                                                    text);
                            }
                        }
                        Text{
                            text: "Green: "
                        }
                        TextInput {
                            anchors.margins: 3
                            validator: DoubleValidator{}
                            text: (modelData.valueType == QStateValue.KglColor) ? modelData.value[2] : ""
                            Keys.onReturnPressed: {
                                if (!acceptableInput) {
                                    text = modelData.value[2];
                                    return;
                                }
                                stateModel.setState(modelData.group,
                                                    modelData.path,
                                                    modelData.name,
                                                    2,
                                                    text);
                            }
                        }
                        Text{
                            text: "Alpha: "
                        }
                        TextInput {
                            validator: DoubleValidator{}
                            anchors.margins: 3
                            text: (modelData.valueType == QStateValue.KglColor) ? modelData.value[3] : ""
                            Keys.onReturnPressed: {
                                if (!acceptableInput) {
                                    text = modelData.value[3];
                                    return;
                                }
                                stateModel.setState(modelData.group,
                                                    modelData.path,
                                                    modelData.name,
                                                    3,
                                                    text);
                            }
                        }
                    }
                    ComboBoxFitContents {
                        id: combo
                        anchors.verticalCenter: parent.verticalCenter
                        model: modelData.choices
                        currentIndex: (modelData.valueType == QStateValue.KglEnum ? modelData.value : 0)
                        visible: (modelData.valueType == QStateValue.KglEnum)
                        onActivated: {
                            stateModel.setState(modelData.group,
                                                modelData.path,
                                                modelData.name,
                                                0,
                                                modelData.choices[currentIndex]);
                        }
                    }
                }
            }
        }
    }
}
