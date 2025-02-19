#include <cstdio>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;
using std::vector;

std::vector<std::string> online_user = {"<Client0>", "<Client1>", "<Client2>"};
std::vector<std::string> user_messages = {"<Client0>: wagwan",
                                          "<Client1>: ain none gang!"};

int main() {
  ScreenInteractive screen = ScreenInteractive::TerminalOutput();

  //**************************************************************************
  // Components and/or Elements
  //**************************************************************************
  std::string input_text = "";
  Component input_box = Input(&input_text, "type message...");

  Element input_window =
      window(text("Type Messages ") | hcenter, text("type "));
  // This should be the side panel that shows who online in the chatroom
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
      user_messages.push_back("<You>: " + input_text +
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

  auto window_layout = Container::Vertical(
      {Container::Horizontal(
           {online_panel | size(WIDTH, GREATER_THAN, online_panel_sizex),
            message_window | size(WIDTH, GREATER_THAN, message_window_sizex)}),
       input_box});

  auto debug = [&]() -> Component {
    input_len = input_text.length();
    std::cout << "The Input Text " << input_len << std::endl;

    return window_layout;
  };

  screen.Loop(debug());
  return EXIT_SUCCESS;
}
