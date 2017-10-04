import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import ApiTrace 1.0

Item {
    property QStateModel stateModel
    function getWidth(choices) {
        console.warn("getWidth called: " + choices.length.toString());
        var len = 0;
        for (var i = 0; i < choices.length; i++) {
            console.warn("getWidth: " + choices[i]);
            var choice_len = choices[i].length;
            if (choice_len > len) {
                len = choice_len;
            }
        }
        console.warn("getWidth returned: " + len.toString());
        return len;
    }

    function getLongest(choices) {
        console.warn("getLongest called: " + choices.length.toString());
        var len = 0;
        var longest = 0;
        for (var i = 0; i < choices.length; i++) {
            console.warn("getLongest: " + choices[i]);
            var choice_len = choices[i].length;
            if (choice_len > len) {
                len = choice_len;
                longest = i;
            }
        }
        console.warn("getLongest returned: " + choices[longest]);
        return choices[longest];
    }

    ScrollView {
        anchors.fill: parent
        ListView {
            id: stateList
            model: stateModel.state
            anchors.fill: parent
            delegate: Component {
                Column {
                    Text {
                        id: nameText
                        text: modelData.name
                    }
                    Row {
                        id: stateGrid
                        anchors.left: parent.left
                        anchors.right: parent.right
                        Repeater {
                            model: modelData.values
                            property var choices: modelData.choices
                            property var name: modelData.name
                            ComboBoxFitContents {
                                model: choices
                                property int stateIndex : index
                                currentIndex: modelData
                                onActivated: {
                                    stateModel.setState(name,
                                                        stateIndex,
                                                        choices[currentIndex]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
