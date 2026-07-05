#include "settings_view.h"
#include <QQmlContext>
#include <QVBoxLayout>

SettingsView::SettingsView(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(1000, 650);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    quick_widget_ = new QQuickWidget(this);
    quick_widget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quick_widget_->setAttribute(Qt::WA_TranslucentBackground);
    quick_widget_->setClearColor(Qt::transparent);

    quick_widget_->rootContext()->setContextProperty("SettingsCtrl", this);
    quick_widget_->setSource(QUrl("qrc:/qml/SettingsView.qml"));

    layout->addWidget(quick_widget_);
}

void SettingsView::updateConfig(const QString &key, const QVariant &value) {
    if (config_[key] != value) {
        config_[key] = value;
        emit configChanged();
    }
}

void SettingsView::updateSetting(const QString &key, const QVariant &value) {
    updateConfig(key, value);
    emit setting_changed(key.toStdString(), value.toString().toStdString());
}

void SettingsView::requestLastfmAuth(const QString &apiKey, const QString &apiSecret, const QString &username, const QString &password) {
    emit lastfm_auth_requested(apiKey.toStdString(), apiSecret.toStdString(), username.toStdString(), password.toStdString());
}

void SettingsView::requestLastfmDisconnect() {
    emit lastfm_disconnect_requested();
}

void SettingsView::set_theme(const std::string &mode) { updateConfig("theme", QString::fromStdString(mode)); }
void SettingsView::set_accent(const std::string &color) { updateConfig("accent", QString::fromStdString(color)); }
void SettingsView::set_font_size(int32_t size) { updateConfig("fontSize", size); }
void SettingsView::set_language(const std::string &lang) { updateConfig("language", QString::fromStdString(lang)); }
void SettingsView::set_region(const std::string &region) { updateConfig("region", QString::fromStdString(region)); }
void SettingsView::set_normalize(bool on) { updateConfig("normalize", on); }
void SettingsView::set_crossfade(bool on) { updateConfig("crossfade", on); }
void SettingsView::set_equalizer_enabled(bool on) { updateConfig("equalizerEnabled", on); }
void SettingsView::set_equalizer_preset(const std::string &preset) { updateConfig("equalizerPreset", QString::fromStdString(preset)); }
void SettingsView::set_equalizer_values(double preamp, const std::vector<double> &bands) {
    // Optional: map EQ values if needed by QML
}
void SettingsView::set_sleep_timer(int32_t minutes) { updateConfig("sleepTimer", minutes); }
void SettingsView::set_settings_discord_rpc(bool on) { updateConfig("discordRpc", on); }
void SettingsView::set_settings_lastfm_enabled(bool on) { updateConfig("lastfmEnabled", on); }
void SettingsView::set_settings_stop_on_close(bool stop) { updateConfig("stopOnClose", stop); }
void SettingsView::set_settings_mpris_enabled(bool on) { updateConfig("mprisEnabled", on); }
void SettingsView::set_settings_lastfm_session(bool authenticated, const std::string &username, const std::string &apiKey, const std::string &apiSecret) {
    updateConfig("lastfmAuthenticated", authenticated);
    updateConfig("lastfmUsername", QString::fromStdString(username));
    updateConfig("lastfmApiKey", QString::fromStdString(apiKey));
}

void SettingsView::set_subtitle_alignment(const std::string &alignment) { updateConfig("subtitleAlignment", QString::fromStdString(alignment)); }
void SettingsView::set_subtitle_font_size(int32_t size) { updateConfig("subtitleFontSize", size); }
void SettingsView::set_subtitle_line_spacing(double spacing) { updateConfig("subtitleLineSpacing", spacing); }
void SettingsView::set_subtitle_auto_scroll(bool on) { updateConfig("subtitleAutoScroll", on); }
void SettingsView::set_subtitle_glow_effect(bool on) { updateConfig("subtitleGlowEffect", on); }
void SettingsView::set_download_location(const std::string &location) { updateConfig("downloadLocation", QString::fromStdString(location)); }
void SettingsView::set_download_format(const std::string &format) { updateConfig("downloadFormat", QString::fromStdString(format)); }
void SettingsView::set_download_quality(const std::string &quality) { updateConfig("downloadQuality", QString::fromStdString(quality)); }

void SettingsView::refresh_storage_sizes() {
    // Emit signal or update properties for QML to show sizes
}

// Getters (used by C++ backend, these MUST return valid values usually stored in config_)
std::string SettingsView::theme() const { return config_["theme"].toString().toStdString(); }
std::string SettingsView::accent() const { return config_["accent"].toString().toStdString(); }
int32_t SettingsView::font_size() const { return config_["fontSize"].toInt(); }
std::string SettingsView::language() const { return config_["language"].toString().toStdString(); }
bool SettingsView::normalize() const { return config_["normalize"].toBool(); }
bool SettingsView::crossfade() const { return config_["crossfade"].toBool(); }
bool SettingsView::equalizer_enabled() const { return config_["equalizerEnabled"].toBool(); }
std::string SettingsView::equalizer_preset() const { return config_["equalizerPreset"].toString().toStdString(); }
int32_t SettingsView::sleep_timer() const { return config_["sleepTimer"].toInt(); }
bool SettingsView::discord_rpc_enabled() const { return config_["discordRpc"].toBool(); }
bool SettingsView::lastfm_enabled() const { return config_["lastfmEnabled"].toBool(); }
