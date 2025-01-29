#include <cstdio>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
using namespace ftxui;

constinit int WINDOW_WIDTH = 120;
constinit int WINDOW_HEIGHT = 120;

int main() {

  // Main component for ui
  auto main_cell = [](std::string title, std::string s, int window_width,
                      int window_height) {
    // Horizontal box with text wrapped in a window
    return vbox({window(text(title) | flex | size(WIDTH, EQUAL, window_width)
                        /*| size(HEIGHT, LESS_THAN, window_height)*/,
                        paragraphAlignLeft(s) | flex)});
  };
  auto main_document =
      vbox({// top banner/header
            hbox({paragraphAlignCenter("CHAT") | borderDouble | flex}),
            vbox({hbox({// In here we're putting the main stuff
                        main_cell("Online", "Online users", WINDOW_WIDTH / 3,
                                  WINDOW_HEIGHT),
                        main_cell("Chat", "Shows the app chat",
                                  (2 * WINDOW_WIDTH) / 3, WINDOW_HEIGHT),
                        main_cell("Server", "Shows the server message history",
                                  (WINDOW_WIDTH) / 3, WINDOW_HEIGHT)}) |
                  flex})});

  /*************************************************************************/
  //        EVENT HANDLING                                                  /
  /*************************************************************************/

  auto screen = Screen::Create(Dimension::Fit(main_document));
  Render(screen, main_document);
  screen.Print();
  getchar();
  return EXIT_SUCCESS;
}
