#ifndef DOREMI_SETTINGS_VIEW_H
#define DOREMI_SETTINGS_VIEW_H

#include <QDialog>
#include <QQuickWidget>
#include <QVariantMap>
#include <QString>
#include <vector>
#include <string>

class SettingsView : public QDialog {
    Q_OBJECT
    Q_PROPERTY(QVariantMap config READ config NOTIFY configChanged)

public:
    explicit SettingsView(QWidget *parent = nullptr);
    void update_theme() {}

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
    void set_download_location(const std::string &location);
    void set_download_format(const std::string &format);
    void set_download_quality(const std::string &quality);
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

    QVariantMap config() const { return config_; }

    Q_INVOKABLE void updateSetting(const QString &key, const QVariant &value);
    Q_INVOKABLE void requestLastfmAuth(const QString &apiKey, const QString &apiSecret, const QString &username, const QString &password);
    Q_INVOKABLE void requestLastfmDisconnect();
    Q_INVOKABLE void closeDialog() { this->close(); }

signals:
    void setting_changed(const std::string &key, const std::string &value);
    void lastfm_auth_requested(const std::string &apiKey, const std::string &apiSecret, const std::string &username, const std::string &password);
    void lastfm_disconnect_requested();
    void configChanged();

private:
    QQuickWidget *quick_widget_ = nullptr;
    QVariantMap config_;
    
    void updateConfig(const QString &key, const QVariant &value);
};

#endif
