# Matriz de paridad Pyrolist -> Doremi

Version del inventario: `1.0.0`

Fecha de auditoria: `2026-06-13`

Fuentes auditadas:

- Pyrolist: commit `ce9958f79996a257e2687321d9000b94bac47522`.
- Doremi: commit base `18c4a513b90dcedb8d2687b1e5989c8bb2460d80` mas cambios locales existentes al momento de la auditoria.
- Casos de referencia: [MANUAL_CASES.md](MANUAL_CASES.md).
- Referencias visuales: [VISUAL_REFERENCES.md](VISUAL_REFERENCES.md).
- Politica de alcance: [DEPRECATION_POLICY.md](DEPRECATION_POLICY.md).

## Estados

- `ausente`: no existe una ruta utilizable en Doremi.
- `UI solamente`: existe presentacion, pero no completa el flujo real.
- `parcial`: parte del flujo funciona, pero faltan acciones, datos o manejo de errores relevantes.
- `funcional`: el flujo principal esta conectado de extremo a extremo.
- `verificado`: es funcional y tiene prueba automatizada o ejecucion manual documentada reproducible.

Un archivo, widget o metodo con nombre equivalente no basta para superar `UI solamente`.

## Aplicacion y autenticacion

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| APP-01 | Arranque, inicializacion de DB y ventana principal | `main.py`, `ui/main_window.py` | `main.rs`, `app.rs`, `main_window.cpp` | funcional | Falta prueba GUI de arranque y cierre. |
| APP-02 | Navegacion lateral entre pantallas | `nav_sidebar.py`, `main_window.py` | `nav_sidebar.cpp`, `main_window.cpp` | funcional | Falta verificar historial y conservacion de scroll. |
| APP-03 | Navegacion atras desde detalles | `main_window.py`, pantallas de detalle | `main_window.cpp`, vistas de detalle | parcial | No todos los destinos restauran estado previo. |
| APP-04 | Login web de YouTube Music | `login_dialog.py`, `youtube_music.py` | `login_dialog.cpp`, `bridge.rs` | parcial | Captura de sesion sin endurecimiento completo de dominios, challenges y expiracion. |
| APP-05 | Restaurar sesion autenticada | `youtube_music.py`, keyring | `secure_storage.rs`, `app.rs` | funcional | Falta prueba de sesion revocada y renovacion. |
| APP-06 | Logout y limpieza de sesion | `youtube_music.py`, settings de cuenta | `bridge.rs`, `secure_storage.rs` | verificado | Cubierto por pruebas de almacenamiento seguro. |
| APP-07 | Perfil, nombre y avatar | `main_window.py`, `nav_sidebar.py` | `main_window.cpp`, `bridge.rs` | parcial | Solo datos capturados durante login; falta refresco remoto robusto. |
| APP-08 | Monitor de red y banner offline | `system/network.py`, `offline_banner.py` | Sin equivalente | ausente | Requiere monitor, estado offline y recuperacion automatica. |
| APP-09 | Instancia unica y argumentos externos | Sin flujo completo | Sin equivalente | ausente | Necesario para URLs y activacion de ventana existente. |

## Descubrimiento y API

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| API-01 | Home anonimo/autenticado | `youtube_music.py`, `home.py` | `api/innertube.rs`, `services/home.rs`, `home_view.cpp` | parcial | Parsing fragil, sin continuations ni cache por sesion/locale. |
| API-02 | Search general | `youtube_music.py`, `search.py` | `services/search.rs`, `search_view.cpp` | parcial | Resultados reales, pero faltan filtros y paginacion completos. |
| API-03 | Sugerencias de busqueda | `global_search.py`, `youtube_music.py` | Sin equivalente conectado | ausente | Falta dropdown, debounce e historial interactivo. |
| API-04 | Historial de busqueda | `global_search.py` | `db/repo.rs`, bridge parcial | parcial | Persistencia existe; falta experiencia global completa. |
| API-05 | Explore y tendencias por region | `youtube_music.py`, `home.py` | `trending_view.cpp`, API parcial | UI solamente | La vista existe, pero no cubre charts reales por region. |
| API-06 | Detalle de album | `youtube_music.py`, `album.py` | `album_detail_view.cpp`, DTOs | parcial | Presentacion y tracks basicos; faltan varias acciones y metadata. |
| API-07 | Detalle de artista | `youtube_music.py`, `artist.py` | `artist_detail_view.cpp`, DTOs | parcial | Faltan singles, relacionados, continuations y acciones completas. |
| API-08 | Detalle de playlist | `youtube_music.py`, `playlist.py` | `playlist_detail_view.cpp`, DTOs | parcial | Faltan continuations y gestion remota. |
| API-09 | Radio, related y watch playlist | `youtube_music.py` | Sin servicio equivalente completo | ausente | Bloquea autoplay real. |
| API-10 | Deteccion descriptiva de cambios de schema | Manejo generico de excepciones | Sin fixtures ni errores de schema | ausente | Requiere parsers aislados y fixtures anonimizados. |
| API-11 | Rate limiting, timeout y backoff | Parcial por librerias | Sin politica central | ausente | Debe definirse por servicio. |

