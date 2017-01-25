import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

Item {
    property QApiModel apiModel
    ScrollView {
        anchors.fill: parent
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
