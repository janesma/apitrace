import QtQuick 2.2
import QtQuick.Controls 1.1
import ApiTrace 1.0

Item {
    property var model
    Row {
        anchors.fill: parent
        Column {
            id: renderOptions
            CheckBox {
                text: "Clear before render"
                onCheckedChanged: {
                    model.clearBeforeRender = checked;
                }
            }
            CheckBox {
                text: "Stop at render"
                onCheckedChanged: {
                    model.stopAtRender = checked;
                }
            }
            CheckBox {
                text: "Highlight selected render"
                onCheckedChanged: {
                    model.highlightRender = checked;
                }
            }
        }
        Item {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width - renderOptions.width
            Image {
                id: rtDisplayImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: model.renderTargetImage
                cache: false
            }
        }
    }
}