## Biblioteca y colecciones

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| LIB-01 | Canciones favoritas locales | `library.py`, repositorios | `db/repo.rs`, `library_view.cpp` | funcional | Falta reconciliacion con likes remotos. |
| LIB-02 | Albums favoritos locales | `library.py` | `db/repo.rs`, `library_view.cpp` | funcional | Falta sincronizacion remota. |
| LIB-03 | Artistas favoritos locales | `library.py` | `db/repo.rs`, `library_view.cpp` | funcional | Falta subscriptions remotas. |
| LIB-04 | Biblioteca remota de canciones | `get_library_songs()` | UI y datos locales | ausente | Falta endpoint y mezcla/reconciliacion. |
| LIB-05 | Biblioteca remota de albums/artistas/playlists | Metodos `get_library_*` | UI y datos locales | ausente | Falta API autenticada y paginacion. |
| LIB-06 | Like/unlike remoto | `rate_song()` | Favorito local solamente | ausente | Falta estado remoto y reconciliacion. |
| LIB-07 | Crear playlist | `create_playlist()` y UI | `PlaylistRepo::create` local | parcial | Solo playlist local; falta YouTube Music. |
| LIB-08 | Renombrar y borrar playlist | UI y cliente YTM parcial | Repositorio local | parcial | Faltan acciones UI completas y sincronizacion remota. |
| LIB-09 | Agregar/eliminar canciones de playlist | `add_playlist_items()` y pantallas | Repositorio local | parcial | Falta flujo UI completo y operacion remota. |

## Reproduccion, cola y audio

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| PLAY-01 | Resolver stream de YouTube | `stream_extractor.py`, `youtube_music.py` | `player/mod.rs`, Innertube | parcial | Acoplado al player, sin expiracion ni fallback controlado. |
| PLAY-02 | Reproducir, pausar y detener | `audio/player.py` | `player/audio.rs`, `player/mod.rs` | funcional | Falta maquina de estados y pruebas con VLC real. |
| PLAY-03 | Seek, volumen y mute | `audio/player.py`, mini player | player y player bar | funcional | Falta sincronizacion verificada con todos los controles. |
| PLAY-04 | Siguiente y anterior | `audio/queue.py`, `main_window.py` | `player/queue.rs`, `player/mod.rs` | funcional | Faltan pruebas de carreras y bordes. |
| PLAY-05 | Shuffle y repeat | `audio/queue.py` | `player/queue.rs` | funcional | Falta verificar restauracion exacta de cola original y bordes. |
| PLAY-06 | Add next y add to end | Menus contextuales y cola | Metodos Rust existen | parcial | Acciones no estan disponibles consistentemente en todas las vistas. |
| PLAY-07 | Remove, move, clear y jump | `queue_panel.py`, `audio/queue.py` | Cola Rust y panel parcial | parcial | Falta drag and drop y cableado completo. |
| PLAY-08 | Persistir/restaurar cola y posicion | Ajustes y ultimo video parciales | Sin sesion completa | ausente | Requiere modelo persistente de sesion. |
| PLAY-09 | Precargar siguiente stream | Cache/prefetch de Pyrolist | Sin resolver desacoplado | ausente | Requiere cancelacion al cambiar cola. |
| PLAY-10 | Mini player flotante | `mini_player.py` | Player bar fijo | parcial | No equivale al flotante ni al modo compacto pulido. |
| PLAY-11 | Vista Now Playing | `now_playing.py` | `now_playing_view.cpp` | parcial | Falta riqueza visual, acciones y sincronizacion completa. |
| PLAY-12 | Ecualizador de 10 bandas | `player.py`, settings EQ | `player/audio.rs`, settings | parcial | Falta exponer/verificar bandas, preamp, presets y reset completos. |
| PLAY-13 | Crossfade | `crossfade.py` | Toggle/configuracion parcial | UI solamente | No existe crossfade real entre dos pipelines. |
| PLAY-14 | Gapless | Toggle en settings | Toggle/configuracion | UI solamente | Comportamiento no verificado ni documentado. |
| PLAY-15 | Normalizacion y skip silence | Toggles en settings | Toggles en settings | UI solamente | Motor no implementado. |
| PLAY-16 | Temporizador de apagado | `sleep_timer.py` | `player/mod.rs`, settings | funcional | Falta prueba de cancelacion y cierre. |

