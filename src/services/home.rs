use crate::api::client::ApiClient;
use once_cell::sync::Lazy;
use tokio::sync::Mutex;

static HOME_CONTINUATION: Lazy<Mutex<Option<String>>> = Lazy::new(|| Mutex::new(None));

pub struct HomeService {
    api: ApiClient,
}

impl HomeService {
    pub fn new() -> Self {
        Self {
            api: ApiClient::new(),
        }
    }

    pub async fn load_home(&self) {
        crate::bridge::bridge::set_home_state("loading", "");
        let (sections, continuation) = match self.api.home_sections_page(None).await {
            Ok((sections, _)) if sections.is_empty() => {
                crate::bridge::bridge::set_home_state(
                    "empty",
                    "No hay recomendaciones disponibles por ahora.",
                );
                return;
            }
            Ok(page) => page,
            Err(error) => {
                log::error!("Home feed API failed: {error}");
                crate::bridge::bridge::set_home_state("error", &error);
                return;
            }
        };
        *HOME_CONTINUATION.lock().await = continuation;
        crate::bridge::bridge::clear_home_sections();
        push_sections(&sections);
        crate::bridge::bridge::set_home_state("content", "");
    }

    pub async fn load_more(&self) {
        let token = HOME_CONTINUATION.lock().await.take();
        let Some(token) = token else { return };
        match self.api.home_sections_page(Some(&token)).await {
            Ok((sections, continuation)) => {
                *HOME_CONTINUATION.lock().await = continuation;
                push_sections(&sections);
            }
            Err(error) => {
                *HOME_CONTINUATION.lock().await = Some(token);
                log::debug!("Could not load more home sections: {error}");
            }
        }
    }
}

fn push_sections(sections: &[crate::api::models::HomeSection]) {
    for section in sections {
        let items = section
            .items
            .iter()
            .map(|item| crate::bridge::bridge::HomeCard {
                id: item
                    .video_id
                    .as_ref()
                    .or(item.browse_id.as_ref())
                    .or(item.playlist_id.as_ref())
                    .cloned()
                    .unwrap_or_default(),
                title: item.title.clone(),
                subtitle: item.subtitle.clone(),
                thumbnail: item.thumbnail.clone(),
                item_type: item.item_type.clone(),
            })
            .collect::<Vec<_>>();
        crate::bridge::bridge::add_home_section(&section.title, items);
    }
}

impl Default for HomeService {
    fn default() -> Self {
        Self::new()
    }
}
