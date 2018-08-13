/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIMPLE_DISPATCHER_H
#define SIMPLE_DISPATCHER_H

#include <iostream>
#include <functional>

class SimpleDispatcher
{
public:
  typedef std::function<void(int)> WorkFunction;

private:
  WorkFunction do_work;

public:
  SimpleDispatcher(WorkFunction f) : do_work(f) {}

  void dispatch(int num_items)
  {
    float progress_percent = 0;

    std::cout.precision(2);
    std::cout << std::fixed;

    for (int i = 0; i < num_items; i++)
    {
      do_work(i);
      int progress = i;

      float progress_percent_new = progress * 100 / (float)(num_items);

      if (progress_percent_new != progress_percent)
      {
        progress_percent = progress_percent_new;
        std::cout<<"progress: "<<progress_percent<<" %"<<std::endl;
      }
    }
  }

};

#endif