## Letras, descargas e historial

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| MEDIA-01 | Letras planas LRCLIB | `api/lyrics.py` | `services/lyrics.rs` | funcional | Falta cache persistente. |
| MEDIA-02 | Letras sincronizadas y seguimiento | `lyrics_view.py`, `lrc_parser.py` | `lyrics_widget.cpp` | parcial | Falta cubrir ajustes y sincronizacion avanzada. |
| MEDIA-03 | Cache persistente de letras | `lyrics_cache.py` | Sin equivalente | ausente | Requiere TTL/invalidation y migracion. |
| MEDIA-04 | Prefetch de letras | `lyrics_prefetcher.py` | Sin equivalente | ausente | Debe seguir la cola y admitir cancelacion. |
| DL-01 | Descargar una cancion | `download_manager.py` | `services/download.rs` | funcional | Usa `yt-dlp`; falta progreso observable robusto. |
| DL-02 | Cola y progreso de descargas | Manager Qt con señales | Worker interno y lista basica | parcial | Faltan estados tipados, progreso, cancelacion y reintentos. |
| DL-03 | Descargar album completo | `downloads.py`, manager | Sin flujo completo | ausente | Requiere expansion, cola y resumen. |
| DL-04 | Descargar playlist completa | `downloads.py`, manager | Sin flujo completo | ausente | Requiere expansion, cola y resumen. |
| DL-05 | Reproducir y borrar descargas | `downloads.py` | Vista y repositorio | parcial | Falta gestion completa y validacion de archivos faltantes. |
| HIST-01 | Historial local | repositorio y `history.py` | `db/repo.rs`, `history_view.cpp` | funcional | Falta toda la semantica y presentacion de Pyrolist. |
| HIST-02 | Historial remoto y eliminacion | `get_history`, `remove_history_items` | Sin equivalente | ausente | Requiere sesion autenticada. |
| HIST-03 | Estadisticas de escucha | `stats.py` | `stats_view.cpp`, DTOs | parcial | Datos base sin todos los periodos y visualizaciones. |

