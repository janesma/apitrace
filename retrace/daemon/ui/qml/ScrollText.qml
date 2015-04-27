import QtQuick 2.0

Item {
    id: scroller
    property var content
    Flickable {
        contentWidth: text_content.width; contentHeight: text_content.height
        clip: true
        TextEdit {
            id: text_content
            text: scroller.content
        }
    }
    Component.onCompleted: {
        print ("ScrollText: " + scroller.content)
    }
}
