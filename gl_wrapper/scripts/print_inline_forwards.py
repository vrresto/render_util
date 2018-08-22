#!/usr/bin/env python

import sys
import os
import parser
import gl_XML

def get_called_parameter_string(func, ep):
    p_string = ""
    comma = ""

    for p in func.parameterIterator(ep):
        if p.is_padding:
            continue
        p_string = p_string + comma + p.name
        comma = ", "

    return p_string

def main():
  api = parser.parseAPI()

  for func in api.functionIterateByOffset():
    if func.desktop != True:
      continue

    for ep in func.entry_points:
      ep_params = func.entry_point_parameters[ep]
      ep_name = "gl" + ep
      print "inline " + func.return_type + " " + ep + "(" + gl_XML.create_parameter_string(ep_params, 1)  + ") {"
      print "  assert(GL_Interface::getCurrent()->" + ep_name + ");"
      if func.return_type != "void":
        return_statement = "  return "
      else:
        return_statement = "  "
      print return_statement + "GL_Interface::getCurrent()->" + ep_name + "(" + get_called_parameter_string(func, ep) + ");"
      print "}"

if __name__ == '__main__':
    main()
