#include "settings_view.h"
#include "ffi_utils.h"
#include "design_tokens.h"
#include "icon_provider.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <algorithm>
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

    auto *title = new QLabel(QString::fromStdString(std::string(doremi_tr("settings_title"))), inner);
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
    content->addWidget(section_header(std::string(doremi_tr("settings_appearance"))));
    content->addSpacing(4);

    theme_cmb_ = new QComboBox(inner);
    theme_cmb_->addItems({"dark", "light"});
    theme_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("theme")), theme_cmb_));

    accent_cmb_ = new QComboBox(inner);
    accent_cmb_->addItems({"#7C4DFF", "#A78BFA", "#22D3EE", "#F472B6", "#34D399"});
    accent_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("accent_color")), accent_cmb_));

    font_cmb_ = new QComboBox(inner);
    font_cmb_->addItems({"12", "13", "14", "15", "16"});
    font_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("font_size")), font_cmb_));

    content->addSpacing(12);
    auto *sep = new QFrame(inner);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep);

    // Playback section
    content->addWidget(section_header(std::string(doremi_tr("settings_playback"))));
    content->addSpacing(4);

    normalize_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("normalize_audio")), normalize_cb_));

    crossfade_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("crossfade_songs")), crossfade_cb_));

    stop_on_close_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("stop_on_close")), stop_on_close_cb_));

    sleep_timer_cmb_ = new QComboBox(inner);
    sleep_timer_cmb_->addItems({
        QString::fromStdString(std::string(doremi_tr("sleep_disabled"))),
        QString::fromStdString(std::string(doremi_tr("minutes_5"))),
        QString::fromStdString(std::string(doremi_tr("minutes_10"))),
        QString::fromStdString(std::string(doremi_tr("minutes_15"))),
        QString::fromStdString(std::string(doremi_tr("minutes_30"))),
        QString::fromStdString(std::string(doremi_tr("minutes_45"))),
        QString::fromStdString(std::string(doremi_tr("minutes_60")))
    });
    sleep_timer_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("sleep_timer")), sleep_timer_cmb_));

    content->addSpacing(12);
    auto *sep2 = new QFrame(inner);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep2);

    // Equalizer section
    content->addWidget(section_header(std::string(doremi_tr("settings_equalizer"))));
    content->addSpacing(4);

    equalizer_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("enable_equalizer")), equalizer_cb_));

    eq_preset_cmb_ = new QComboBox(inner);
    eq_preset_cmb_->addItems({"Flat", "Bass Boost", "Treble Boost", "Vocal", "Classical", "Electronic", "Hip-Hop", "Rock", "Jazz", "Pop", "Custom"});
    eq_preset_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("preset")), eq_preset_cmb_));

    auto *preamp_row = new QWidget(inner);
    auto *preamp_layout = new QHBoxLayout(preamp_row);
    preamp_layout->setContentsMargins(12, 4, 12, 4);
    auto *preamp_label = new QLabel("Preamp", preamp_row);
    eq_preamp_slider_ = new QSlider(Qt::Horizontal, preamp_row);
    eq_preamp_slider_->setRange(-200, 200);
    eq_preamp_slider_->setValue(0);
    eq_preamp_value_ = new QLabel("0.0 dB", preamp_row);
    eq_preamp_value_->setMinimumWidth(58);
    preamp_layout->addWidget(preamp_label);
    preamp_layout->addWidget(eq_preamp_slider_, 1);
    preamp_layout->addWidget(eq_preamp_value_);
    content->addWidget(preamp_row);

    auto *bands_widget = new QWidget(inner);
    auto *bands_layout = new QGridLayout(bands_widget);
    bands_layout->setContentsMargins(12, 4, 12, 4);
    bands_layout->setHorizontalSpacing(8);
    static const char *band_labels[] = {"60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k"};
    for (int i = 0; i < 10; ++i) {
        auto *label = new QLabel(band_labels[i], bands_widget);
        label->setAlignment(Qt::AlignCenter);
        auto *slider = new QSlider(Qt::Vertical, bands_widget);
        slider->setRange(-200, 200);
        slider->setValue(0);
        slider->setMinimumHeight(110);
        auto *value = new QLabel("0.0", bands_widget);
        value->setAlignment(Qt::AlignCenter);
        bands_layout->addWidget(label, 0, i);
        bands_layout->addWidget(slider, 1, i, Qt::AlignHCenter);
        bands_layout->addWidget(value, 2, i);
        eq_band_sliders_.push_back(slider);
        eq_band_values_.push_back(value);
    }
    content->addWidget(bands_widget);

    eq_reset_btn_ = new QPushButton("Reset Flat", inner);
    eq_reset_btn_->setCursor(Qt::PointingHandCursor);
    content->addWidget(eq_reset_btn_, 0, Qt::AlignRight);

    content->addSpacing(12);
    auto *sep3 = new QFrame(inner);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep3);

    // Integrations section
    content->addWidget(section_header(std::string(doremi_tr("settings_integrations"))));
    content->addSpacing(4);

    discord_rpc_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("activate_discord_rpc")), discord_rpc_cb_));

    lastfm_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("activate_lastfm")), lastfm_cb_));

    mpris_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("activate_mpris")), mpris_cb_));

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

    lastfm_status_lbl_ = new QLabel(QString::fromStdString(std::string(doremi_tr("lastfm_status_disconnected"))), lastfm_auth_widget_);
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
    lastfm_username_input_->setPlaceholderText(QString::fromStdString(std::string(doremi_tr("username"))));
    lastfm_username_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row(std::string(doremi_tr("username")), lastfm_username_input_));

    lastfm_password_input_ = new QLineEdit(lastfm_auth_widget_);
    lastfm_password_input_->setPlaceholderText(QString::fromStdString(std::string(doremi_tr("password"))));
    lastfm_password_input_->setEchoMode(QLineEdit::Password);
    lastfm_password_input_->setStyleSheet(input_style);
    lf_layout->addWidget(input_row(std::string(doremi_tr("password")), lastfm_password_input_));

    // Connect button with dynamic variants
    lastfm_auth_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("lastfm_btn_connect"))), lastfm_auth_widget_, RippleButton::Variant::Primary);
    lf_layout->addWidget(lastfm_auth_btn_);

    content->addWidget(lastfm_auth_widget_);
    lastfm_auth_widget_->setVisible(false);

    content->addSpacing(12);
    auto *sep_lf = new QFrame(inner);
    sep_lf->setFrameShape(QFrame::HLine);
    sep_lf->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_lf);

    // Subtitles section
    content->addWidget(section_header(std::string(doremi_tr("settings_subtitles"))));
    content->addSpacing(4);

    sub_alignment_cmb_ = new QComboBox(inner);
    sub_alignment_cmb_->addItems({
        QString::fromStdString(std::string(doremi_tr("align_left"))),
        QString::fromStdString(std::string(doremi_tr("align_center"))),
        QString::fromStdString(std::string(doremi_tr("align_right")))
    });
    sub_alignment_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("subtitle_alignment")), sub_alignment_cmb_));

    sub_font_size_cmb_ = new QComboBox(inner);
    for (int size = 10; size <= 36; size += 2) {
        sub_font_size_cmb_->addItem(QString::number(size) + " px", size);
    }
    sub_font_size_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("subtitle_font_size")), sub_font_size_cmb_));

    sub_line_spacing_cmb_ = new QComboBox(inner);
    sub_line_spacing_cmb_->addItems({"1.0", "1.2", "1.5", "1.8", "2.0"});
    sub_line_spacing_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("subtitle_line_spacing")), sub_line_spacing_cmb_));

    sub_auto_scroll_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("subtitle_auto_scroll")), sub_auto_scroll_cb_));

    sub_glow_cb_ = new AnimatedToggle(inner);
    content->addWidget(check_row(std::string(doremi_tr("subtitle_glow_effect")), sub_glow_cb_));

    content->addSpacing(12);
    auto *sep_sub = new QFrame(inner);
    sep_sub->setFrameShape(QFrame::HLine);
    sep_sub->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_sub);

    // Storage section
    content->addWidget(section_header(std::string(doremi_tr("settings_storage"))));
    content->addSpacing(4);

    db_size_lbl_ = new QLabel("0.00 MB", inner);
    db_size_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    db_size_lbl_->setFont(DesignTokens::getFont("body", 13));
    auto *db_widget = new QWidget(inner);
    db_widget->setFixedHeight(36);
    auto *db_lay = new QHBoxLayout(db_widget);
    db_lay->setContentsMargins(0, 0, 0, 0);
    auto *db_title_lbl = new QLabel(QString::fromStdString(std::string(doremi_tr("db_size"))), db_widget);
    db_title_lbl->setFont(DesignTokens::getFont("body", 13));
    db_title_lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    db_lay->addWidget(db_title_lbl);
    db_lay->addStretch(1);
    db_lay->addWidget(db_size_lbl_);
    content->addWidget(db_widget);

    cache_size_lbl_ = new QLabel("0.00 MB", inner);
    cache_size_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    cache_size_lbl_->setFont(DesignTokens::getFont("body", 13));
    clean_cache_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("clean_cache"))), inner, RippleButton::Variant::Secondary);
    clean_cache_btn_->setFixedWidth(140);
    clean_cache_btn_->setFixedHeight(28);
    content->addWidget(storage_row(std::string(doremi_tr("cache_size")), cache_size_lbl_, clean_cache_btn_));

    downloads_size_lbl_ = new QLabel("0.00 MB", inner);
    downloads_size_lbl_->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    downloads_size_lbl_->setFont(DesignTokens::getFont("body", 13));
    clear_downloads_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("clear_downloads"))), inner, RippleButton::Variant::Danger);
    clear_downloads_btn_->setFixedWidth(140);
    clear_downloads_btn_->setFixedHeight(28);
    content->addWidget(storage_row(std::string(doremi_tr("downloads_size")), downloads_size_lbl_, clear_downloads_btn_));

    // Backup actions row
    auto *backup_widget = new QWidget(inner);
    auto *backup_lay = new QHBoxLayout(backup_widget);
    backup_lay->setContentsMargins(0, 8, 0, 0);
    backup_lay->setSpacing(12);

    export_backup_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("export_backup"))), backup_widget, RippleButton::Variant::Primary);
    export_backup_btn_->setIcon(IconProvider::getIcon("save", QColor("#FFFFFF"), 16));
    export_backup_btn_->setFixedHeight(36);
    backup_lay->addWidget(export_backup_btn_, 1);

    import_backup_btn_ = new RippleButton(QString::fromStdString(std::string(doremi_tr("import_backup"))), backup_widget, RippleButton::Variant::Secondary);
    import_backup_btn_->setIcon(IconProvider::getIcon("open_in_new", c.accent, 16));
    import_backup_btn_->setFixedHeight(36);
    backup_lay->addWidget(import_backup_btn_, 1);
    content->addWidget(backup_widget);

    content->addSpacing(12);
    auto *sep_stor = new QFrame(inner);
    sep_stor->setFrameShape(QFrame::HLine);
    sep_stor->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_stor);

    // Language section
    content->addWidget(section_header(std::string(doremi_tr("language"))));
    content->addSpacing(4);

    lang_cmb_ = new QComboBox(inner);
    lang_cmb_->addItems({"es", "en"});
    lang_cmb_->setStyleSheet(comboStyle);
    content->addWidget(combo_row(std::string(doremi_tr("language")), lang_cmb_));

    content->addSpacing(12);
    auto *sep_about = new QFrame(inner);
    sep_about->setFrameShape(QFrame::HLine);
    sep_about->setStyleSheet(QString("color: %1;").arg(c.border.name()));
    content->addWidget(sep_about);

    // Acerca de section
    content->addWidget(section_header(std::string(doremi_tr("settings_about"))));
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
        QString::fromStdString(std::string(doremi_tr("about_desc"))),
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
    auto *check_updates_btn = new RippleButton(QString::fromStdString(std::string(doremi_tr("check_updates"))), inner, RippleButton::Variant::Secondary);
    check_updates_btn->setIcon(IconProvider::getIcon("sync", c.accent, 20));
    check_updates_btn->setMinimumHeight(44);
    
    QObject::connect(check_updates_btn, &QPushButton::clicked, this, [check_updates_btn]() {
        check_updates_btn->setEnabled(false);
        check_updates_btn->setText(QString::fromStdString(std::string(doremi_tr("checking_updates_btn"))));
        
        // Call Rust check function
        on_check_for_updates_requested();

        // Restore button state after 4 seconds (fallback)
        QTimer::singleShot(4000, check_updates_btn, [check_updates_btn]() {
            check_updates_btn->setEnabled(true);
            check_updates_btn->setText(QString::fromStdString(std::string(doremi_tr("check_updates"))));
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
        emit setting_changed("theme_mode", Ffi::to_std_string(v));
    });
    QObject::connect(accent_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("accent_color", Ffi::to_std_string(v));
    });
    QObject::connect(font_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("font_size", Ffi::to_std_string(v));
    });
    QObject::connect(lang_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("language", Ffi::to_std_string(v));
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
        if (v != "Custom") emit setting_changed("equalizer_preset", Ffi::to_std_string(v));
    });
    QObject::connect(eq_preamp_slider_, &QSlider::valueChanged, this, [this](int value) {
        eq_preamp_value_->setText(QString::number(value / 10.0, 'f', 1) + " dB");
        eq_preset_cmb_->blockSignals(true);
        eq_preset_cmb_->setCurrentText("Custom");
        eq_preset_cmb_->blockSignals(false);
        emit setting_changed("equalizer_preamp", std::to_string(value / 10.0));
    });
    for (auto *slider : eq_band_sliders_) {
        QObject::connect(slider, &QSlider::valueChanged, this, [this](int) {
            std::string values;
            for (size_t i = 0; i < eq_band_sliders_.size(); ++i) {
                eq_band_values_[i]->setText(QString::number(eq_band_sliders_[i]->value() / 10.0, 'f', 1));
                if (!values.empty()) values += ',';
                values += std::to_string(eq_band_sliders_[i]->value() / 10.0);
            }
            eq_preset_cmb_->blockSignals(true);
            eq_preset_cmb_->setCurrentText("Custom");
            eq_preset_cmb_->blockSignals(false);
            emit setting_changed("equalizer_bands", values);
        });
    }
    QObject::connect(eq_reset_btn_, &QPushButton::clicked, this, [this]() {
        eq_preset_cmb_->setCurrentText("Flat");
    });
    QObject::connect(discord_rpc_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("discord_rpc_enabled", v ? "true" : "false");
    });
    QObject::connect(lastfm_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        lastfm_auth_widget_->setVisible(v);
        emit setting_changed("lastfm_enabled", v ? "true" : "false");
    });
    QObject::connect(mpris_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("mpris_enabled", v ? "true" : "false");
    });
    QObject::connect(stop_on_close_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("stop_on_close", v ? "true" : "false");
    });
    QObject::connect(lastfm_auth_btn_, &QPushButton::clicked, this, [this]() {
        if (lastfm_auth_btn_->text() == "Conectar Cuenta") {
            std::string apiKey = Ffi::to_std_string(lastfm_api_key_input_->text());
            std::string apiSecret = Ffi::to_std_string(lastfm_api_secret_input_->text());
            std::string username = Ffi::to_std_string(lastfm_username_input_->text());
            std::string password = Ffi::to_std_string(lastfm_password_input_->text());
            emit lastfm_auth_requested(apiKey, apiSecret, username, password);
            std::fill(password.begin(), password.end(), '\0');
            std::fill(apiSecret.begin(), apiSecret.end(), '\0');
            lastfm_password_input_->clear();
            lastfm_api_secret_input_->clear();
        } else {
            emit lastfm_disconnect_requested();
        }
    });

    // Subtitles connections
    QObject::connect(sub_alignment_cmb_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        std::string align = "center";
        if (idx == 0) align = "left";
        else if (idx == 2) align = "right";
        emit setting_changed("subtitle_alignment", align);
    });
    QObject::connect(sub_font_size_cmb_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int size = sub_font_size_cmb_->itemData(idx).toInt();
        emit setting_changed("subtitle_font_size", std::to_string(size));
    });
    QObject::connect(sub_line_spacing_cmb_, &QComboBox::currentTextChanged, this, [this](const QString &v) {
        emit setting_changed("subtitle_line_spacing", Ffi::to_std_string(v));
    });
    QObject::connect(sub_auto_scroll_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("subtitle_auto_scroll", v ? "true" : "false");
    });
    QObject::connect(sub_glow_cb_, &AnimatedToggle::toggled, this, [this](bool v) {
        emit setting_changed("subtitle_glow_effect", v ? "true" : "false");
    });

    // Storage connections
    QObject::connect(clean_cache_btn_, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            QString::fromStdString(std::string(doremi_tr("warning"))),
            QString::fromStdString(std::string(doremi_tr("confirm_clear_cache"))),
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            clear_cache();
            refresh_storage_sizes();
            show_notification(std::string(doremi_tr("success")), "success");
        }
    });

    QObject::connect(clear_downloads_btn_, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            QString::fromStdString(std::string(doremi_tr("warning"))),
            QString::fromStdString(std::string(doremi_tr("confirm_clear_downloads"))),
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            clear_downloads();
            refresh_storage_sizes();
            show_notification(std::string(doremi_tr("success")), "success");
        }
    });

    QObject::connect(export_backup_btn_, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this,
            QString::fromStdString(std::string(doremi_tr("export_backup"))),
            QDir::homePath() + "/doremi_backup.zip",
            "Zip Files (*.zip)"
        );
        if (!path.isEmpty()) {
            bool ok = export_backup(Ffi::to_std_string(path));
            if (ok) {
                show_notification(std::string(doremi_tr("backup_export_success")), "success");
            } else {
                show_notification(std::string(doremi_tr("backup_export_error")), "error");
            }
        }
    });

    QObject::connect(import_backup_btn_, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this,
            QString::fromStdString(std::string(doremi_tr("import_backup"))),
            QDir::homePath(),
            "Zip Files (*.zip)"
        );
        if (!path.isEmpty()) {
            bool ok = import_backup(Ffi::to_std_string(path));
            if (ok) {
                refresh_storage_sizes();
                show_notification(std::string(doremi_tr("backup_import_success")), "success");
            } else {
                show_notification(std::string(doremi_tr("backup_import_error")), "error");
            }
        }
    });

    refresh_storage_sizes();
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

