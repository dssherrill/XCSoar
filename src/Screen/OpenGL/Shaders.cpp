/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Shaders.hpp"
#include "Program.hpp"

#include <stdio.h>

namespace OpenGL {
  GLProgram *solid_shader;
  GLProgram *texture_shader;
}

static constexpr char solid_vertex_shader[] =
  "uniform mat4 projection;"
  "uniform mat4 modelview;"
  "attribute vec4 position;"
  "void main() {"
  "  gl_Position = projection * modelview * position;"
  "}";

static constexpr char solid_fragment_shader[] =
  "precision mediump float;"
  "uniform vec4 color;"
  "void main() {"
  "  gl_FragColor = color;"
  "}";

static constexpr char texture_vertex_shader[] =
  "uniform mat4 projection;"
  "uniform mat4 modelview;"
  "attribute vec4 position;"
  "attribute vec2 texcoord;"
  "varying vec2 texcoordvar;"
  "void main() {"
  "  gl_Position = projection * modelview * position;"
  "  texcoordvar = texcoord;"
  "}";

static constexpr char texture_fragment_shader[] =
  "precision mediump float;"
  "uniform sampler2D texture;"
  "uniform vec2 texcoordvar;"
  "void main() {"
  "  gl_FragColor = texture2D(texture, texcoordvar);"
  "}";

static void
CompileAttachShader(GLProgram &program, GLenum type, const char *code)
{
  GLShader shader(type);
  shader.Source(code);
  shader.Compile();

  if (shader.GetCompileStatus() != GL_TRUE) {
    char log[4096];
    shader.GetInfoLog(log, sizeof(log));
    fprintf(stderr, "Shader compiler failed: %s\n", log);
  }

  program.AttachShader(shader);
}

static GLProgram *
MakeProgram(const char *vertex_shader, const char *fragment_shader)
{
  GLProgram *program = new GLProgram();
  CompileAttachShader(*program, GL_VERTEX_SHADER, vertex_shader);
  CompileAttachShader(*program, GL_FRAGMENT_SHADER, fragment_shader);
  program->Link();

  if (program->GetLinkStatus() != GL_TRUE) {
    char log[4096];
    program->GetInfoLog(log, sizeof(log));
    fprintf(stderr, "Shader linker failed: %s\n", log);
  }

  return program;
}

void
OpenGL::InitShaders()
{
  DeinitShaders();

  solid_shader = MakeProgram(solid_vertex_shader, solid_fragment_shader);
  solid_shader->BindAttribLocation(Attribute::COLOR, "color");
  solid_shader->BindAttribLocation(Attribute::POSITION, "position");
  solid_shader->BindAttribLocation(Attribute::PROJECTION, "projection");

  texture_shader = MakeProgram(texture_vertex_shader, texture_fragment_shader);
  texture_shader->BindAttribLocation(Attribute::COLOR, "color");
  texture_shader->BindAttribLocation(Attribute::POSITION, "position");
  texture_shader->BindAttribLocation(Attribute::PROJECTION, "projection");
  texture_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
}

void
OpenGL::DeinitShaders()
{
  delete solid_shader;
  solid_shader = nullptr;
}