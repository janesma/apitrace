import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import ApiTrace 1.0

Item {
    property QUniformsModel uniformModel
    function columnsForDimension(dim) {
        switch(dim) {
        case QUniformValue.K1x1:
            return 1;
        case QUniformValue.K2x1:
        case QUniformValue.K2x2:
        case QUniformValue.K2x3:
        case QUniformValue.K2x4:
            return 2;
        case QUniformValue.K3x1:
        case QUniformValue.K3x2:
        case QUniformValue.K3x3:
        case QUniformValue.K3x4:
            return 3;
        case QUniformValue.K4x1:
        case QUniformValue.K4x2:
        case QUniformValue.K4x3:
        case QUniformValue.K4x4:
            return 4;
        }
    }
    SplitView {
        anchors.fill: parent
        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                id: render_selection
                model: uniformModel.renders
                focus: true
                highlight: Rectangle { color: "lightsteelblue"; radius: 5; }
                delegate: Component {
                    Item {
                        height: render_text.height
                        width: render_selection.width
                        Text {
                            id: render_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                uniformModel.setIndex(index);
                                render_selection.currentIndex = index;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (render_selection.currentIndex + 1 < render_selection.count) {
                        render_selection.currentIndex += 1;
                        uniformModel.setIndex(render_selection.currentIndex);
                    }
                }
                Keys.onUpPressed: {
                    if (render_selection.currentIndex > 0) {
                        render_selection.currentIndex -= 1;
                        uniformModel.setIndex(render_selection.currentIndex);
                    }
                }
            }
        }
        ScrollView {
            id: uniformScroll
            Layout.preferredWidth: 1000
            Layout.preferredHeight: parent.height
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                model: uniformModel.uniforms
                anchors.fill: parent
                delegate: Component {
                    Column {
                        Text {
                            id: nameText
                            text: modelData.name
                        }
                        Grid {
                            id: uniformGrid
                            columns: columnsForDimension(modelData.dimension)
                            Repeater {
                                model: modelData.data
                                delegate: Component {
                                    Rectangle {
                                        width: uniformScroll.width / 4
                                        height: uniformText.height
                                        border.width: 1
                                        TextInput {
                                            id: uniformText
                                            anchors.centerIn: parent
                                            selectByMouse: true
                                            text: modelData
                                            font.family: "Monospace"
                                            validator: DoubleValidator{}
                                            Keys.onReturnPressed: {
                                                if (!acceptableInput) {
                                                    text = modelData;
                                                    return;
                                                }
                                                uniformModel.setUniform(nameText.text,
                                                                        index,
                                                                        text);
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
    }
}
