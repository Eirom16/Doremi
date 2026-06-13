#include "settings_view.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include "doremi/src/bridge.rs.h"

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent)
{
    const auto &c = DesignTokens::current();

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto *inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    auto *content = new QVBoxLayout(inner);
    content->setContentsMargins(32, 24, 32, 24);
    content->setSpacing(10);

    auto *title = new QLabel("Configuración", inner);
    title->setFont(DesignTokens::getFont("display", 24));
    title->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.text_primary.name()));
    content->addWidget(title);
    content->addSpacing(8);

    // Style for comboboxes
    QString comboStyle = QString(
        "QComboBox {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 6px;\n"
        "    padding: 6px 32px 6px 12px;\n"
        "    color: %3;\n"
        "    font-size: 13px;\n"
        "}\n"
        "QComboBox:hover {\n"
        "    border-color: %4;\n"
        "}\n"
        "QComboBox::drop-down {\n"
        "    border: none;\n"
        "    width: 20px;\n"
        "}\n"
        "QComboBox QAbstractItemView {\n"
        "    background-color: %1;\n"
        "    color: %3;\n"
        "    selection-background-color: %5;\n"
        "    selection-color: #FFFFFF;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 6px;\n"
        "}\n"
    )
    .arg(c.bg_elevated.name())
    .arg(c.border.name())
    .arg(c.text_primary.name())
    .arg(c.accent.name())
    .arg(c.accent_dim.name());

    // Appearance section
    content->addWidget(section_header("Apariencia"));
    content->addSpacing(4);

    theme_cmb_ = new QComboBox(inner);
    theme_cmb_->addItems({"dark", "light"});
    theme_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Tema", theme_cmb_));

    accent_cmb_ = new QComboBox(inner);
    accent_cmb_->addItems({"#7C4DFF", "#A78BFA", "#22D3EE", "#F472B6", "#34D399"});
    accent_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Color de acento", accent_cmb_));

    font_cmb_ = new QComboBox(inner);
    font_cmb_->addItems({"12", "13", "14", "15", "16"});
    font_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Tamaño de fuente", font_cmb_));

    content->addSpacing(12);
    auto *sep = new QFrame(inner);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep);

    // Playback section
    content->addWidget(section_header("Reproducción"));
    content->addSpacing(4);

    normalize_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row("Normalizar audio", normalize_cb_));

    crossfade_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row("Crossfade entre canciones", crossfade_cb_));

    sleep_timer_cmb_ = new QComboBox(inner);
    sleep_timer_cmb_->addItems({"Desactivado", "5 minutos", "10 minutos", "15 minutos", "30 minutos", "45 minutos", "60 minutos"});
    sleep_timer_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Temporizador de apagado", sleep_timer_cmb_));

    content->addSpacing(12);
    auto *sep2 = new QFrame(inner);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep2);

    // Equalizer section
    content->addWidget(section_header("Ecualizador"));
    content->addSpacing(4);

    equalizer_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row("Activar ecualizador", equalizer_cb_));

    eq_preset_cmb_ = new QComboBox(inner);
    eq_preset_cmb_->addItems({"Flat", "Bass Boost", "Treble Boost", "Vocal", "Classical", "Electronic", "Hip-Hop", "Rock", "Jazz", "Pop"});
    eq_preset_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Preset", eq_preset_cmb_));

    content->addSpacing(12);
    auto *sep3 = new QFrame(inner);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep3);

    // Integrations section
    content->addWidget(section_header("Integraciones"));
    content->addSpacing(4);

    discord_rpc_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row("Activar Discord Rich Presence", discord_rpc_cb_));

    lastfm_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row("Activar Last.fm Scrobbler", lastfm_cb_));

    // Last.fm Auth container (styled as a premium card)
    lastfm_auth_widget_ = new QWidget(inner);
    auto *lf_layout = new QVBoxLayout(lastfm_auth_widget_);
    lf_layout->setContentsMargins(16, 16, 16, 16);
    lf_layout->setSpacing(12);
    
    lastfm_auth_widget_->setStyleSheet(QString(
        "QWidget {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 8px;\n"
        "}\n"
    )
    .arg(c.bg_surface.name())
    .arg(c.border.name()));

    lastfm_status_lbl_ = new QLabel("Estado: Desconectado", lastfm_auth_widget_);
    lastfm_status_lbl_->setFont(DesignTokens::getFont("body", 12));
    lastfm_status_lbl_->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent;").arg(c.text_secondary.name()));
    lf_layout->addWidget(lastfm_status_lbl_);

    QString input_style = QString(
        "QLineEdit {\n"
        "    background-color: %1;\n"
        "    border: 1px solid %2;\n"
        "    border-radius: 6px;\n"
        "    padding: 6px 12px;\n"
        "    color: %3;\n"
        "    font-size: 13px;\n"
        "}\n"
        "QLineEdit:focus {\n"
        "    border-color: %4;\n"
        "}\n"
    )
    .arg(c.bg_base.name())
    .arg(c.border.name())
    .arg(c.text_primary.name())
    .arg(c.accent.name());
    
    lastfm_api_key_input_ = new QLineEdit(lastfm_auth_widget_);
    lastfm_api_key_input_->setPlaceholderText("API Key");
    lastfm_api_key_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row("API Key", lastfm_api_key_input_));

    lastfm_api_secret_input_ = new QLineEdit(lastfm_auth_widget_);
    lastfm_api_secret_input_->setPlaceholderText("API Secret");
    lastfm_api_secret_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row("API Secret", lastfm_api_secret_input_));

    lastfm_username_input_ = new QLineEdit(lastfm_auth_widget_);
    lastfm_username_input_->setPlaceholderText("Usuario");
    lastfm_username_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row("Usuario", lastfm_username_input_));

    lastfm_password_input_ = new QLineEdit(lastfm_auth_widget_);
    lastfm_password_input_->setPlaceholderText("Contraseña");
    lastfm_password_input_->setEchoMode(QLineEdit::Password);
    lastfm_password_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row("Contraseña", lastfm_password_input_));

    // Connect button with dynamic variants
    lastfm_auth_btn_ = new RippleButton("Conectar Cuenta", lastfm_auth_widget_, RippleButton::Variant::Primary);
    lf_layout->addWidget(lastfm_auth_btn_);

    content->addWidget(lastfm_auth_widget_);
    lastfm_auth_widget_->setVisible(false);

    content->addSpacing(12);
    auto *sep_lf = new QFrame(inner);
    sep_lf->setFrameShape(QFrame::HLine);
    sep_lf->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_lf);

    // Language section
    content->addWidget(section_header("Idioma"));
    content->addSpacing(4);

    lang_cmb_ = new QComboBox(inner);
    lang_cmb_->addItems({"es", "en"});
    lang_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row("Idioma", lang_cmb_));

    content->addSpacing(12);
    auto *sep_about = new QFrame(inner);
    sep_about->setFrameShape(QFrame::HLine);
    sep_about->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_about);

    // Acerca de section
    content->addWidget(section_header("Acerca de"));
    content->addSpacing(8);

    auto *about_logo = IconProvider::createIconLabel("album", 64, c.accent, true, inner);
    about_logo->setAlignment(Qt::AlignCenter);
    content->addWidget(about_logo);

    auto *about_name = new QLabel("Doremi", inner);
    about_name->setFont(DesignTokens::getFont("display", 20));
    about_name->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent;").arg(c.accent.name()));
    about_name->setAlignment(Qt::AlignCenter);
    content->addWidget(about_name);

    auto *about_ver = new QLabel(QString("Versión %1").arg(QString::fromStdString(std::string(get_app_version()))), inner);
    about_ver->setFont(DesignTokens::getFont("body", 12));
    about_ver->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    about_ver->setAlignment(Qt::AlignCenter);
    content->addWidget(about_ver);

    auto *about_desc = new QLabel(
        "Cliente de escritorio premium para YouTube Music construido con Rust, C++ y Qt6.\n"
        "Soporta descargas offline, letras sincronizadas, ecualizador e integración con Discord y Last.fm.",
        inner
    );
    about_desc->setFont(DesignTokens::getFont("body", 13));
    about_desc->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    about_desc->setAlignment(Qt::AlignCenter);
    about_desc->setWordWrap(true);
    content->addWidget(about_desc);

    // Changelog card
    auto *changelog_card = new QFrame(inner);
    changelog_card->setStyleSheet(QString(
        "QFrame {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 12px;"
        "    padding: 16px;"
        "}"
    ).arg(c.bg_surface.name()).arg(c.border.name()));
    
    auto *cl_layout = new QVBoxLayout(changelog_card);
    cl_layout->setSpacing(10);

    auto *cl_title = new QLabel(QString::fromStdString(std::string(doremi_tr("changelog_title"))), changelog_card);
    cl_title->setFont(DesignTokens::getFont("body", 14));
    cl_title->setStyleSheet(QString("font-weight: bold; color: %1; background: transparent; border: none;").arg(c.text_primary.name()));
    cl_layout->addWidget(cl_title);

    // List changelog items
    struct ChangelogSection {
        std::string title;
        std::vector<std::string> items;
    };
    std::vector<ChangelogSection> changelog = {
        {std::string(doremi_tr("changelog_novedades")), {
            std::string(doremi_tr("changelog_new_1")),
            std::string(doremi_tr("changelog_new_2")),
            std::string(doremi_tr("changelog_new_3")),
            std::string(doremi_tr("changelog_new_4"))
        }},
        {std::string(doremi_tr("changelog_correcciones")), {
            std::string(doremi_tr("changelog_fix_1")),
            std::string(doremi_tr("changelog_fix_2"))
        }}
    };

    for (const auto &sec : changelog) {
        auto *sec_lbl = new QLabel(QString::fromStdString(sec.title), changelog_card);
        sec_lbl->setFont(DesignTokens::getFont("micro", 11));
        sec_lbl->setStyleSheet(QString("color: %1; font-weight: bold; background: transparent; border: none;").arg(c.accent.name()));
        cl_layout->addWidget(sec_lbl);

        for (const auto &item : sec.items) {
            auto *item_row = new QHBoxLayout();
            item_row->setContentsMargins(0, 0, 0, 0);
            item_row->setSpacing(8);

            auto *check_icon = IconProvider::createIconLabel("check_circle", 14, c.success, true, changelog_card);
            check_icon->setStyleSheet("background: transparent; border: none;");
            
            auto *item_lbl = new QLabel(QString::fromStdString(item), changelog_card);
            item_lbl->setFont(DesignTokens::getFont("body", 12));
            item_lbl->setStyleSheet(QString("color: %1; background: transparent; border: none;").arg(c.text_secondary.name()));
            item_lbl->setWordWrap(true);

            item_row->addWidget(check_icon, 0, Qt::AlignTop);
            item_row->addWidget(item_lbl, 1);
            cl_layout->addLayout(item_row);
        }
    }
    content->addWidget(changelog_card);

    // Update Check button
    auto *check_updates_btn = new RippleButton("Buscar actualizaciones", inner, RippleButton::Variant::Secondary);
    check_updates_btn->setIcon(IconProvider::getIcon("sync", c.accent, 20));
    check_updates_btn->setMinimumHeight(44);
    
    QObject::connect(check_updates_btn, &QPushButton::clicked, this, [check_updates_btn]() {
        check_updates_btn->setEnabled(false);
        check_updates_btn->setText("Comprobando...");
        
        // Call Rust check function
        on_check_for_updates_requested();

        // Restore button state after 4 seconds (fallback)
        QTimer::singleShot(4000, check_updates_btn, [check_updates_btn]() {
            check_updates_btn->setEnabled(true);
            check_updates_btn->setText("Buscar actualizaciones");
        });
    });
    content->addWidget(check_updates_btn);

    content->addStretch(1);
    scroll->setWidget(inner);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(scroll);
    setStyleSheet("background: transparent;");

    // Emit signals on changes
    QObject::connect(theme_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("theme_mode", v.toStdString());
    });
    QObject::connect(accent_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("accent_color", v.toStdString());
    });
    QObject::connect(font_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("font_size", v.toStdString());
    });
    QObject::connect(lang_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("language", v.toStdString());
    });
    QObject::connect(normalize_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("normalize_audio", v ? "true" : "false");
    });
    QObject::connect(crossfade_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("crossfade_enabled", v ? "true" : "false");
    });
    QObject::connect(sleep_timer_cmb_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int minutes = 0;
        if (idx == 1) minutes = 5;
        else if (idx == 2) minutes = 10;
        else if (idx == 3) minutes = 15;
        else if (idx == 4) minutes = 30;
        else if (idx == 5) minutes = 45;
        else if (idx == 6) minutes = 60;
        emit setting_changed("sleep_timer", std::to_string(minutes));
    });
    QObject::connect(equalizer_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("equalizer_enabled", v ? "true" : "false");
    });
    QObject::connect(eq_preset_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("equalizer_preset", v.toStdString());
    });
    QObject::connect(discord_rpc_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("discord_rpc_enabled", v ? "true" : "false");
    });
    QObject::connect(lastfm_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        lastfm_auth_widget_->setVisible(v);
        emit setting_changed("lastfm_enabled", v ? "true" : "false");
    });
    QObject::connect(lastfm_auth_btn_, &QPushButton::clicked, this, [this]() {
        if (lastfm_auth_btn_->text() == "Conectar Cuenta") {
            std::string apiKey = lastfm_api_key_input_->text().toStdString();
            std::string apiSecret = lastfm_api_secret_input_->text().toStdString();
            std::string username = lastfm_username_input_->text().toStdString();
            std::string password = lastfm_password_input_->text().toStdString();
            emit lastfm_auth_requested(apiKey, apiSecret, username, password);
        } else {
            emit lastfm_disconnect_requested();
        }
    });
}

