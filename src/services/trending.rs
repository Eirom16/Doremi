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
        let sections = self.api.charts().await;
        let items = sections
            .into_iter()
            .flat_map(|section| section.items)
            .take(30)
            .collect::<Vec<_>>();
        let titles = items.iter().map(|item| item.title.clone()).collect();
        let subtitles = items.iter().map(|item| item.subtitle.clone()).collect();
        let thumbnails = items.iter().map(|item| item.thumbnail.clone()).collect();
        crate::bridge::bridge::set_trending_items(titles, subtitles, thumbnails);
    }
}

impl Default for TrendingService {
    fn default() -> Self {
        Self::new()
    }
}
