#!/usr/bin/env python

import sys
import os
import parser
import gl_XML
import enabled_procs

def getArgs(func, ep):
    p_string = ""
    comma = ""

    for p in func.parameterIterator(ep):
        if p.is_padding:
            continue
        p_string = p_string + comma + p.name
        comma = ", "

    return p_string


api = parser.parseAPI()

for func in api.functionIterateByOffset():
  if func.desktop != True:
    continue

  for ep in func.entry_points:
    if not enabled_procs.isProcEnabled(ep):
      continue

    ep_params = func.entry_point_parameters[ep]
    ep_name = ep
    print "inline " + func.return_type + " " + ep + "(" + gl_XML.create_parameter_string(ep_params, 1)  + ")"
    print "{"

    print "  auto iface = getCurrentInterface();"

    if func.return_type != "void":
      ret_assignment = "  auto ret = "
    else:
      ret_assignment = "  "

    print ret_assignment + "iface->" + ep_name + "(" + getArgs(func, ep) + ");"

    print "  assert(!iface->hasError());"

    if func.return_type != "void":
      print "  return ret;"

    print "}"
    print
