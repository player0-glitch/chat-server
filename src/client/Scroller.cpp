#include "../include/Scroller.h"

#include <algorithm>                          // for max, min
#include <ftxui/component/component_base.hpp> // for Component, ComponentBase
#include <ftxui/component/event.hpp> // for Event, Event::ArrowDown, Event::ArrowUp, Event::End, Event::Home, Event::PageDown, Event::PageUp
#include <utility>                   // for move

#include <ftxui/component/component.hpp> // for Make
#include <ftxui/component/mouse.hpp> // for Mouse, Mouse::WheelDown, Mouse::WheelUp
#include <ftxui/dom/deprecated.hpp> // for text
#include <ftxui/dom/elements.hpp> // for operator|, Element, size, vbox, EQUAL, HEIGHT, dbox, reflect, focus, inverted, nothing, select, vscroll_indicator, yflex, yframe
#include <ftxui/dom/node.hpp>     // for Node
#include <ftxui/dom/requirement.hpp> // for Requirement
#include <ftxui/screen/box.hpp>      // for Box

namespace ftxui {

class ScrollerBase : public ComponentBase {
public:
  ScrollerBase(Component child) { Add(child); }

private:
  Element Render() {
    auto focused = Focused() ? focus : ftxui::select;
    auto style = Focused() ? inverted : nothing;

    Element background = ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;
    return dbox({
               std::move(background),
               vbox({
                   text(L"") | size(HEIGHT, EQUAL, selected_),
                   text(L"") | style | focused,
               }),
           }) |
           vscroll_indicator | yframe | yflex | reflect(box_);
  }

  bool OnEvent(Event event) final {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
      TakeFocus();

    int selected_old = selected_;
    if (event == Event::ArrowUp || event == Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
      selected_--;
    }
    if ((event == Event::ArrowDown || event == Event::Character('j') ||
         (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
      selected_++;
    }
    if (event == Event::PageDown)
      selected_ += box_.y_max - box_.y_min;
    if (event == Event::PageUp)
      selected_ -= box_.y_max - box_.y_min;
    if (event == Event::Home)
      selected_ = 0;
    if (event == Event::End)
      selected_ = size_;

    selected_ = std::max(0, std::min(size_ - 1, selected_));
    return selected_old != selected_;
  }

  bool Focusable() const final { return true; }

  int selected_ = 0;
  int size_ = 0;
  Box box_;
};

Component Scroller(Component child) {
  return Make<ScrollerBase>(std::move(child));
}
} // namespace ftxui

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
