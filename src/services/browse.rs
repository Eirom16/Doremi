pub async fn load_album(browse_id: &str) {
    match crate::api::innertube::album_detail(browse_id).await {
        Ok((album, tracks)) => {
            let album = crate::bridge::bridge::Album {
                id: album.id,
                title: album.title,
                artist: album.artists.join(", "),
                year: album.year.map(|year| year.to_string()).unwrap_or_default(),
                thumbnail: album.thumbnail,
                track_count: album.track_count.unwrap_or(tracks.len() as i32),
            };
            let tracks = tracks
                .into_iter()
                .map(|track| crate::bridge::bridge::Track {
                    id: track.id,
                    title: track.title,
                    artist: track.artists.join(", "),
                    album: track.album.unwrap_or_default(),
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                })
                .collect();
            crate::bridge::bridge::set_album_detail(album, tracks);
        }
        Err(error) => crate::bridge::bridge::show_notification(
            &format!("No se pudo cargar el álbum: {error}"),
            "error",
        ),
    }
}

pub async fn load_artist(browse_id: &str) {
    match crate::api::innertube::artist_detail(browse_id).await {
        Ok(mut detail) => {
            let artist = crate::bridge::bridge::Artist {
                id: detail.artist.id,
                name: detail.artist.name,
                thumbnail: detail.artist.thumbnail,
                description: detail.description.unwrap_or_default(),
                subscribers: detail.artist.subscriber_count.unwrap_or_default(),
            };
            let tracks = detail
                .top_songs
                .into_iter()
                .map(|track| crate::bridge::bridge::Track {
                    id: track.id,
                    title: track.title,
                    artist: track.artists.join(", "),
                    album: track.album.unwrap_or_default(),
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                })
                .collect();
            detail.albums.append(&mut detail.singles);
            let albums = detail
                .albums
                .into_iter()
                .map(|album| crate::bridge::bridge::Album {
                    id: album.id,
                    title: album.title,
                    artist: album.artists.join(", "),
                    year: album.year.map(|year| year.to_string()).unwrap_or_default(),
                    thumbnail: album.thumbnail,
                    track_count: album.track_count.unwrap_or_default(),
                })
                .collect();
            crate::bridge::bridge::set_artist_detail(artist, tracks, albums);
        }
        Err(error) => crate::bridge::bridge::show_notification(
            &format!("No se pudo cargar el artista: {error}"),
            "error",
        ),
    }
}
