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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.1
import ApiTrace 1.0

ApplicationWindow {
    width: 600
    height: 500
    visible: true
    id: mainWindow

    Selection {
        id: selection
    }

    BarGraph {
        id: barGraph
        selection: selection
        visible: true
        anchors.fill: parent
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
                barGraph.mouseWheel(wheel.angleDelta.y / 5, wheelx);
            }
            onReleased : {
                barGraph.mouseRelease();
            }
        }
    }
}
