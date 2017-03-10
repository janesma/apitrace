import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property FrameRetrace metricsModel
    Item {
        id: metricsItem
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.fill: parent
        anchors.topMargin: 10
        width: 400

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 5
            id: filterText
            text: "Metrics Filter:"
        }
        Rectangle {
            anchors.top: parent.top
            anchors.left: filterText.right
            anchors.leftMargin: 20
            height: filterText.height * 1.5
            border.width: 1
            width: metricsItem.width/2
            id: textRect
            TextInput {
                height: filterText.height
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 4
                id: metricFilter
                text: ""
                onDisplayTextChanged: {
                    metricsModel.filterMetrics(displayText)
                }
            }
        }
        Button {
            id: refreshButton
            anchors.left: textRect.right
            anchors.leftMargin: 25
            text: "Refresh"
            onClicked: {
                metricsModel.refreshMetrics();
            }
        }
    }
}