QWidget *SettingsView::section_header(const std::string &title) {
    const auto &c = DesignTokens::current();
    auto *w = new QWidget(this);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 12, 0, 0);
    
    auto *label = new QLabel(QString::fromStdString(title), w);
    label->setFont(DesignTokens::getFont("micro", 11));
    label->setStyleSheet(QString("color: %1; text-transform: uppercase; background: transparent;").arg(c.accent.name()));
    
    l->addWidget(label);
    l->addStretch(1);
    return w;
}

QWidget *SettingsView::combo_row(const std::string &label, QComboBox *cmb) {
    const auto &c = DesignTokens::current();
    auto *w = new QWidget(this);
    w->setFixedHeight(36);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    
    auto *lb = new QLabel(QString::fromStdString(label), w);
    lb->setFont(DesignTokens::getFont("body", 13));
    lb->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    
    l->addWidget(lb);
    l->addStretch(1);
    l->addWidget(cmb);
    return w;
}

QWidget *SettingsView::check_row(const std::string &label, AnimatedToggle *cb) {
    const auto &c = DesignTokens::current();
    auto *w = new QWidget(this);
    w->setFixedHeight(36);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    
    auto *lb = new QLabel(QString::fromStdString(label), w);
    lb->setFont(DesignTokens::getFont("body", 13));
    lb->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    
    l->addWidget(lb);
    l->addStretch(1);
    l->addWidget(cb);
    return w;
}

