import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

import ApiTrace 1.0

Item {
    property QExperimentModel experimentModel
    Column {
        anchors.centerIn: parent
        spacing: 5
        CheckBox {
            text: "Disable Draw"
            checkedState: experimentModel.selectionDisabled
            partiallyCheckedEnabled : false
            onClicked: {
                if (checkedState == Qt.PartiallyChecked)
                    checkedState = Qt.Unchecked;
                experimentModel.disableDraw(checkedState);
                partiallyCheckedEnabled = false
                // restore the binding, which is overwritten when the
                // ui is clicked (not sure why they would do it that
                // way)
                // http://stackoverflow.com/questions/23860270/binding-checkbox-checked-property-with-a-c-object-q-property
                checkedState = Qt.binding(function () { 
                    return experimentModel.selectionDisabled;
                });
            }
        }
        CheckBox {
            text: "Simple Shader"
            checkedState: experimentModel.selectionSimpleShader
            partiallyCheckedEnabled : false
            onClicked: {
                if (checkedState == Qt.PartiallyChecked)
                    checkedState = Qt.Unchecked;
                experimentModel.simpleShader(checkedState);
                partiallyCheckedEnabled = false
                checkedState = Qt.binding(function () { 
                    return experimentModel.selectionSimpleShader;
                });
            }
        }
    }
}
