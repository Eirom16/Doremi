#ifndef DOREMI_SETTINGS_VIEW_H
#define DOREMI_SETTINGS_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QSlider>
#include <vector>
#include "components/animated_toggle.h"
#include "components/ripple_button.h"

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(QWidget *parent = nullptr);

    void set_theme(const std::string &mode);
    void set_accent(const std::string &color);
    void set_font_size(int32_t size);
    void set_language(const std::string &lang);
    void set_region(const std::string &region);
    void set_normalize(bool on);
    void set_crossfade(bool on);
    void set_equalizer_enabled(bool on);
    void set_equalizer_preset(const std::string &preset);
    void set_equalizer_values(double preamp, const std::vector<double> &bands);
    void set_sleep_timer(int32_t minutes);
    void set_settings_discord_rpc(bool on);
    void set_settings_lastfm_enabled(bool on);
    void set_settings_stop_on_close(bool stop);
    void set_settings_mpris_enabled(bool on);
    void set_settings_lastfm_session(bool authenticated, const std::string &username, const std::string &apiKey, const std::string &apiSecret);

    void set_subtitle_alignment(const std::string &alignment);
    void set_subtitle_font_size(int32_t size);
    void set_subtitle_line_spacing(double spacing);
    void set_subtitle_auto_scroll(bool on);
    void set_subtitle_glow_effect(bool on);
    void refresh_storage_sizes();

    std::string theme() const;
    std::string accent() const;
    int32_t font_size() const;
    std::string language() const;
    bool normalize() const;
    bool crossfade() const;
    bool equalizer_enabled() const;
    std::string equalizer_preset() const;
    int32_t sleep_timer() const;
    bool discord_rpc_enabled() const;
    bool lastfm_enabled() const;

signals:
    void setting_changed(const std::string &key, const std::string &value);
    void lastfm_auth_requested(const std::string &apiKey, const std::string &apiSecret, const std::string &username, const std::string &password);
    void lastfm_disconnect_requested();

private:
    QComboBox *theme_cmb_;
    QComboBox *accent_cmb_;
    QComboBox *font_cmb_;
    QComboBox *lang_cmb_;
    QComboBox *region_cmb_;
    AnimatedToggle *normalize_cb_;
    AnimatedToggle *crossfade_cb_;
    AnimatedToggle *equalizer_cb_;
    QComboBox *eq_preset_cmb_;
    QSlider *eq_preamp_slider_;
    QLabel *eq_preamp_value_;
    std::vector<QSlider *> eq_band_sliders_;
    std::vector<QLabel *> eq_band_values_;
    QPushButton *eq_reset_btn_;
    QComboBox *sleep_timer_cmb_;

    AnimatedToggle *discord_rpc_cb_;
    AnimatedToggle *lastfm_cb_;
    AnimatedToggle *mpris_cb_;
    AnimatedToggle *stop_on_close_cb_;

    QWidget *lastfm_auth_widget_;
    QLineEdit *lastfm_api_key_input_;
    QLineEdit *lastfm_api_secret_input_;
    QLineEdit *lastfm_username_input_;
    QLineEdit *lastfm_password_input_;
    RippleButton *lastfm_auth_btn_;
    QLabel *lastfm_status_lbl_;

    // Subtitles
    QComboBox *sub_alignment_cmb_;
    QComboBox *sub_font_size_cmb_;
    QComboBox *sub_line_spacing_cmb_;
    AnimatedToggle *sub_auto_scroll_cb_;
    AnimatedToggle *sub_glow_cb_;

    // Storage
    QLabel *db_size_lbl_;
    QLabel *cache_size_lbl_;
    QLabel *downloads_size_lbl_;
    RippleButton *clean_cache_btn_;
    RippleButton *clear_downloads_btn_;
    RippleButton *export_backup_btn_;
    RippleButton *import_backup_btn_;

    QWidget *section_header(const std::string &title);
    QWidget *combo_row(const std::string &label, QComboBox *cmb);
    QWidget *check_row(const std::string &label, AnimatedToggle *cb);
    QWidget *input_row(const std::string &label, QLineEdit *input);
    QWidget *storage_row(const std::string &label, QLabel *val_lbl, RippleButton *btn);
};

#endif
