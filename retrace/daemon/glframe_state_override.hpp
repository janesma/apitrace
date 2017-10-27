// Copyright (C) Intel Corp.  2017.  All Rights Reserved.

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

#include <GL/gl.h>

#include <map>
#include <vector>

#include "glframe_retrace_interface.hpp"


namespace glretrace {

class StateOverride {
 public:
  StateOverride() {}
  void setState(const StateKey &item,
                int offset,
                float value);
  void setState(const StateKey &item,
                GLint value);
  void saveState();
  void overrideState() const;
  void restoreState() const;

  void onState(SelectionId selId,
               ExperimentId experimentCount,
               RenderId renderId,
               OnFrameRetrace *callback);

  void getState(const StateKey &item,
                std::vector<uint32_t> *data);

 private:
  enum Type {
    kUnknown,
    kBool,
    kFloat,
    kEnum
  };

  typedef std::map<StateKey, std::vector<uint32_t>> KeyMap;
  void enact_state(const KeyMap &m) const;
  void enact_enabled_state(GLint setting, bool v) const;
  void enact_int_state(uint32_t k, const std::vector<uint32_t> &v) const;
  void save_enabled_state(const StateKey &k, GLint v);
  void save_int_state(const StateKey &k, GLint v);
  void get_enabled_state(GLint k, std::vector<uint32_t> *data);
  void get_integer_state(GLint k, std::vector<uint32_t> *data);
  void get_float_state(GLint k, std::vector<uint32_t> *data);


  KeyMap m_overrides;
  KeyMap m_saved_state;
  std::map<StateKey, Type> m_data_types;
};

}  // namespace glretrace
