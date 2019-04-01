/**************************************************************************
 *
 * Copyright 2019 Intel Corporation
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

#include "glframe_retrace_texture.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "md5.h"  // NOLINT
#include "state_writer.hpp"
#include "glframe_state_enums.hpp"
#include "glstate_internal.hpp"

using glretrace::state_name_to_enum;
using glretrace::RenderId;
using glretrace::SelectionId;
using glretrace::ExperimentId;
using glretrace::OnFrameRetrace;
using image::Image;
using glretrace::TextureKey;
using glretrace::TextureData;
using glretrace::Textures;

class TextureCollector : public StateWriter {
 public:
  TextureCollector(RenderId render,
                   SelectionId selectionCount,
                   ExperimentId experimentCount,
                   OnFrameRetrace *callback,
                   Textures *filter)
      : m_state(k_none),
        m_render(render),
        m_selection_id(selectionCount),
        m_experiment_id(experimentCount),
        m_cb(callback),
        m_filter(filter) {}
  ~TextureCollector() {}

  // sample object to show state machine iteration:
  //   {
  //   "textures": {
  //     "GL_TEXTURE0, GL_TEXTURE_2D, level = 0": {
  //       "__class__": "image",
  //       "__width__": 2,
  //       "__height__": 2,
  //       "__depth__": 1,
  //       "__format__": "GL_RGBA",
  //       "__data__": "iV"
  // A/3T6pQBAAAAAElFTkSuQmCC"
  //     }
  //   }
  // }

  virtual void
  beginObject(void) {}

  virtual void
  endObject(void) {}

  virtual void
  beginMember(const char * name) {
    switch (m_state) {
      case k_none:
        if (strcmp(name, "textures") == 0)
          m_state = k_textures;
        else
          assert(false);
        return;
      case k_a_texture:
        if (strcmp(name, "__class__") ==0) {
          m_state = k_class;
        } else if (strcmp(name, "__width__") ==0) {
          m_state = k_width;
        } else if (strcmp(name, "__height__") ==0) {
          m_state = k_height;
        } else if (strcmp(name, "__depth__") ==0) {
          m_state = k_depth;
        } else if (strcmp(name, "__format__") ==0) {
          m_state = k_format;
        } else if (strcmp(name, "__data__") ==0) {
          m_state = k_data;
        } else {
          assert(false);
          return;
        }
        return;
      case k_textures: {
        // split name on comma
        std::vector<std::string> words;
        std::string word;
        std::istringstream name_s(name);
        while (std::getline(name_s, word, ',')) {
          word.erase(0, word.find_first_not_of(" "));
          words.push_back(word);
        }

        // defaults will be used for image load store (unsupported)
        int next_unit = -1;
        int next_target = -1;
        if (words.size() > 1) {
          next_unit = state_name_to_enum(words[0]);
          next_target= state_name_to_enum(words[1]);
        }

        if ((next_unit != m_key.unit) ||
             (next_target != m_key.target)) {
          // new binding.  Send the cached textures before proceeding
          if (m_key.valid()) {
            // supported unit (not image load store)
            assert(m_textures.size());
            m_cb->onTexture(m_selection_id,
                            m_experiment_id,
                            m_render,
                            m_key,
                            m_textures);
          }
          m_textures.clear();
        }
        m_key.unit = next_unit;
        m_key.target = next_target;

        // new texture
        m_textures.push_back(TextureData());

        // image load store textures and GL_TEXTURE_BUFFER bindings
        // will be invalid
        if (m_key.target == GL_TEXTURE_BUFFER)
          m_key.target = -1;
        m_textures.back().level = -1;

        if (m_key.valid()) {
          const char* level_prefix = "level = ";
          assert(strncmp(level_prefix,
                         words[2].c_str(),
                         strlen(level_prefix)) == 0);
          m_textures.back().level = atoi(words[2].c_str() +
                                         strlen(level_prefix));
        }
        m_state = k_a_texture;
        return;
      }
      case k_width:
      case k_height:
      case k_depth:
      case k_data:
      case k_class:
      case k_format:
        assert(false);
        return;
    }
  }

  virtual void
  endMember(void) {
    switch (m_state) {
      case k_none:
        assert(false);
        return;
      case k_width:
      case k_height:
      case k_depth:
      case k_data:
      case k_class:
      case k_format:
        m_state = k_a_texture;
        return;
      case k_a_texture:
        m_state = k_textures;
        return;
      case k_textures:
        if (m_textures.size()) {
          if (m_key.unit > -1)
            m_cb->onTexture(m_selection_id,
                            m_experiment_id,
                            m_render,
                            m_key,
                            m_textures);
          m_state = k_none;
        }
        return;
    }
  }

  virtual void beginArray(void) { assert(false); }

  virtual void endArray(void) { assert(false); }

  virtual void
  writeString(const char *s) {
    switch (m_state) {
      case k_none:
      case k_width:
      case k_height:
      case k_depth:
      case k_data:
      case k_textures:
      case k_a_texture:
        assert(false);
        return;
      case k_class:
        m_textures.back().type = s;
        return;
      case k_format:
        m_textures.back().format = s;
        return;
    }
  }

  virtual void
  writeBlob(const void *bytes, size_t size) {
    assert(m_state == k_data);

    if (m_key.unit == -1)
      // image_load_store
      return;

    // create a md5sum name for the image
    struct MD5Context md5c;
    MD5Init(&md5c);
    const unsigned char* ucb = reinterpret_cast<const unsigned char*>(bytes);
    MD5Update(&md5c, const_cast<unsigned char*>(ucb), size);
    std::vector<unsigned char> md5(16);
    MD5Final(md5.data(), &md5c);
    std::vector<char> md5sum(md5.size() * 2 + 1);
    md5sum.back() = '\0';
    for (size_t i = 0; i < md5.size(); ++i)
      snprintf(&md5sum[i * 2], 3, "%02X", md5[i]); // NOLINT
    m_textures.back().md5sum = md5sum.data();

    std::vector<unsigned char> image_data(size);
    memcpy(image_data.data(), bytes, size);
    m_filter->onTextureData(m_experiment_id, m_textures.back().md5sum,
                            image_data);
  }

  virtual void
  writeNull(void) { assert(false); }

  virtual void
  writeBool(bool d) { assert(false); }

  virtual void
  writeSInt(signed long long d) {   // NOLINT
    handleint(d);
  }

  virtual void
  writeUInt(unsigned long long d) {   // NOLINT
    handleint(d);
  }

  virtual void
  writeFloat(float d) { assert(false); }

  virtual void
  writeFloat(double d) { assert(false); }

 private:
  void handleint(int d) {
    switch (m_state) {
      case k_none:
      case k_textures:
      case k_a_texture:
      case k_class:
      case k_format:
      case k_data:
        assert(false);
        return;
      case k_width:
        m_textures.back().width = d;
        return;
      case k_height:
        m_textures.back().height = d;
        return;
      case k_depth:
        m_key.offset = d;
        return;
    }
  }


  TextureKey m_key;
  std::vector<TextureData> m_textures;
  enum MachineState {
    k_none,
    k_textures,
    k_a_texture,
    k_class,
    k_width,
    k_height,
    k_depth,
    k_format,
    k_data } m_state;
  RenderId m_render;
  SelectionId m_selection_id;
  ExperimentId m_experiment_id;
  OnFrameRetrace *m_cb;
  Textures *m_filter;
};

void
Textures::retraceTextures(RenderId render,
                          SelectionId selectionCount,
                          ExperimentId experimentCount,
                          OnFrameRetrace *callback) {
  if (experimentCount > m_last_experiment) {
    m_cached_textures.clear();
    m_last_experiment = experimentCount;
  }

  m_callback = callback;
  glstate::Context context;

  // // for debugging texture data
  // std::ostringstream os;
  // StateWriter *dump = createJSONStateWriter(os);
  // glstate::dumpTextures(*dump, c);
  // // glframe_logger.hpp will probably drop data.  Increase BUF_SIZE
  // GRLOGF(WARN, "JSON dump of texture: %s", os.str().c_str());

  TextureCollector tex_collect(render,
                               selectionCount, experimentCount,
                               callback, this);
  glstate::dumpTextures(tex_collect, context);
}

void
Textures::onTextureData(ExperimentId experimentCount,
                        const std::string &md5sum,
                        const std::vector<unsigned char> &image) {
  if (m_cached_textures.find(md5sum) != m_cached_textures.end())
      // client already has this image
      return;
  m_cached_textures[md5sum] = true;
  m_callback->onTextureData(experimentCount, md5sum, image);
}


