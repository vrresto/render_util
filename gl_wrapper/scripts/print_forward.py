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
      out_str = "gl: " + ep;

      print "static " + func.return_type + " GLAPIENTRY forward_" + ep + "(" + gl_XML.create_parameter_string(ep_params, 1)  + ") {"

      #print "cout << \"" + out_str + "\" << endl;"
      #print "assert(current_GL_Interface()->" + ep_name + ");"

      if func.return_type != "void":
        print "  return current_GL_Interface()->" + ep_name + "(" + get_called_parameter_string(func, ep) + ");"
      else:
        print "  if (!discard_gl_calls)"
        print "    current_GL_Interface()->" + ep_name + "(" + get_called_parameter_string(func, ep) + ");"

      print "}"

if __name__ == '__main__':
    main()
