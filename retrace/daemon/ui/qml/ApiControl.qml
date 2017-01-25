import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QApiModel apiModel
    SplitView {
        anchors.fill: parent

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
