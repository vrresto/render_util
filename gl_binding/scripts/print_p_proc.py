#!/usr/bin/env python

import sys
import os
import parser
import enabled_procs

def main():
  api = parser.parseAPI()

  for func in api.functionIterateByOffset():
    if func.desktop != True:
      continue

    for ep in func.entry_points:
      if enabled_procs.isProcEnabled(ep):
        print func.return_type + " GLAPIENTRY (*" + ep + ") (" + func.get_parameter_string(ep) + ") = nullptr;"

if __name__ == '__main__':
    main()
