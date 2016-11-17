import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QMetricsModel metricsModel
    ColumnLayout {
        anchors.fill: parent
        TableView {
            id: table
            selectionMode: SelectionMode.ExtendedSelection
            Layout.fillWidth: true
            Layout.fillHeight: true
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
            TableViewColumn {
                role: "description"
                title: "Description"
                width: 600
            }
            Keys.onPressed: {
                if ((event.key == Qt.Key_C) && (event.modifiers & Qt.ControlModifier)) {
                    event.accepted = true;
                    table.selection.forEach( function(rowIndex) {
                        metricsModel.copySelect(rowIndex);
                    });
                    metricsModel.copy();
                }
            }
        }
    }
}
