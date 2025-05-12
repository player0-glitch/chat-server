#include "../include/Scroller.h"

#include <cstdio>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <string>
#include <vector>

using namespace ftxui;
using std::vector;

std::vector<std::string> online_user = {"<Client0>", "<Client1>", "<Client2>"};
std::vector<std::string> user_messages = {"<Client0>: wagwan",
                                          "<Client1>: ain none gang!",
                                          "<Client0>: Aight say less then"};
// auto create_messages_window(vector<std::string> &user_messages) ->
// Component;
auto create_online_side_panel(vector<std::string> &users_online) -> Component;

int main() {
  ScreenInteractive screen = ScreenInteractive::TerminalOutput();
  user_messages.reserve(500);
  //**************************************************************************
  // Components and/or Elements
  //**************************************************************************
  std::string input_text = "";
  Component input_box = Input(&input_text, "type message...");

  auto online_panel = Renderer([] {
    vector<Element> user_lines;
    for (const auto &u : online_user)
      user_lines.push_back(paragraphAlignLeft(u));

    return window(text("Online") | hcenter,
                  vbox(user_lines) | frame | size(WIDTH, GREATER_THAN, 20));
  });
  // This shows the main messages thread
  auto message_window = Renderer([] {
    vector<Element> message_lines;
    for (const auto &msg : user_messages)
      message_lines.emplace_back(paragraphAlignLeft(msg));

    return window(text("Messages") | hcenter,
                  vbox(message_lines) | size(WIDTH, GREATER_THAN, 20));
  });

  //**************************************************************************
  //  Event Handling
  //**************************************************************************

  // this CatchEvent function takes a lambda as an arguement
  int input_len = -1;
  input_box |= CatchEvent([&](Event e) {
    if (e == Event::Return) {

      input_len = input_text.length();
      if (!(input_len > 0))
        return false;
      user_messages.push_back("<You>: " + input_text + " " +
                              std::to_string(input_len));
      input_text.clear();
      return true;
    }
    return false;
  });

  //**************************************************************************
  // Container for screen loop rendering
  //**************************************************************************

  /* 2/3 of the terminal window will be the messages part
   * 1/3 of the terminal window will be the online part
   */
  Dimensions terminal_size = Terminal::Size();
  int online_panel_sizex = (terminal_size.dimx) / 3;
  int message_window_sizex = (2 * terminal_size.dimx) / 3;
  auto side_panel = create_online_side_panel(online_user);
  auto window_layout = Container::Vertical(
      {Container::Horizontal(
           {Container::Vertical({side_panel |
                                 size(WIDTH, GREATER_THAN, online_panel_sizex) |
                                 size(HEIGHT, GREATER_THAN, 12)}),
            message_window | size(WIDTH, GREATER_THAN, message_window_sizex) |
                size(HEIGHT, LESS_THAN, 20)}),
       input_box});

  auto debug = [&]() -> Component {
    input_len = input_text.length();
    /*std::cout << "The Input Text " << input_len << std::endl;*/

    return window_layout;
  };

  screen.Loop(debug());
  return EXIT_SUCCESS;
}

/*@brief method used to create scrollable message window area
 *@param user_messages is a vector of all the user messages coming and going
 * to the server
auto create_messages_window(vector<std::string> &user_messages) -> Component {
  vector<Component> message_components;
  for (const auto &msg : user_messages)

    // Create a vertical Container to be able to make it scrollable
    auto online_panel = Renderer([] {
      vector<Element> user_lines;
      for (const auto &u : online_user)
        user_lines.push_back(paragraphAlignLeft(u));

      return window(text("Online") | hcenter,
                    vbox(user_lines) | frame | size(WIDTH, GREATER_THAN, 20));
    });

  auto container =
      Container::Vertical({Container::Horizontal({message_components})});
   auto container = middle;
   container = ResizableSplitLeft(left, container, &left_size);
   container = ResizableSplitRight(right, container, &right_size);
   container = ResizableSplitTop(top, container, &top_size);
   container = ResizableSplitBottom(bottom, container, &bottom_size);

   auto renderer =
       Renderer(container, [&] { return container->Render() | border; });
   */
// Making the online panel for users to see who is connected to chat server
/*
  container |= vscroll_indicator;
  return container;
}*/

auto create_scrollable_messages_win(vector<std::string> &user_messages)
    -> Component {
  return nullptr;
}

auto create_online_side_panel(vector<std::string> &users_online) -> Component {

  // create a menu item for every item in the passed in vector
  vector<std::string> entries = {"\0", "local", "remote"};
  vector<Component> drop_downs;
  for (const auto &u : users_online) {
    entries[0] = u; // the first drop down entry should be the name
    auto dropdown_1 = Dropdown(
        {.radiobox = {.entries = &entries},
         .transform = [](bool open, Element checkbox, Element radiobox) {
           if (open) {
             return vbox({
                 radiobox | vscroll_indicator | frame |
                     size(HEIGHT, LESS_THAN, 10) | bgcolor(Color::Blue),
                 filler(),
             });
           }
           return vbox({
               radiobox | bgcolor(Color::Blue),
               filler(),
           });
         }});
    drop_downs.push_back(dropdown_1);
  }

  auto container = Container::Vertical({drop_downs});

  return Scroller(container);
}
