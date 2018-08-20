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

#ifndef THREADED_DISPATCHER_H
#define THREADED_DISPATCHER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <functional>
#include <cassert>

class ThreadedDispatcher
{
public:
  typedef std::function<void(int)> WorkFunction;

private:
  typedef std::unique_lock<std::mutex> Lock;

  enum
  {
    NUM_THREADS = 4
  };

  std::condition_variable start_cond;
  std::mutex start_cond_mutex;
  int num_ready_threads = 0;
  int m_progress = 0;
  WorkFunction do_work;


  void threadMain(int thread_id, int start, int end)
  {
    {
      Lock lock(start_cond_mutex);
      num_ready_threads++;
      start_cond.wait(lock);
      std::cout<<"thread "<<thread_id<<" start."<<std::endl;
    }

    for (int i = start; i < end; i++)
    {
      do_work(i);

      {
        Lock lock(start_cond_mutex);
        m_progress++;
      }
    }

    {
      Lock lock(start_cond_mutex);
      std::cout<<"thread "<<thread_id<<" end."<<std::endl;
    }
  }


public:
  ThreadedDispatcher(WorkFunction f) : do_work(f) {}


  void dispatch(int num_items)
  {
    int batch_size = num_items / NUM_THREADS;
    assert(num_items % NUM_THREADS == 0);

    std::cout<<"batch_size: "<<batch_size<<std::endl;

    std::thread threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
      std::cout<<"creating thread "<<i<<std::endl;

      int start_item = i * batch_size;
      int end_item = start_item + batch_size;

      assert(start_item < num_items);
      assert(end_item <= num_items);

      threads[i] = std::thread(&ThreadedDispatcher::threadMain, this, i, start_item, end_item);
    }

    while (1)
    {
      Lock lock(start_cond_mutex);
      if (num_ready_threads == NUM_THREADS)
        break;
    }

    start_cond.notify_all();

    int progress = 0;
    float progress_percent = 0;

    std::cout.precision(2);
    std::cout << std::fixed;

    while (progress < num_items)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      {
        Lock lock(start_cond_mutex);
        progress = m_progress;
      }

      float progress_percent_new = progress * 100 / (float)(num_items);

      if (progress_percent_new != progress_percent)
      {
        progress_percent = progress_percent_new;
        std::cout<<"progress: "<<progress_percent<<" %"<<std::endl;
      }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
      threads[i].join();
    }
  }

};

#endif