## Escritorio, integraciones y datos

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| SYS-01 | MPRIS | `system/mpris.py` | `mpris.rs` | parcial | Faltan emisiones de propiedades y metodos completos. |
| SYS-02 | Media keys | `system/media_keys.py` | MPRIS/atajos parciales | parcial | Falta verificacion GNOME, KDE y Wayland. |
| SYS-03 | Bandeja del sistema | `system/tray.py` | `main_window.cpp` | parcial | Falta politica consistente de cierre y acciones completas. |
| SYS-04 | Atajos de teclado | `main_window.py` | `main_window.cpp` | parcial | Falta inventario y paridad de todos los atajos. |
| INT-01 | Last.fm login, now playing y scrobble | `api/lastfm.py` | `services/lastfm.rs` | funcional | Falta prueba de red y reautenticacion. |
| INT-02 | Discord Rich Presence | `api/discord_rpc.py` | `services/discord.rs` | funcional | Falta prueba de reconexion y cierre. |
| DATA-01 | SQLite y migraciones Doremi | SQLAlchemy/Alembic parcial | `db/mod.rs`, `db/repo.rs` | verificado | Pruebas Rust cubren inicializacion y repositorios. |
| DATA-02 | Migracion automatica desde Pyrolist | No aplica | `utils/migration.rs` | verificado | Cubierta por pruebas de instalaciones vacia, parcial y danada. |
| DATA-03 | Backup y restore | `utils/backup.py` | `utils/backup.rs` | funcional | Falta ampliar pruebas de corrupcion y compatibilidad. |
| DATA-04 | Almacenamiento seguro | `secure_storage.py` | `secure_storage.rs` | verificado | Falla cerrado cuando no hay keyring. |
| DATA-05 | Cache real de artwork | `image_cache.py` | Sin equivalente completo | ausente | Requiere descarga, deduplicacion, limites y carga async. |
| DATA-06 | Limpiar cache y descargas | Settings storage | `utils/storage.rs`, settings | funcional | Falta confirmacion/errores detallados. |

## Interfaz, configuracion y entrega

| ID | Flujo de Pyrolist | Referencia Pyrolist | Equivalente Doremi | Estado | Brecha principal |
|---|---|---|---|---|---|
| UI-01 | Temas oscuro/claro y acento | tokens, stylesheet, settings | themes, design tokens, settings | funcional | Falta verificacion visual sistematica. |
| UI-02 | Color dinamico desde artwork | themes y ambient background | Color/ambient parcial | parcial | Falta cache e integracion estable con artwork real. |
| UI-03 | Sidebar compacta | `nav_sidebar.py` | sidebar nativa | parcial | Falta comprobar animacion y persistencia. |
| UI-04 | Componentes animados y skeletons | widgets de diseno | componentes C++ | parcial | Cobertura desigual entre pantallas. |
| UI-05 | Toasts | `toast.py` | `toast_notification.cpp` | funcional | Falta centro/historial de notificaciones. |
| UI-06 | Centro de notificaciones | notification button/dropdown | Sin equivalente | ausente | Debe integrar descargas, errores y eventos. |
| UI-07 | i18n ES/EN | locales y `i18n.py` | locales y `i18n.rs` | parcial | Menor cobertura y textos C++ hardcodeados. |
| UI-08 | Accesibilidad y teclado | Qt basico | Qt basico | ausente | Sin auditoria de foco, nombres accesibles ni contraste. |
| SET-01 | Apariencia | settings/appearance.py | settings_view.cpp | parcial | Varios controles no se reflejan totalmente en runtime. |
| SET-02 | Reproductor y audio | settings/player_settings.py | settings_view.cpp | parcial | Toggles sin motor real. |
| SET-03 | Letras y subtitulos | settings/subtitles.py | settings_view.cpp | parcial | Falta aplicar toda la configuracion al widget. |
| SET-04 | Cuentas e integraciones | settings/accounts.py | settings_view.cpp | parcial | Falta robustez de sesion y estados de error. |
| SET-05 | Almacenamiento, backup y restore | settings/storage.py | settings_view.cpp | funcional | Falta UX de errores y pruebas GUI. |
| SET-06 | Acerca de y actualizador | about.py, updater.py | updater.rs, dialogo C++ | parcial | Falta firma, endurecimiento y cobertura de distribuciones. |
| REL-01 | Paquetes instalables | packaging PyInstaller/AppImage parcial | Sin paquetes Doremi | ausente | Faltan AppImage/deb/rpm o estrategia acordada. |
| REL-02 | CI/CD | workflows parciales de Pyrolist | Sin pipeline Doremi | ausente | Faltan build, test, artefactos y releases. |
| REL-03 | Pruebas GUI | Sin suite completa | Sin suite | ausente | Necesario para elevar flujos a `verificado`. |

## Regla de mantenimiento

Toda implementacion que cambie un flujo debe actualizar su fila, evidencia y brecha en el mismo cambio. Subir a `verificado` requiere enlazar el test o anotar la ejecucion en `MANUAL_CASES.md`.
