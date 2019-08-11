#!/usr/bin/env python

import sys
import os
import parser

api = parser.parseAPI()

for func in api.functionIterateByOffset():
  if func.desktop != True:
    continue

  for ep in func.entry_points:
    print "{\"gl" + ep + "\", (void*) &forward_" + ep + "},"
