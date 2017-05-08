import QtQuick 2.2
import QtQuick.Controls 1.4
import ApiTrace 1.0

Item {
    property var rtModel
    property int rtIndex

    function boundedIndex(i) {
        if (i >= rtModel.renderTargetImages.length) {
            return rtModel.renderTargetImages.length - 1;
        }
        return i;
    }
            
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
                spacing: 5
                Component {
                    id: imageDelegate
                    Rectangle {
                        width: thumbnailImage.width + 6
                        height: thumbnailImage.height + 6
                        border.width: 3
                        border.color: index == boundedIndex(rtIndex) ? "yellow" : "transparent"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                rtIndex = index;
                            }
                        }
                        Image {
                            anchors.centerIn: parent
                            id: thumbnailImage
                            width: thumbnails.width - 6
                            fillMode: Image.PreserveAspectFit
                            source: modelData
                            cache: false
                        }
                    }
                }
                model: rtModel.renderTargetImages
                delegate: imageDelegate
            }
            Image {
                id: rtDisplayImage
                fillMode: Image.PreserveAspectFit
                source: rtModel.renderTargetImages[boundedIndex(rtIndex)]
                cache: false
            }
        }
    }
}
