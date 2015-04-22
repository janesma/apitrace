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

#include "glframe_state.hpp"

#include <string.h>

#include "trace_model.hpp"

using glretrace::StateTrack;
using trace::Call;

// TODO: use a lookup table
void
StateTrack::track(const trace::Call &call)
{
    const char *name = call.sig->name;
    if (strcmp(name, "glAttachShader") == 0)
        return trackAttachShader(call.args[0].value->toDouble(),
                                 call.args[1].value->toDouble());
    if (strcmp(name, "glCreateShader") == 0)
        return trackCreateShader(call.args[0].value->toDouble(),
                                 call.ret->toDouble());

    if (strcmp(name, "glShaderSource") == 0)
        return trackShaderSource(call.args[0].value->toDouble(),
                                 call.args[2].value->toString());
}

void
StateTrack::trackAttachShader(int program, int shader)
{
    program_to_shaders[program].push_back(shader);
}

void
StateTrack::trackCreateShader(int shader_type, int shader)
{
    shader_to_type[shader] = shader_type;
}

void
StateTrack::trackShaderSource(int shader, const char *source)
{
    shader_to_source[shader] = source;
}