QWidget *SettingsView::input_row(const std::string &label, QLineEdit *input) {
    const auto &c = DesignTokens::current();
    auto *w = new QWidget(this);
    w->setFixedHeight(36);
    w->setStyleSheet("background: transparent;");
    
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    
    auto *lb = new QLabel(QString::fromStdString(label), w);
    lb->setFont(DesignTokens::getFont("body", 12));
    lb->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    
    l->addWidget(lb);
    l->addStretch(1);
    
    input->setFixedWidth(250);
    l->addWidget(input);
    return w;
}

// -- getters/setters --

void SettingsView::set_theme(const std::string &mode) {
    theme_cmb_->blockSignals(true);
    int idx = theme_cmb_->findText(QString::fromStdString(mode));
    if (idx >= 0) theme_cmb_->setCurrentIndex(idx);
    theme_cmb_->blockSignals(false);
}
void SettingsView::set_accent(const std::string &color) {
    accent_cmb_->blockSignals(true);
    int idx = accent_cmb_->findText(QString::fromStdString(color));
    if (idx >= 0) accent_cmb_->setCurrentIndex(idx);
    accent_cmb_->blockSignals(false);
}
void SettingsView::set_font_size(int32_t size) {
    font_cmb_->blockSignals(true);
    int idx = font_cmb_->findText(QString::number(size));
    if (idx >= 0) font_cmb_->setCurrentIndex(idx);
    font_cmb_->blockSignals(false);
}
void SettingsView::set_language(const std::string &lang) {
    lang_cmb_->blockSignals(true);
    int idx = lang_cmb_->findText(QString::fromStdString(lang));
    if (idx >= 0) lang_cmb_->setCurrentIndex(idx);
    lang_cmb_->blockSignals(false);
}
void SettingsView::set_normalize(bool on) {
    normalize_cb_->blockSignals(true);
    normalize_cb_->setChecked(on);
    normalize_cb_->blockSignals(false);
}
void SettingsView::set_crossfade(bool on) {
    crossfade_cb_->blockSignals(true);
    crossfade_cb_->setChecked(on);
    crossfade_cb_->blockSignals(false);
}
void SettingsView::set_equalizer_enabled(bool on) {
    equalizer_cb_->blockSignals(true);
    equalizer_cb_->setChecked(on);
    equalizer_cb_->blockSignals(false);
}
void SettingsView::set_equalizer_preset(const std::string &preset) {
    eq_preset_cmb_->blockSignals(true);
    int idx = eq_preset_cmb_->findText(QString::fromStdString(preset));
    if (idx >= 0) eq_preset_cmb_->setCurrentIndex(idx);
    eq_preset_cmb_->blockSignals(false);
}
void SettingsView::set_sleep_timer(int32_t minutes) {
    sleep_timer_cmb_->blockSignals(true);
    int idx = 0; // Desactivado
    if (minutes == 5) idx = 1;
    else if (minutes == 10) idx = 2;
    else if (minutes == 15) idx = 3;
    else if (minutes == 30) idx = 4;
    else if (minutes == 45) idx = 5;
    else if (minutes == 60) idx = 6;
    sleep_timer_cmb_->setCurrentIndex(idx);
    sleep_timer_cmb_->blockSignals(false);
}

