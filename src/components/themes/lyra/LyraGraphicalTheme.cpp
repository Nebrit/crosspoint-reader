#include "LyraGraphicalTheme.h"

#include <algorithm>
#include <cctype>

#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/book24.h"
#include "components/icons/cover.h"
#include "components/icons/file24.h"
#include "components/icons/folder24.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "fontIds.h"

namespace {

// Grid layout
constexpr int GRID_COLS = 2;
constexpr int TILE_HEIGHT = 120;
constexpr int TILE_GAP = 12;
constexpr int ICON_SIZE = 32;

// Card layout
constexpr int CARD_RADIUS = 10;
constexpr int CARD_PADDING = 12;

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

} // namespace

std::string LyraGraphicalTheme::basename(const std::string& path) {
  auto pos = path.find_last_of('/');
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string LyraGraphicalTheme::fileExtLower(const std::string& path) {
  auto pos = path.find_last_of('.');
  if (pos == std::string::npos) return {};
  return toLower(path.substr(pos + 1));
}

/**
 * HOME MENU — grid de categorías con iconos grandes
 */
void LyraGraphicalTheme::drawButtonMenu(
    GfxRenderer& renderer,
    Rect rect,
    int buttonCount,
    int selectedIndex,
    const std::function<std::string(int)>& buttonLabel,
    const std::function<UIIcon(int)>& rowIcon) const {

  const int sidePad = LyraMetrics::values.contentSidePadding;
  const int x0 = rect.x + sidePad;
  const int w0 = rect.width - sidePad * 2;

  const int tileWidth = (w0 - TILE_GAP) / GRID_COLS;

  for (int i = 0; i < buttonCount; ++i) {
    const int row = i / GRID_COLS;
    const int col = i % GRID_COLS;

    const int x = x0 + col * (tileWidth + TILE_GAP);
    const int y = rect.y + row * (TILE_HEIGHT + TILE_GAP);

    const bool selected = (i == selectedIndex);

    // Tile background
    if (selected) {
      renderer.fillRoundedRect(x, y, tileWidth, TILE_HEIGHT, CARD_RADIUS, Color::LightGray);
    } else {
      renderer.fillRoundedRect(x, y, tileWidth, TILE_HEIGHT, CARD_RADIUS, Color::White);
    }
    renderer.drawRoundedRect(x, y, tileWidth, TILE_HEIGHT, 1, CARD_RADIUS, true);

    // Icon
    const UIIcon icon = rowIcon ? rowIcon(i) : UIIcon::File;
    const uint8_t* bmp = UITheme::getIcon(icon, ICON_SIZE);

    if (bmp) {
      renderer.drawIcon(
          bmp,
          x + (tileWidth - ICON_SIZE) / 2,
          y + CARD_PADDING,
          ICON_SIZE,
          ICON_SIZE);
    }

    // Label
    const std::string label = buttonLabel ? buttonLabel(i) : "";
    const int textWidth =
        renderer.getTextWidth(UI_12_FONT_ID, label.c_str(), EpdFontFamily::REGULAR);

    renderer.drawText(
        UI_12_FONT_ID,
        x + (tileWidth - textWidth) / 2,
        y + CARD_PADDING + ICON_SIZE + 8,
        label.c_str(),
        true);
  }
}

/**
 * LIBRO ACTIVO — card “Continue reading” con resumen
 */
void LyraGraphicalTheme::drawRecentBookCover(
    GfxRenderer& renderer,
    Rect rect,
    const std::vector<RecentBook>& recentBooks,
    const int selectorIndex,
    bool& coverRendered,
    bool& coverBufferStored,
    bool& bufferRestored,
    std::function<bool()> storeCoverBuffer) const {

  const int sidePad = LyraMetrics::values.contentSidePadding;
  const int x = rect.x + sidePad;
  const int y = rect.y;
  const int w = rect.width - sidePad * 2;
  const int h = rect.height;

  const bool selected = (selectorIndex == 0);

  // Card background
  if (selected) {
    renderer.fillRoundedRect(x, y, w, h, CARD_RADIUS, Color::LightGray);
  } else {
    renderer.fillRoundedRect(x, y, w, h, CARD_RADIUS, Color::White);
  }
  renderer.drawRoundedRect(x, y, w, h, 1, CARD_RADIUS, true);

  // Header
  renderer.drawText(UI_10_FONT_ID, x + 12, y + 8, tr(STR_CONTINUE_READING), true);

  if (recentBooks.empty()) {
    renderer.drawText(
        UI_12_FONT_ID,
        x + 12,
        y + h / 2,
        tr(STR_NO_OPEN_BOOK),
        true,
        EpdFontFamily::BOLD);
    return;
  }

  const RecentBook& book = recentBooks.front();

  // Cover
  const int coverH = LyraMetrics::values.homeCoverHeight;
  const int coverW = static_cast<int>(coverH * 0.65f);
  const int coverX = x + 12;
  const int coverY = y + 28;

  if (!coverRendered) {
    UITheme::drawCover(
        renderer,
        book.coverBmpPath,
        coverX,
        coverY,
        coverW,
        coverH);
    coverBufferStored = storeCoverBuffer();
    coverRendered = true;
  }

  // Text area
  const int textX = coverX + coverW + 16;
  int textY = coverY + 8;
  const int textW = x + w - textX - 12;

  // Title
  renderer.drawText(
      UI_12_FONT_ID,
      textX,
      textY,
      renderer.truncatedText(
          UI_12_FONT_ID,
          book.title.c_str(),
          textW,
          EpdFontFamily::BOLD)
          .c_str(),
      true,
      EpdFontFamily::BOLD);
  textY += renderer.getLineHeight(UI_12_FONT_ID) + 4;

  // Author
  if (!book.author.empty()) {
    renderer.drawText(
        UI_10_FONT_ID,
        textX,
        textY,
        renderer.truncatedText(
            UI_10_FONT_ID,
            book.author.c_str(),
            textW,
            EpdFontFamily::REGULAR)
            .c_str(),
        true);
    textY += renderer.getLineHeight(UI_10_FONT_ID) + 6;
  }

  // Meta line
  const std::string meta =
      "." + fileExtLower(book.path) + " · " + basename(book.path);

  renderer.drawText(
      SMALL_FONT_ID,
      textX,
      textY,
      renderer.truncatedText(
          SMALL_FONT_ID,
          meta.c_str(),
          textW,
          EpdFontFamily::REGULAR)
          .c_str(),
      true);
}