void SettingsView::set_equalizer_values(double preamp, const std::vector<double> &bands) {
    eq_preamp_slider_->blockSignals(true);
    eq_preamp_slider_->setValue(qRound(preamp * 10.0));
    eq_preamp_value_->setText(QString::number(preamp, 'f', 1) + " dB");
    eq_preamp_slider_->blockSignals(false);
    for (size_t i = 0; i < eq_band_sliders_.size(); ++i) {
        const double value = i < bands.size() ? bands[i] : 0.0;
        eq_band_sliders_[i]->blockSignals(true);
        eq_band_sliders_[i]->setValue(qRound(value * 10.0));
        eq_band_values_[i]->setText(QString::number(value, 'f', 1));
        eq_band_sliders_[i]->blockSignals(false);
    }
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
        lastfm_status_lbl_->setText(QString::fromStdString(std::string(doremi_tr("lastfm_status_connected")) + username));
        lastfm_api_key_input_->setText(QString::fromStdString(apiKey));
        lastfm_api_secret_input_->setText(QString::fromStdString(apiSecret));
        lastfm_api_key_input_->setEnabled(false);
        lastfm_api_secret_input_->setEnabled(false);
        lastfm_username_input_->setVisible(false);
        lastfm_password_input_->setVisible(false);
        lastfm_auth_btn_->setText(QString::fromStdString(std::string(doremi_tr("lastfm_btn_disconnect"))));
        lastfm_auth_btn_->setVariant(RippleButton::Variant::Danger);
    } else {
        lastfm_status_lbl_->setText(QString::fromStdString(std::string(doremi_tr("lastfm_status_disconnected"))));
        lastfm_api_key_input_->setText("");
        lastfm_api_secret_input_->setText("");
        lastfm_username_input_->setText("");
        lastfm_password_input_->setText("");
        lastfm_api_key_input_->setEnabled(true);
        lastfm_api_secret_input_->setEnabled(true);
        lastfm_username_input_->setVisible(true);
        lastfm_password_input_->setVisible(true);
        lastfm_auth_btn_->setText(QString::fromStdString(std::string(doremi_tr("lastfm_btn_connect"))));
        lastfm_auth_btn_->setVariant(RippleButton::Variant::Primary);
    }
}

