#ifndef THEME_H
#define THEME_H

#include <SFML/Graphics.hpp>

// ==================================================================
// Theme
// ------------------------------------------------------------------
// Centralized color palette so the whole app reads as one designed
// product instead of "every button its own random color". Loosely
// inspired by Instagram's neutral-with-one-accent look: mostly
// black/white/gray, one blue accent for actions, one red-pink for
// "liked" state. No DSA content here -- pure visual consistency.
// ==================================================================
namespace Theme
{
    // Core neutrals
    const sf::Color background   = sf::Color(250, 250, 250);
    const sf::Color headerBg     = sf::Color(24, 24, 27);      // near-black, not steel blue
    const sf::Color cardBg       = sf::Color::White;
    const sf::Color cardBorder   = sf::Color(219, 219, 219);
    const sf::Color panelBg      = sf::Color::White;
    const sf::Color panelBorder  = sf::Color(224, 224, 224);

    // Text
    const sf::Color textPrimary  = sf::Color(38, 38, 38);
    const sf::Color textMuted    = sf::Color(142, 142, 142);
    const sf::Color textOnDark   = sf::Color(250, 250, 250);

    // Accent (single blue used everywhere for primary actions)
    const sf::Color accent       = sf::Color(24, 119, 242);
    const sf::Color accentHover  = sf::Color(60, 145, 255);

    // Semantic
    const sf::Color liked        = sf::Color(237, 73, 86);     // heart red
    const sf::Color unliked      = sf::Color(230, 230, 230);   // light neutral
    const sf::Color danger       = sf::Color(220, 53, 69);
    const sf::Color success      = sf::Color(56, 142, 60);

    // Card state tints (soft, not neon)
    const sf::Color searchTint   = sf::Color(255, 247, 214);   // soft amber
    const sf::Color likedTint    = sf::Color(253, 235, 236);   // soft rose

    // Neutral dark button (secondary actions: Home/Help/About/Logout)
    const sf::Color neutralDark  = sf::Color(64, 64, 68);
}

#endif
