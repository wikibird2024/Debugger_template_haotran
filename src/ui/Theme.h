#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QString>

/* Light and dark from one sheet with @token colors. Every color in the tool
 * comes from here; tests/check_no_hard_coded_colors.sh fails on any other. */
namespace Theme
{

void    apply(bool isDark);
bool    isDark();
QColor  color(const char *token); /* a wrong name gives magenta, so it shows at once */
QString sheet(bool isDark);       /* the sheet with every @token filled in */

} // namespace Theme

#endif // THEME_H
