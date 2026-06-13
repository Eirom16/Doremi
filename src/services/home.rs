use crate::api::client::ApiClient;

pub struct HomeService {
    api: ApiClient,
}

impl HomeService {
    pub fn new() -> Self {
        Self { api: ApiClient::new() }
    }

    pub fn load_home(&self) {
        let sections = self.api.home_sections();

        crate::bridge::bridge::clear_home_sections();
        for section in &sections {
            let items: Vec<String> = section.items.iter()
                .map(|i| format!("{} — {}", i.title, i.subtitle))
                .collect();
            crate::bridge::bridge::add_home_section(&section.title, items);
        }
    }
}

impl Default for HomeService {
    fn default() -> Self {
        Self::new()
    }
}
