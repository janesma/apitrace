// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

import QtQuick 2.5
import ApiTrace 1.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

ApplicationWindow {
    width: 600
    height: 500
    visible: true
    id: mainWindow

    Selection {
        id: selection
    }

    ColumnLayout {
        anchors.fill: parent

        BarGraph {
            id: barGraph
            width: mainWindow.width
            height: mainWindow.height - scrollBar.height
            selection: selection
            visible: true
            randomBarCount: 1000
            MouseArea {
                property var startx : -1.0;
                property var starty : -1.0;
                anchors.fill: parent
                onPressed : {
                    startx = mouse.x / barGraph.width;
                    starty = (barGraph.height - mouse.y) / barGraph.height;
                    barGraph.mouseDrag(startx, starty, startx, starty);
                }
                onPositionChanged : {
                    if (mouse.buttons & Qt.LeftButton) {
                        var endx = mouse.x / barGraph.width;
                        var endy = (barGraph.height - mouse.y) / barGraph.height;
                        barGraph.mouseDrag(startx, starty, endx, endy)
                    }
                }
                onWheel : {
                    var wheelx = 1.0;
                    wheelx = wheel.x / barGraph.width;
                    barGraph.mouseWheel(wheel.angleDelta.y / 2, wheelx);
                }
                onReleased : {
                    barGraph.mouseRelease();
                }
            }
            onZoomChanged: {
                scrollBar.positionHandle()
            }
            onTranslateChanged: {
                scrollBar.positionHandle()
            }
        }
        Item {
            id: scrollBar
            height: 20
            width: mainWindow.width
            anchors {
                left: mainWindow.left;
                margins: 1;
            }
            function zoomIn () {
                barGraph.mouseWheel(45, 0.5);
            }
            function zoomOut () {
                barGraph.mouseWheel(-45, 0.5);
            }
            function positionHandle() {
                handle.width = (backScrollbar.width - 2 * backScrollbar.height) / barGraph.zoom
                var maxXOffset = backScrollbar.width - 2 * backScrollbar.height - handle.width;
                var fullTranslation = 1.0 - barGraph.zoom;
                if (fullTranslation == 0) {
                    handle.x = backScrollbar.height;
                } else {
                    handle.x = backScrollbar.height + barGraph.translate * maxXOffset / fullTranslation;
                }
            }
            Rectangle {
                id: backScrollbar;
                radius: 2;
                color: Qt.rgba(0.5, 0.5, 0.5, 0.85);
                anchors { fill: parent; }
            }
            MouseArea {
                width: height;
                anchors {
                    top: parent.top;
                    left: parent.left;
                    bottom: parent.bottom;
                    margins: (backScrollbar.border.width +1);
                }
                onClicked: { scrollBar.zoomOut(); }

                Text {
                    text: "-";
                    anchors.centerIn: parent;
                }
            }
            MouseArea {
                width: height;
                anchors {
                    top: parent.top;
                    right: parent.right;
                    bottom: parent.bottom;
                    margins: (backScrollbar.border.width +1);
                }
                onClicked: { scrollBar.zoomIn(); }

                Text {
                    text: "+";
                    anchors.centerIn: parent;
                }
            }

            Item {
                id: handle;
                width: (backScrollbar.width - 2 * backScrollbar.height) / barGraph.zoom
                x: backScrollbar.height
                onXChanged: {
                    if (barGraph.zoom == 1.0) {
                        barGraph.translate = 0.0;
                    } else {
                        var fullTranslation = 1.0 - barGraph.zoom;
                        var xOffset = handle.x - backScrollbar.height;
                        var maxXOffset = backScrollbar.width - 2 * backScrollbar.height - handle.width;
                        barGraph.translate = fullTranslation * xOffset / maxXOffset;
                    }
                }

                anchors {
                    top: parent.top;
                    bottom: parent.bottom;
                }
                Rectangle {
                    id: backHandle;
                    color: Qt.rgba(0.2, 0.2, 0.2, 1.0)
                    anchors { fill: parent; }
                }
                MouseArea {
                    id: clicker;
                    drag {
                        target: handle;
                        minimumX: backScrollbar.height;
                        maximumX: (backScrollbar.width - backScrollbar.height - handle.width);
                        axis: Drag.XAxis;
                    }
                    anchors { fill: parent; }
                }
            }
        }
    }
}
