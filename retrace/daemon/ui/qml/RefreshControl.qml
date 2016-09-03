import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property FrameRetrace metricsModel
    Button {
        anchors.right: parent.right
        text: "Refresh"
        onClicked: {
            metricsModel.refreshMetrics();
        }
    }
}
