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
                    Rectangle {
                        id: indent
                        width: nameText.height * modelData.indent
                        height: 1
                        opacity: 0.0
                    }
                    Rectangle {
                        id: collapse
                        anchors.bottom: nameText.bottom
                        width: nameText.height
                        height: nameText.height
                        visible: (modelData.valueType == QStateValue.KglDirectory)
                        property var collapsed: false
                        color: collapse.collapsed ? "red" : "green"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (collapse.collapsed) {
                                    collapse.color = "green";
                                    stateModel.expand(modelData.path);
                                    collapse.collapsed = false
                                } else {
                                    collapse.color = "red";
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
