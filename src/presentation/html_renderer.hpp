#pragma once

#include <string>

namespace bluray::presentation {

/**
 * Handles HTML generation for the Single Page Application (SPA)
 */
class HtmlRenderer {
public:
  HtmlRenderer() = default;

  /**
   * Render the complete HTML for the SPA
   */
  std::string renderSPA();

private:
  // Helper methods to break down the massive HTML structure
  std::string renderHead();
  std::string renderStyles();
  std::string renderScripts();
  std::string renderSidebar();
  std::string renderMainContent();
  std::string renderModals();
};

} // namespace bluray::presentation
