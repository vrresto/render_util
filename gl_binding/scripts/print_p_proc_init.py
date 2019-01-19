#!/usr/bin/env python

import sys
import os
import parser
import gl_XML

def main():
  api = parser.parseAPI()

  for func in api.functionIterateByOffset():
    if func.desktop != True:
      continue

    for ep in func.entry_points:
      ep_name = "gl" + ep
      ep_params = func.entry_point_parameters[ep]
      ep_type = func.return_type + " GLAPIENTRY (*) (" + gl_XML.create_parameter_string(ep_params, 0)  + ")"
      print ep_name + " = (" + ep_type + ") getProcAddress(\"gl" + ep + "\");"


if __name__ == '__main__':
    main()
