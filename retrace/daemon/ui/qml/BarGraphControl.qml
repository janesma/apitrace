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
    function formatFloat(num) {
        if (num < 10) {
            return num.toPrecision(2);
        }
        return num.toFixed(0);
    }
    
    ColumnLayout {
        anchors.fill: parent
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
                width: (parent.width / 2) - vertLabel.width
                id: vertMetric
                model: metricNames()
                onCurrentIndexChanged : {
                    var currentId = metricId(model[currentIndex]);
                    if (currentId)
                        metric_model.setMetric(0, currentId);
                }
                Component.onCompleted : {
                    metric_model.onQMetricList.connect(selectGpuDuration)
                }
                function selectGpuDuration() {
                    for (var i = 0; i < metric_model.metricList.length; ++i) {
                        if (metric_model.metricList[i].name == "GPU Time Elapsed") {
                            metric_model.setMetric(0, metric_model.metricList[i].id);
                            currentIndex = i;
                            return;
                        }
                    }
                }
            }
            Text {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: vertMetric.right
                id: horizLabel
                text: "Horizontal Metric: "
            }
            ComboBox {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: horizLabel.right
                width: (parent.width / 2) - horizLabel.width
                model: metricNames()
                onCurrentIndexChanged : {
                    var currentId = metricId(model[currentIndex]);
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

            BarGraph {
                id: barGraph
                anchors.top: parent.top
                anchors.bottom: parent.bottom
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
