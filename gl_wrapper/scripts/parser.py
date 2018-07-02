import sys
import os
import gl_XML

def parseAPI():
  glapi_path = os.environ['GLAPI_PATH']
  return gl_XML.parse_GL_API(glapi_path + "/gl_API.xml")
