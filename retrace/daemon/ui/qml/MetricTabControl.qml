import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QMetricsModel metricsModel
    TableView {
        anchors.fill: parent
        model: metricsModel.metrics
        TableViewColumn {
            role: "name"
            title: "Metric"
            width: 350
        }
        TableViewColumn {
            role: "value"
            title: "Selection"
            width: 100
        }
        TableViewColumn {
            role: "frameValue"
            title: "Frame"
            width: 100
        }
    }
}
