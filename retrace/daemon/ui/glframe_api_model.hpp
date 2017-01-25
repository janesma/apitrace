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

#ifndef _GLFRAME_API_MODEL_HPP_
#define _GLFRAME_API_MODEL_HPP_

#include <QObject>
#include <QString>

#include <mutex>
#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"

namespace glretrace {

class QApiModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString apiCalls READ apiCalls NOTIFY onApiCalls)
 public:
  ~QApiModel();
  QString apiCalls();
  void onApi(SelectionId selectionCount,
             RenderId renderId,
             const std::vector<std::string> &api_calls);
 signals:
  void onApiCalls();

 private:
  QString m_api_calls;
};

}  // namespace glretrace

#endif  // _GLFRAME_API_MODEL_HPP_
