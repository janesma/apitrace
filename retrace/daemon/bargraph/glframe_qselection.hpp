/**************************************************************************
 *
 * Copyright 2015 Intel Corporation
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#ifndef RETRACE_DAEMON_BARGRAPH_GLFRAME_QSELECTION_H_
#define RETRACE_DAEMON_BARGRAPH_GLFRAME_QSELECTION_H_

#include <QObject>
#include <QList>

#include "glframe_traits.hpp"

namespace glretrace {

class QSelection : public QObject,
                   NoCopy, NoAssign, NoMove {
  Q_OBJECT
 public:
  QSelection();
  virtual ~QSelection();
 public slots:
  void select(QList<int> selection);
 signals:
  void onSelect(QList<int> selection);
 private:
  QList<int> _selection;
};

}  // namespace glretrace

#endif  // RETRACE_DAEMON_BARGRAPH_GLFRAME_QSELECTION_H_
