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

#ifndef _GLFRAME_BATCH_MODEL_HPP_
#define _GLFRAME_BATCH_MODEL_HPP_

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
class QBatchModel : public QObject,
                  NoCopy, NoAssign, NoMove{
  Q_OBJECT
  Q_PROPERTY(QString batch READ batch NOTIFY onBatchChanged)
  Q_PROPERTY(QStringList renders READ renders NOTIFY onRenders)
 public:
  QBatchModel();
  ~QBatchModel();
  QString batch();
  QStringList renders() const;
  void onBatch(SelectionId selectionCount,
                 RenderId renderId,
                 const std::string &batch);
  Q_INVOKABLE void setIndex(int index);

 signals:
  void onBatchChanged();
  void onRenders();

 private:
  std::map<QString, QString> m_batch;
  QStringList m_renders;
  SelectionId m_sel_count;
  int m_index;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_BATCH_MODEL_HPP_
