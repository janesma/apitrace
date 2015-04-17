/**************************************************************************
 *
 * Copyright 2011-2014 Jose Fonseca
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

#include "glstate.hpp"

namespace glstate {
struct ImageDesc
{
    GLint width;
    GLint height;
    GLint depth;
    GLint samples;
    GLint internalFormat;

    inline
    ImageDesc() :
        width(0),
        height(0),
        depth(0),
        samples(0),
        internalFormat(GL_NONE)
    {}

    inline bool
    operator == (const ImageDesc &other) const {
        return width == other.width &&
               height == other.height &&
               depth == other.depth &&
               samples == other.samples &&
               internalFormat == other.internalFormat;
    }

    inline bool
    valid(void) const {
        return width > 0 && height > 0 && depth > 0;
    }
};
}
