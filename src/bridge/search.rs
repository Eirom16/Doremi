use super::bridge;

pub fn on_search_submitted(query: &str, filter: &str) {
    if query.trim().is_empty() {
        return;
    }
    log::info!("Search: {query} with filter: {filter}");
    let query = query.to_string();
    let filter = filter.to_string();
    tokio::spawn(async move {
        let q_clone = query.clone();
        let f_clone = filter.clone();
        tokio::task::spawn_blocking(move || {
            let _ = crate::db::repo::SearchHistoryRepo::record(&q_clone, &f_clone);
        })
        .await
        .ok();

        push_search_history_to_ui().await;

        if let Some(search) = super::SEARCH.get() {
            let res = search.search(&query, &filter).await;
            search.push_to_ui(&res);
        } else {
            log::error!("Search service is not initialized during search submission!");
        }
    });
}

pub fn on_search_suggestions_requested(query: &str) {
    let query = query.to_string();
    tokio::spawn(async move {
        let suggestions = match super::SEARCH.get() {
            Some(search) => search.suggestions(&query).await,
            None => {
                log::warn!("Search service is not initialized during suggestions request!");
                Vec::new()
            }
        };
        bridge::set_search_suggestions(&query, suggestions);
    });
}

async fn push_search_history_to_ui() {
    let queries = tokio::task::spawn_blocking(|| {
        crate::db::repo::SearchHistoryRepo::recent(20)
            .unwrap_or_default()
            .into_iter()
            .map(|entry| entry.query)
            .collect::<Vec<_>>()
    })
    .await
    .unwrap_or_default();
    bridge::set_search_history(queries);
}

pub fn on_search_history_requested() {
    tokio::spawn(push_search_history_to_ui());
}

pub fn on_search_history_delete(query: &str) {
    let query = query.trim().to_string();
    if query.is_empty() {
        return;
    }
    tokio::spawn(async move {
        tokio::task::spawn_blocking(move || {
            let _ = crate::db::repo::SearchHistoryRepo::delete_entry(&query);
        })
        .await
        .ok();
        push_search_history_to_ui().await;
    });
}

pub fn on_search_history_clear() {
    tokio::spawn(async move {
        tokio::task::spawn_blocking(|| {
            let _ = crate::db::repo::SearchHistoryRepo::clear();
        })
        .await
        .ok();
        push_search_history_to_ui().await;
    });
}
