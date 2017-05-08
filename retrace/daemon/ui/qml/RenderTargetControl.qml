import QtQuick 2.2
import QtQuick.Controls 1.4
import ApiTrace 1.0

Item {
    property var rtModel
    property int rtIndex
    Row {
        anchors.fill: parent
        Column {
            id: renderOptions
            CheckBox {
                text: "Clear before render"
                onCheckedChanged: {
                    rtModel.clearBeforeRender = checked;
                }
            }
            CheckBox {
                text: "Stop at render"
                onCheckedChanged: {
                    rtModel.stopAtRender = checked;
                }
            }
            CheckBox {
                text: "Highlight selected render"
                onCheckedChanged: {
                    rtModel.highlightRender = checked;
                }
            }
        }
        SplitView {
            orientation: Qt.Horizontal
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width - renderOptions.width
            ListView {
                id: thumbnails
                width: 100
                Component {
                    id: imageDelegate
                    Image {
                        id: thumbnailImage
                        width: thumbnails.width
                        fillMode: Image.PreserveAspectFit
                        source: modelData
                        cache: false
                    }
                }
                model: rtModel.renderTargetImages
                delegate: imageDelegate
            }
            Image {
                id: rtDisplayImage
                fillMode: Image.PreserveAspectFit
                source: rtModel.renderTargetImages[0]
                cache: false
            }
        }
    }
}
