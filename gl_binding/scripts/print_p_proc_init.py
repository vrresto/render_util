#!/usr/bin/env python

import sys
import os
import parser
import gl_XML
import enabled_procs

api = parser.parseAPI()

for func in api.functionIterateByOffset():
  if func.desktop != True:
    continue

  for ep in func.entry_points:
    if not enabled_procs.isProcEnabled(ep):
      continue
    ep_name = ep
    ep_params = func.entry_point_parameters[ep]
    ep_type = func.return_type + " GLAPIENTRY (*) (" + gl_XML.create_parameter_string(ep_params, 0)  + ")"
    print ep_name + " = (" + ep_type + ") get_proc_address(\"gl" + ep + "\");"
