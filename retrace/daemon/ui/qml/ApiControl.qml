import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QApiModel apiModel
    Item {
        id: apiItem
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 10
        width: 400
        height: textRect.height
        
        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 5
            id: filterText
            text: "Api Filter:"
        }
        Rectangle {
            anchors.top: parent.top
            anchors.left: filterText.right
            anchors.leftMargin: 20
            height: filterText.height * 1.5
            border.width: 1
            width: apiItem.width/2
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
                    apiModel.filter(displayText)
                }
            }
        }
    }
    
    SplitView {
        anchors.top: apiItem.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                id: api_selection
                model: apiModel.renders
                focus: true
                highlight: Rectangle { color: "lightsteelblue"; radius: 5; }
                delegate: Component {
                    Item {
                        height: render_text.height
                        width: api_selection.width
                        Text {
                            id: render_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                apiModel.setIndex(index);
                                api_selection.currentIndex = index;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (api_selection.currentIndex + 1 < api_selection.count) {
                        api_selection.currentIndex += 1;
                        apiModel.setIndex(api_selection.currentIndex);
                    }
                }
                Keys.onUpPressed: {
                    if (api_selection.currentIndex > 0) {
                        api_selection.currentIndex -= 1;
                        apiModel.setIndex(api_selection.currentIndex);
                    }
                }
            }
        }
        ScrollView {
            Layout.preferredWidth: 1000
            Layout.preferredHeight: parent.height
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Flickable {
                anchors.fill: parent
                contentWidth: api.width; contentHeight: api.height
                clip: true
                TextEdit {
                    id: api
                    readOnly: true
                    selectByMouse: true
                    text: apiModel.apiCalls
                }
            }
        }
    }
}
