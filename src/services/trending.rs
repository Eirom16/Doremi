use crate::api::client::ApiClient;

pub struct TrendingService {
    api: ApiClient,
}

impl TrendingService {
    pub fn new() -> Self {
        Self {
            api: ApiClient::new(),
        }
    }

    pub async fn load(&self) {
        crate::bridge::bridge::set_trending_state("loading", "");
        let sections = match self.api.charts().await {
            Ok(sections) => sections,
            Err(error) => {
                log::error!("Charts API failed: {error}");
                crate::bridge::bridge::set_trending_state("error", &error);
                return;
            }
        };
        let items = sections
            .into_iter()
            .flat_map(|section| section.items)
            .take(30)
            .collect::<Vec<_>>();
        if items.is_empty() {
            crate::bridge::bridge::set_trending_state(
                "empty",
                "No hay tendencias disponibles para esta región.",
            );
            return;
        }
        let cards = items
            .into_iter()
            .map(|item| crate::bridge::bridge::HomeCard {
                id: item
                    .video_id
                    .or(item.browse_id)
                    .or(item.playlist_id)
                    .unwrap_or_default(),
                title: item.title,
                subtitle: item.subtitle,
                thumbnail: item.thumbnail,
                item_type: item.item_type,
            })
            .collect();
        crate::bridge::bridge::set_trending_items(cards);
        crate::bridge::bridge::set_trending_state("content", "");
    }
}

impl Default for TrendingService {
    fn default() -> Self {
        Self::new()
    }
}
