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

#include "glframe_api_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_qutil.hpp"

using glretrace::QApiModel;
using glretrace::RenderId;
using glretrace::ScopedLock;
using glretrace::SelectionId;

QApiModel::~QApiModel() {
    m_api_calls.clear();
}

QString
QApiModel::apiCalls() {
  return m_api_calls;
}

void
QApiModel::onApi(SelectionId selectionCount,
                 RenderId renderId,
                 const std::vector<std::string> &api_calls) {
  m_api_calls.clear();

  // TODO(majanes): cache calls by render
  for (auto i : api_calls) {
    m_api_calls.append(QString::fromStdString(i));
  }
  emit onApiCalls();
}

