import QtQuick 2.7
import QtQuick.Controls 2.2

// based on
// https://stackoverflow.com/questions/45029968/how-do-i-set-the-combobox-width-to-fit-the-largest-item

ComboBox {
    id: control

    property int modelWidth
    width: modelWidth + 2*leftPadding + 2*rightPadding

    delegate: ItemDelegate {
        width:control.width
        text: modelData
        highlighted: control.highlightedIndex === index
        hoverEnabled: control.hoverEnabled
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        font.family: control.font.family
        font.pointSize: control.font.pointSize
    }

    TextMetrics {
        id: textMetrics
    }

    onModelChanged: {
        textMetrics.font = control.font
        for(var i = 0; i < model.length; i++){
            textMetrics.text = model[i]
            modelWidth = Math.max(textMetrics.width, modelWidth)
        }
    }
}
