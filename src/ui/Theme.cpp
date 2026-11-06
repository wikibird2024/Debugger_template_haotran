#include "ui/Theme.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <algorithm>
#include <vector>

/* Must sit outside a namespace: this library is static, so the sheet has to be
   registered by hand. */
static void loadThemeResource()
{
    Q_INIT_RESOURCE(theme);
}

namespace Theme
{

namespace
{

struct Token {
    const char *name;
    const char *light;
    const char *dark;
};

/* Palette from the plasma debugger, which took its light colors from Dekit Grader v2. */
const Token kTokens[] = {
    {"bg", "#F2F3F5", "#16191D"},
    {"surface", "#FFFFFF", "#1F2328"},
    {"surface-alt", "#E8EAED", "#2A2F35"},
    {"sidebar", "#E9EBEE", "#1B1F23"},
    {"hover", "#DCE0E3", "#343A41"},
    {"border", "#C7CCD1", "#3A4047"},
    {"text", "#1F2429", "#E6E8EB"},
    {"text-muted", "#5F6870", "#9AA3AB"},
    {"text-disabled", "#A6ACB1", "#5E666E"},
    {"accent", "#1A72BB", "#4A9BE0"},
    {"accent-soft", "#D6E7F5", "#1E3A52"},
    {"accent-text", "#0F5A94", "#9CCBF2"},
    {"input", "#FFFFFF", "#1A1D21"},
    {"read-only", "#F7F8F9", "#1C2024"},
    {"tooltip-bg", "#1F2429", "#E6E8EB"},
    {"tooltip-text", "#FFFFFF", "#1F2429"},
    {"danger", "#D0342C", "#D0342C"},
    {"danger-hover", "#B02A23", "#E04A42"},
    {"state-on", "#2E9E4F", "#46B868"},
    {"state-look", "#D9A400", "#E6B91E"},
    {"state-alarm", "#D0342C", "#E04A42"},
    {"state-off", "#757575", "#7A828A"},
};

bool s_isDark = false;

const char *valueOf(const Token &token, bool isDark)
{
    return isDark ? token.dark : token.light;
}

void applyPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, color("bg"));
    palette.setColor(QPalette::WindowText, color("text"));
    palette.setColor(QPalette::Base, color("input"));
    palette.setColor(QPalette::AlternateBase, color("read-only"));
    palette.setColor(QPalette::Text, color("text"));
    palette.setColor(QPalette::Button, color("surface-alt"));
    palette.setColor(QPalette::ButtonText, color("text"));
    palette.setColor(QPalette::Highlight, color("accent"));
    palette.setColor(QPalette::HighlightedText, color("surface"));
    palette.setColor(QPalette::ToolTipBase, color("tooltip-bg"));
    palette.setColor(QPalette::ToolTipText, color("tooltip-text"));
    palette.setColor(QPalette::PlaceholderText, color("text-disabled"));
    for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, color("text-disabled"));
    }
    QApplication::setPalette(palette);
}

} // namespace

QColor color(const char *token)
{
    for (const Token &entry : kTokens) {
        if (qstrcmp(entry.name, token) == 0) {
            return QColor(valueOf(entry, s_isDark));
        }
    }
    return QColor(Qt::magenta);
}

bool isDark()
{
    return s_isDark;
}

QString sheet(bool isDark)
{
    loadThemeResource();
    QFile file(":/theme.qss");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("Cannot open the theme sheet");
        return {};
    }
    QString text = QString::fromUtf8(file.readAll());

    /* Longest name first: "@text-muted" must not be cut by "@text". */
    std::vector<const Token *> tokens;
    for (const Token &entry : kTokens) {
        tokens.push_back(&entry);
    }
    std::sort(tokens.begin(), tokens.end(), [](const Token *first, const Token *second) {
        return qstrlen(first->name) > qstrlen(second->name);
    });
    for (const Token *entry : tokens) {
        text.replace(QString("@") + entry->name, valueOf(*entry, isDark));
    }
    return text;
}

void apply(bool isDark)
{
    s_isDark = isDark;
    applyPalette();
    qApp->setStyleSheet(sheet(isDark));
}

} // namespace Theme
