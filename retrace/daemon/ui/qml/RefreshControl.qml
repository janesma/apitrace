import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property FrameRetrace metricsModel
    RowLayout {
        anchors.fill: parent
        Item {
            anchors.left: parent.left
            anchors.top: parent.top
            width: 400
            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                id: filterText
                text: "Metrics Filter:"
            }
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                height: filterText.height * 1.5
                width: 300
                border.width: 1
                TextInput {
                    anchors.fill: parent
                    id: metricFilter
                    text: ""
                    onDisplayTextChanged: {
                        metricsModel.filterMetrics(displayText)
                    }
                }
            }
        }
        Button {
            id: refreshButton
            anchors.right: parent.right
            text: "Refresh"
            onClicked: {
                metricsModel.refreshMetrics();
            }
        }
    }
}