std::string SettingsView::theme() const { return theme_cmb_->currentText().toStdString(); }
std::string SettingsView::accent() const { return accent_cmb_->currentText().toStdString(); }
int32_t SettingsView::font_size() const { return font_cmb_->currentText().toInt(); }
std::string SettingsView::language() const { return lang_cmb_->currentText().toStdString(); }
bool SettingsView::normalize() const { return normalize_cb_->isChecked(); }
bool SettingsView::crossfade() const { return crossfade_cb_->isChecked(); }
bool SettingsView::equalizer_enabled() const { return equalizer_cb_->isChecked(); }
std::string SettingsView::equalizer_preset() const { return eq_preset_cmb_->currentText().toStdString(); }
int32_t SettingsView::sleep_timer() const {
    int idx = sleep_timer_cmb_->currentIndex();
    if (idx == 1) return 5;
    if (idx == 2) return 10;
    if (idx == 3) return 15;
    if (idx == 4) return 30;
    if (idx == 5) return 45;
    if (idx == 6) return 60;
    return 0;
}

void SettingsView::set_settings_discord_rpc(bool on) {
    discord_rpc_cb_->blockSignals(true);
    discord_rpc_cb_->setChecked(on);
    discord_rpc_cb_->blockSignals(false);
}

void SettingsView::set_settings_lastfm_enabled(bool on) {
    lastfm_cb_->blockSignals(true);
    lastfm_cb_->setChecked(on);
    lastfm_auth_widget_->setVisible(on);
    lastfm_cb_->blockSignals(false);
}

