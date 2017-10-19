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
                    Text {
                        id: nameText
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.name + " : "
                    }
                    ComboBoxFitContents {
                        anchors.verticalCenter: parent.verticalCenter
                        model: modelData.choices
                        currentIndex: modelData.value
                        onActivated: {
                            stateModel.setState(modelData.name,
                                                modelData.choices[currentIndex]);
                        }
                    }
                }
            }
        }
    }
}
