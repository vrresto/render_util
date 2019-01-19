#!/usr/bin/env python

import sys
import os
import parser

def main():
  api = parser.parseAPI()

  for func in api.functionIterateByOffset():
    if func.desktop != True:
      continue

    for ep in func.entry_points:
      print func.return_type + " GLAPIENTRY (*gl" + ep + ") (" + func.get_parameter_string(ep) + ") = 0;"


if __name__ == '__main__':
    main()