void SettingsView::set_settings_lastfm_session(bool authenticated, const std::string &username, const std::string &apiKey, const std::string &apiSecret) {
    if (authenticated) {
        lastfm_status_lbl_->setText(QString::fromStdString("Estado: Conectado como " + username));
        lastfm_api_key_input_->setText(QString::fromStdString(apiKey));
        lastfm_api_secret_input_->setText(QString::fromStdString(apiSecret));
        lastfm_api_key_input_->setEnabled(false);
        lastfm_api_secret_input_->setEnabled(false);
        lastfm_username_input_->setVisible(false);
        lastfm_password_input_->setVisible(false);
        lastfm_auth_btn_->setText("Desconectar Cuenta");
        lastfm_auth_btn_->setVariant(RippleButton::Variant::Danger);
    } else {
        lastfm_status_lbl_->setText("Estado: Desconectado");
        lastfm_api_key_input_->setText("");
        lastfm_api_secret_input_->setText("");
        lastfm_username_input_->setText("");
        lastfm_password_input_->setText("");
        lastfm_api_key_input_->setEnabled(true);
        lastfm_api_secret_input_->setEnabled(true);
        lastfm_username_input_->setVisible(true);
        lastfm_password_input_->setVisible(true);
        lastfm_auth_btn_->setText("Conectar Cuenta");
        lastfm_auth_btn_->setVariant(RippleButton::Variant::Primary);
    }
}

bool SettingsView::discord_rpc_enabled() const { return discord_rpc_cb_->isChecked(); }
bool SettingsView::lastfm_enabled() const { return lastfm_cb_->isChecked(); }
