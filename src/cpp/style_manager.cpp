#include "style_manager.h"
#include "design_tokens.h"
#include <QApplication>
#include <QFile>
#include <QDebug>

QString StyleManager::getStyleSheet() {
    QFile file(":/assets/styles/base.qss");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "StyleManager: could not open base.qss from resources";
        return {};
    }
    QString qss = QString::fromUtf8(file.readAll());
    file.close();
    return resolvePlaceholders(qss);
}

void StyleManager::applyTheme() {
    qApp->setStyleSheet(getStyleSheet());
}

QString StyleManager::resolvePlaceholders(const QString &qss) {
    const auto &c = DesignTokens::current();
    const auto &r = DesignTokens::radius();
    const auto &s = DesignTokens::spacing();

    QString result = qss;

    // Colors
    result.replace("{{accent}}", c.accent.name());
    result.replace("{{accent_bright}}", c.accent_bright.name());
    result.replace("{{accent_dim}}", DesignTokens::rgba(c.accent_dim));
    result.replace("{{accent_glow}}", DesignTokens::rgba(c.accent_glow));
    result.replace("{{text_primary}}", c.text_primary.name());
    result.replace("{{text_on_accent}}", c.text_on_accent.name());
    result.replace("{{text_secondary}}", c.text_secondary.name());
    result.replace("{{text_muted}}", c.text_muted.name());
    result.replace("{{bg_base}}", c.bg_base.name());
    result.replace("{{bg_surface}}", c.bg_surface.name());
    result.replace("{{bg_elevated}}", c.bg_elevated.name());
    result.replace("{{bg_overlay}}", DesignTokens::rgba(c.bg_overlay));
    result.replace("{{border}}", DesignTokens::rgba(c.border));
    result.replace("{{border_accent}}", DesignTokens::rgba(c.border_accent));
    result.replace("{{surface}}", c.surface.name());
    result.replace("{{surface_raised}}", c.surface_raised.name());
    result.replace("{{surface_selected}}", DesignTokens::rgba(c.surface_selected));
    result.replace("{{success}}", c.success.name());
    result.replace("{{warning}}", c.warning.name());
    result.replace("{{error}}", c.error.name());
    result.replace("{{type_album}}", c.type_album.name());
    result.replace("{{type_playlist}}", c.type_playlist.name());
    result.replace("{{type_mix}}", c.type_mix.name());
    result.replace("{{type_podcast}}", c.type_podcast.name());

    // Radii
    result.replace("{{r-xs}}", QString::number(r.xs));
    result.replace("{{r-sm}}", QString::number(r.sm));
    result.replace("{{r-md}}", QString::number(r.md));
    result.replace("{{r-lg}}", QString::number(r.lg));
    result.replace("{{r-xl}}", QString::number(r.xl));
    result.replace("{{r-pill}}", QString::number(r.pill));

    // Spacings
    result.replace("{{sp-xs}}", QString::number(s.xs));
    result.replace("{{sp-sm}}", QString::number(s.sm));
    result.replace("{{sp-md}}", QString::number(s.md));
    result.replace("{{sp-lg}}", QString::number(s.lg));
    result.replace("{{sp-xl}}", QString::number(s.xl));
    result.replace("{{sp-xxl}}", QString::number(s.xxl));

    return result;
}
