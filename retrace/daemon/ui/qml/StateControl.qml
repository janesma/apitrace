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
                Column {
                    Text {
                        id: nameText
                        text: modelData.name
                    }
                    Row {
                        id: stateGrid
                        anchors.left: parent.left
                        anchors.right: parent.right
                        Repeater {
                            model: modelData.values
                            Rectangle {
                                width: stateText.width
                                height: stateText.height
                                anchors.margins:10
                                border.width: 1
                                Text {
                                    id: stateText
                                    anchors.centerIn: parent
                                    text: modelData
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