void SettingsView::set_settings_stop_on_close(bool stop) {
    stop_on_close_cb_->blockSignals(true);
    stop_on_close_cb_->setChecked(stop);
    stop_on_close_cb_->blockSignals(false);
}

void SettingsView::set_settings_mpris_enabled(bool on) {
    mpris_cb_->blockSignals(true);
    mpris_cb_->setChecked(on);
    mpris_cb_->blockSignals(false);
}

bool SettingsView::discord_rpc_enabled() const { return discord_rpc_cb_->isChecked(); }
bool SettingsView::lastfm_enabled() const { return lastfm_cb_->isChecked(); }

void SettingsView::set_subtitle_alignment(const std::string &alignment) {
    sub_alignment_cmb_->blockSignals(true);
    int idx = 1; // Center default
    if (alignment == "left") idx = 0;
    else if (alignment == "right") idx = 2;
    sub_alignment_cmb_->setCurrentIndex(idx);
    sub_alignment_cmb_->blockSignals(false);
}

void SettingsView::set_subtitle_font_size(int32_t size) {
    sub_font_size_cmb_->blockSignals(true);
    int idx = sub_font_size_cmb_->findData(size);
    if (idx >= 0) sub_font_size_cmb_->setCurrentIndex(idx);
    sub_font_size_cmb_->blockSignals(false);
}

