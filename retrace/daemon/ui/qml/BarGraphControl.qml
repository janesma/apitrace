import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property var selection
    property var metric_model
    // hold the currently selected metric names for the graph
    property var vert_metric
    property var horiz_metric

    // hold the currently filtered list of selectable metrics
    property var vert_metrics
    property var horiz_metrics
    id: control

    function vertMetricNames() {
        var output = new Array()
        // always put the currently selected metric first in the list.
        // A new model will automatically selected index 0, and we
        // don't want the bargraph to reselect crazily as the user
        // types a filter string.
        output.push(vert_metric);
        for (var i = 0; i < metric_model.metricList.length; ++i) {
            output.push(metric_model.metricList[i].name);
        }
        vert_metrics = output;
    }

    function horizMetricNames() {
        var output = new Array()
        // always put the currently selected metric first in the list.
        // A new model will automatically selected index 0, and we
        // don't want the bargraph to reselect crazily as the user
        // types a filter string.
        output.push(horiz_metric);
        for (var i = 0; i < metric_model.metricList.length; ++i) {
            output.push(metric_model.metricList[i].name);
        }
        horiz_metrics = output;
    }

    function metricId(name) {
        for (var i = 0; i < metric_model.metricList.length; ++i) {
            if (metric_model.metricList[i].name == name)
                return metric_model.metricList[i].id;
        }
        return 0;
    }
    function formatFloat(num) {
        if (num < 10) {
            return num.toPrecision(2);
        }
        return num.toFixed(0);
    }

    Component.onCompleted: {
        vert_metrics = ["No metric"];
        vert_metric = "No metric";
        horiz_metrics = ["No metric"];
        horiz_metric = "No metric";
        metric_model.onQMetricList.connect(vertMetricNames)
        metric_model.onQMetricList.connect(horizMetricNames)
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20
        Item {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 40
            Layout.fillWidth: true
            Layout.fillHeight: true
            id: comboBoxes
            Text {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                id: vertLabel
                text: "Vertical Metric: "
            }
            ComboBox {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: vertLabel.right
                anchors.leftMargin: 5
                width: (parent.width / 2) - vertLabel.width - 15
                id: vertMetric
                model: vert_metrics
                onCurrentIndexChanged : {
                    vert_metric = model[currentIndex]
                    var currentId = metricId(vert_metric);
                    if (currentId)
                        metric_model.setMetric(0, currentId);
                }
            }
            Text {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: vertMetric.right
                anchors.leftMargin: 10
                id: horizLabel
                text: "Horizontal Metric: "
            }
            ComboBox {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: horizLabel.right
                anchors.leftMargin: 5
                width: (parent.width / 2) - horizLabel.width - 5
                model: horiz_metrics
                onCurrentIndexChanged : {
                    horiz_metric = model[currentIndex]
                    var currentId = metricId(horiz_metric);
                    if (currentId)
                        metric_model.setMetric(1, currentId);
                }
            }
        }
        Item {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 400
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.top: comboBoxes.bottom
            anchors.topMargin: 10

            BarGraph {
                id: barGraph
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.left: parent.left
                anchors.right: scale.left
                model: metric_model
                selection: control.selection
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
                    onWheel : {
                        var wheelx = 1.0;
                        wheelx = wheel.x / barGraph.width;
                        barGraph.mouseWheel(wheel.angleDelta.y / 5, wheelx);
                    }
                    onReleased : {
                        barGraph.mouseRelease();
                    }
                }
            }
            Rectangle {
                id: scale
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.right: parent.right
                width: axisText.width
                Text {
                    id: axisText
                    lineHeightMode: Text.FixedHeight
                    lineHeight: scale.height / 5.0
                    text: formatFloat(metric_model.maxMetric) + "\n"
                        + formatFloat(metric_model.maxMetric * 0.8) + "\n"
                        + formatFloat(metric_model.maxMetric * 0.6) + "\n"
                        + formatFloat(metric_model.maxMetric * 0.4) + "\n"
                        + formatFloat(metric_model.maxMetric * 0.2)
                }
            }
        }
    }
}
