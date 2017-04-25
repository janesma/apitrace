import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

// This control is highly similar to ApiControl.  Consider merging
// them and making the filter functionality optional.  For now, it
// should remain separate, as the feature is a proof-of-concept that
// may soon be deleted.
Item {
    property QBatchModel batchModel
    
    SplitView {
        anchors.fill: parent

        ScrollView {
            Layout.preferredWidth: 100
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            ListView {
                id: batch_selection
                model: batchModel.renders
                focus: true
                highlight: Rectangle { color: "lightsteelblue"; radius: 5; }
                delegate: Component {
                    Item {
                        height: render_text.height
                        width: batch_selection.width
                        Text {
                            id: render_text
                            text: modelData
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                batchModel.setIndex(index);
                                batch_selection.currentIndex = index;
                            }
                        }
                    }
                }
                Keys.onDownPressed: {
                    if (batch_selection.currentIndex + 1 < batch_selection.count) {
                        batch_selection.currentIndex += 1;
                        batchModel.setIndex(batch_selection.currentIndex);
                    }
                }
                Keys.onUpPressed: {
                    if (batch_selection.currentIndex > 0) {
                        batch_selection.currentIndex -= 1;
                        batchModel.setIndex(batch_selection.currentIndex);
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
                contentWidth: batch.width; contentHeight: batch.height
                clip: true
                TextEdit {
                    id: batch
                    readOnly: true
                    selectByMouse: true
                    text: batchModel.batch
                }
            }
        }
    }
}