void SettingsView::set_subtitle_line_spacing(double spacing) {
    sub_line_spacing_cmb_->blockSignals(true);
    QString spacing_str = QString::number(spacing, 'f', 1);
    int idx = sub_line_spacing_cmb_->findText(spacing_str);
    if (idx >= 0) sub_line_spacing_cmb_->setCurrentIndex(idx);
    sub_line_spacing_cmb_->blockSignals(false);
}

void SettingsView::set_subtitle_auto_scroll(bool on) {
    sub_auto_scroll_cb_->blockSignals(true);
    sub_auto_scroll_cb_->setChecked(on);
    sub_auto_scroll_cb_->blockSignals(false);
}

void SettingsView::set_subtitle_glow_effect(bool on) {
    sub_glow_cb_->blockSignals(true);
    sub_glow_cb_->setChecked(on);
    sub_glow_cb_->blockSignals(false);
}

void SettingsView::refresh_storage_sizes() {
    auto sizes = get_storage_sizes();
    if (sizes.size() >= 3) {
        db_size_lbl_->setText(QString::number(sizes[0], 'f', 2) + " MB");
        cache_size_lbl_->setText(QString::number(sizes[1], 'f', 2) + " MB");
        downloads_size_lbl_->setText(QString::number(sizes[2], 'f', 2) + " MB");
    }
}

QWidget *SettingsView::storage_row(const std::string &label, QLabel *val_lbl, RippleButton *btn) {
    const auto &c = DesignTokens::current();
    auto *w = new QWidget(this);
    w->setFixedHeight(36);
    w->setStyleSheet("background: transparent;");
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    
    auto *lb = new QLabel(QString::fromStdString(label), w);
    lb->setFont(DesignTokens::getFont("body", 13));
    lb->setStyleSheet(QString("color: %1; background: transparent;").arg(c.text_secondary.name()));
    l->addWidget(lb);
    
    l->addStretch(1);
    l->addWidget(val_lbl);
    l->addSpacing(16);
    l->addWidget(btn);
    return w;
}
