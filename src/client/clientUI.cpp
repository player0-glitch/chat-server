#include <cstdio>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <vector>

using namespace ftxui;
using std::vector;

std::vector<std::string> online_user = {"<Client0>", "<Client1>", "<Client2>"};
std::vector<std::string> user_messages = {"<Client0>: wagwan",
                                          "<Client1>: ain none gang!"};

int main() {
  auto screen = ScreenInteractive::TerminalOutput();
  // Main component for ui
  /*auto main_cell = [](std::string title, std::string s, int window_width,*/
  /*                    int window_height) {*/
  /*  // Horizontal box with text wrapped in a window*/
  /*  return vbox({window(text(title) | flex | size(WIDTH, EQUAL,
   * window_width)*/
  /*                      size(HEIGHT, LESS_THAN, window_height)*/

  /*                      paragraphAlignLeft(s) | flex)});*/
  /*};*/
  std::string input_text;
  auto input_box = Input(&input_text, "type message...");

  // This should be the side panel that shows who online in the chatroom
  auto online_panel = Renderer([] {
    vector<Element> user_lines;
    for (const auto &u : online_user)
      user_lines.push_back(paragraphAlignLeft(u));

    return window(text("Online"),
                  vbox(user_lines) | frame | size(WIDTH, LESS_THAN, 20));
  });

  // This shows the main messages thread
  auto message_window = Renderer([] {
    vector<Element> message_lines;
    for (const auto &msg : user_messages)
      message_lines.emplace_back(paragraphAlignLeft(msg));

    return window(text("Messages"),
                  vbox(message_lines) | frame | size(WIDTH, LESS_THAN, 20));
  });

  //**************************************************************************
  //  Event Handling
  //**************************************************************************

  // this CatchEvent function takes a lambda as an arguement
  input_box |= CatchEvent([&](Event e) {
    if (e == Event::Return) {
      user_messages.push_back("<You>: " + input_text);
      input_text.clear();
      return true;
    }
    return false;
  });

  //**************************************************************************
  // Container for screen loop rendering
  //**************************************************************************

  auto window_layout = Container::Vertical(
      {Container::Horizontal({online_panel, // should show who is online
                              message_window}),
       input_box});

  screen.Loop(window_layout);

  return EXIT_SUCCESS;
}
