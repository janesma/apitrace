/**************************************************************************
 *
 * Copyright 2017 Intel Corporation
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
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#ifndef _GLFRAME_EXPERIMENT_MODEL_HPP_
#define _GLFRAME_EXPERIMENT_MODEL_HPP_

#include <QObject>
#include <QString>

#include <mutex>
#include <string>
#include <map>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

// This class is highly similar to QApiModel.  Consider merging them
// and making the filter functionality optional.  For now, it should
// remain separate, as the feature is a proof-of-concept that may soon
// be deleted.
class FrameRetraceModel;
class QExperimentModel : public QObject,
                         NoCopy, NoAssign, NoMove{
  Q_OBJECT
  Q_PROPERTY(Qt::CheckState selectionDisabled READ selectionDisabled
             NOTIFY onDisabled)
 public:
  QExperimentModel();
  explicit QExperimentModel(IFrameRetrace *retrace);
  ~QExperimentModel() {}
  Qt::CheckState selectionDisabled() const { return m_checkbox; }
  void onSelect(SelectionId selection_count, QList<int> selection);
  Q_INVOKABLE void disableDraw(Qt::CheckState disable);
 signals:
  void onDisabled();
  void onExperiment();

 private:
  IFrameRetrace *m_retrace;
  std::map<RenderId, bool> m_disabled;
  QList<int> m_selection;
  SelectionId m_count;
  Qt::CheckState m_checkbox;
};

}  // namespace glretrace

#endif  // _GLFRAME_EXPERIMENT_MODEL_HPP_
