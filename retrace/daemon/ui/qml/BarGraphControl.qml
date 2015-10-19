import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property var selection
    property var metric_model
    id: control

    function metricNames() {
        var output = new Array()
        for (var i = 0; i < metric_model.metricList.length; ++i) {
            output.push(metric_model.metricList[i].name);
        }
        return output;
    }
    function metricId(name) {
        for (var i = 0; i < metric_model.metricList.length; ++i) {
            if (metric_model.metricList[i].name == name)
                return metric_model.metricList[i].id;
        }
        return 0;
    }
    
    ColumnLayout {
        anchors.fill: parent
        ComboBox {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 40
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: metricNames()
            onCurrentIndexChanged : {
                var currentId = metricId(model[currentIndex]);
                if (currentId)
                    metric_model.setMetric(0, currentId);
            }
        }
        Item {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 400
            Layout.fillWidth: true
            Layout.fillHeight: true

            BarGraph {
                id: barGraph
                selection: control.selection
                visible: true
                anchors.fill: parent
            }
            MouseArea {
                property var startx : -1.0;
                property var starty : -1.0;
                anchors.fill: parent
                onPressed : {
                    startx = mouse.x / barGraph.width;
                    starty = (barGraph.height - mouse.y) / barGraph.height;
                    barGraph.mouseDrag(startx, starty, startx, starty);
                }
                onPositionChanged : {
                    if (mouse.buttons & Qt.LeftButton) {
                        var endx = mouse.x / barGraph.width;
                        var endy = (barGraph.height - mouse.y) / barGraph.height;
                        barGraph.mouseDrag(startx, starty, endx, endy)
                    }
                }
                onReleased : {
                    barGraph.mouseRelease();
                }
            }
        }
   }
}
