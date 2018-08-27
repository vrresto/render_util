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

#ifndef RENDER_UTIL_UTIL_BLOCK_ALLOCATOR_H
#define RENDER_UTIL_UTIL_BLOCK_ALLOCATOR_H

#include <array>
#include <memory>
#include <stack>
#include <iostream>

namespace util
{


template <typename T, size_t N>
class BlockAllocator
{
  class Container
  {
    std::array<T, N> m_elements;
    size_t m_size = 0;

  public:
    bool isFull() { return m_elements.size() <= m_size; };

    T *alloc()
    {
      if (!isFull())
      {
        T *element = &m_elements[m_size];
        m_size++;
        return element;
      }
      return nullptr;
    }
  };

  std::stack<std::shared_ptr<Container>> m_containers;

public:
  T *alloc()
  {
    if (m_containers.empty() || m_containers.top()->isFull())
      m_containers.push(std::make_shared<Container>());

    assert(!m_containers.top()->isFull());

    return m_containers.top()->alloc();
  }

  void clear()
  {
    m_containers = {};
  }

};


} // namespace util

#endif
