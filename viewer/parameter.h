/**
 *    Rendering utilities
 *    Copyright (C) 2020 Jan Lepper
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

#ifndef RENDER_UTIL_VIEWER_PARAMETER_H
#define RENDER_UTIL_VIEWER_PARAMETER_H

#include <vector>
#include <functional>

namespace render_util::viewer
{


template <typename T>
inline std::string getParameterValueString(T value)
{
  return std::to_string(value);
}


template <>
inline std::string getParameterValueString<float>(float value)
{
  char buf[100];
  snprintf(buf, sizeof(buf), "%.2f", value);
  return buf;
}


template <>
inline std::string getParameterValueString<bool>(bool value)
{
  return value ? "on" : "off";
}


struct Parameter
{
  enum class Dimension
  {
    X, Y, Z
  };

  const std::string name;

  Parameter(std::string name) : name(name) {}

  virtual void reset() = 0;
  virtual void decrease(float step_factor = 1, Dimension dim = Dimension::X) = 0;
  virtual void increase(float step_factor = 1, Dimension dim = Dimension::X) = 0;
  virtual std::string getValueString() = 0;
};


template <typename T>
class SimpleValueWrapper
{
  T &m_value;

public:
  SimpleValueWrapper(T value) : m_value(value) {}
  T get() { return m_value; }
  void set(T value) { m_value = value; }
};


template <typename T>
struct ValueWrapper
{
  using GetFunc = std::function<T()>;
  using SetFunc = std::function<void(T)>;

  const GetFunc get;
  const SetFunc set;
};


template <typename T, class W>
class SimpleParameter : public Parameter
{
  using Wrapper = W;

  const T default_value {};
  const T step {};

  Wrapper m_value;

public:
  SimpleParameter(std::string name, float step, Wrapper wrapper) : Parameter(name),
    default_value(wrapper.get()), step(step), m_value(wrapper)
  {
  }

  void reset() override
  {
    m_value.set(default_value);
  }

  void increase(float step_factor, Dimension) override
  {
    assert(step_factor > 0);
    m_value.set(m_value.get() + step_factor * step);
  }

  void decrease(float step_factor, Dimension) override
  {
    assert(step_factor > 0);
    m_value.set(m_value.get() - step_factor * step);
  }

  std::string getValueString() override
  {
    return getParameterValueString(m_value.get());
  }
};


template <typename T>
class MultipleChoiceParameter : public Parameter
{
  using Applier = std::function<void(T)>;

  const std::vector<T> m_possible_values;
  int m_current_index = 0;
  Applier m_apply;

public:
  MultipleChoiceParameter(std::string name,
                          Applier apply_,
                          const std::vector<T> &possible_values) :
    Parameter(name),
    m_possible_values(possible_values),
    m_apply(apply_)
  {
    apply();
  }

  T getCurrentValue() { return m_possible_values.at(m_current_index); }
  void apply() { m_apply(getCurrentValue()); }

  void reset() override
  {
    m_current_index = 0;
    apply();
  }

  void increase(float step_factor, Dimension) override
  {
    m_current_index = (m_current_index+1) % m_possible_values.size();
    apply();
  }

  void decrease(float step_factor, Dimension) override
  {
    m_current_index = (m_current_index-1) % m_possible_values.size();
    apply();
  }

  std::string getValueString() override
  {
    return getParameterValueString(getCurrentValue());
  }
};


struct Vec2Parameter : public Parameter
{
  const glm::vec2 default_value = glm::vec2(0);
  const float step = 1;
  glm::vec2 &value;

  Vec2Parameter(std::string name, glm::vec2 &value, float step) : Parameter(name),
    default_value(value), step(step), value(value)
  {
  }

  void change(Dimension dim, float step_factor)
  {
    switch (dim)
    {
      case Dimension::X:
        value.x += step_factor * step;
        break;
      case Dimension::Y:
        value.y += step_factor * step;
        break;
    }
  }

  void reset() override
  {
    value = default_value;
  }

  void decrease(float step_factor, Dimension dim) override
  {
    assert(step_factor > 0);
    change(dim, -step_factor);
  }

  void increase(float step_factor, Dimension dim) override
  {
    assert(step_factor > 0);
    change(dim, +step_factor);
  }

  std::string getValueString() override
  {
    char buf[100];
    snprintf(buf, sizeof(buf), "[%.2f, %.2f]", value.x, value.y);
    return buf;
  }
};


class Parameters
{
  std::vector<std::unique_ptr<Parameter>> m_parameters;
  int m_active_parameter_index = 0;

public:
  const std::vector<std::unique_ptr<Parameter>> &getParameters() { return m_parameters; }

  int getActiveIndex() { return m_active_parameter_index; }

  void setActive(int index)
  {
    if (m_parameters.empty())
      return;

    if (index < 0)
      index += m_parameters.size();
    index = index % m_parameters.size();
    m_active_parameter_index = index;
  }

  Parameter &getActive() { return *m_parameters.at(m_active_parameter_index); }

  template <typename T>
  void addMultipleChoice(std::string name,
                         std::function<void(T)> apply,
                         const std::vector<T> &values)
  {
    auto p = std::make_unique<MultipleChoiceParameter<T>>(name, apply, values);
    m_parameters.push_back(std::move(p));
  }

  void addBool(std::string name, std::function<void(bool)> apply, bool default_value)
  {
    std::vector<bool> values { default_value, !default_value };
    addMultipleChoice<bool>(name, apply, values);
  }

  template <typename T, typename ... Args>
  void add(Args ... args)
  {
    auto p = std::make_unique<T>(args...);
    m_parameters.push_back(std::move(p));
  }

  template <typename T>
  void add(std::string name, T &value, float step = 1.0)
  {
    using Wrapper = SimpleValueWrapper<T>;
    auto p = std::make_unique<SimpleParameter<T,Wrapper>>(name, step, Wrapper(value));
    m_parameters.push_back(std::move(p));
  }

  template <typename T, class Wrapper>
  void add(std::string name, Wrapper wrapper, float step = 1.0)
  {
    auto p = std::make_unique<SimpleParameter<T,Wrapper>>(name, step, wrapper);
    m_parameters.push_back(std::move(p));
  }

  void add(std::string name, glm::vec2 &value, float step)
  {
    auto p = std::make_unique<Vec2Parameter>(name, value, step);
    m_parameters.push_back(std::move(p));
  }
};


} // namespace render_util::viewer


#endif
