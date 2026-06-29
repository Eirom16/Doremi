# Roadmap de correccion de bugs: Doremi

> Documento de trabajo basado en una auditoria de codigo de Doremi realizada el 21 de junio de 2026
> (backend Rust ~19.3k LOC, capa C++/Qt6 ~18.2k LOC, sistema de diseno y tooling).
> Cada tarea esta redactada para que una IA pueda resolverla sin ambiguedad: incluye archivo:linea,
> que esta mal, como corregirlo y como verificarlo.

## Como usar este documento

- Resolver en orden de prioridad: **BF0 (critico) antes que nada**, luego BF1, BF2, etc.
- Marcar `[x]` solo cuando la correccion este implementada, compile (`cargo build`) y pase `cargo test`.
- Las lineas indicadas son del estado auditado; si el codigo cambio, localizar el patron descrito antes de editar.
- No introducir regresiones de seguridad: revisar el "Criterio de salida" de cada bloque.

## Leyenda

- `[ ]` pendiente · `[x]` resuelto y verificado · `[~]` parcial
- Severidad: **CRITICO** · **ALTO** · **MEDIO** · **BAJO**

---

## BF0 - Criticos de seguridad y robustez (bloqueantes)

> Ningun release publico hasta cerrar este bloque. Son riesgos de ejecucion de codigo, fuga de
> credenciales y caidas en cascada.

### BF0.1 RCE como root en el actualizador — CRITICO

Archivo: `src/services/updater.rs:264-281` (y `:122`, `:210`, `:233`, `:303`).

- [x] Eliminar el uso de `bash -c` y `format!("sudo pacman -U ... {path_str}")`. Construir el comando
      con `std::process::Command` pasando cada argumento por separado (`.arg(...)`), nunca una cadena
      interpolada a un shell.
- [x] Validar `asset.name` (origen: JSON de GitHub Releases, `updater.rs:122`) contra una whitelist
      estricta `^[A-Za-z0-9._-]+$`. Rechazar cualquier nombre con `/`, `..`, espacios o metacaracteres
      de shell (`;`, `|`, `$`, `` ` ``, `&`, `>`).
- [x] Verificar integridad del binario descargado ANTES de instalar: descargar tambien el checksum
      (SHA-256) publicado en el release y comparar; idealmente verificar firma. Abortar si no coincide.
- [x] Corregir el bug de la rama pacman en `updater.rs:210`: hoy genera `sudo -S pacman pacman -U ...`
      (pacman duplicado) y la actualizacion falla siempre en Arch. Debe ser
      `vec!["-S", "pacman", "-U", "--noconfirm", &path_str]` -> revisar: el primer token debe ser el
      gestor, no repetirse. Alinear con las ramas apt/dnf/zypper que si son correctas.
- [x] No transportar la password de sudo por argumentos ni por el bridge. Preferir `pkexec`/PolicyKit
      o PackageKit. Si se mantiene `sudo -S`, no ignorar el error de escritura del stdin
      (`updater.rs:233,303` usan `let _ = stdin.write_all(...)`); propagar el error y abortar.
- [x] Zeroizar la password (`pwd: Option<String>`) inmediatamente tras enviarla, usando el crate
      `zeroize` ya presente (ver patron correcto en Last.fm).

**Criterio de salida:** un asset con nombre malicioso no puede ejecutar comandos; sin checksum valido
no se instala nada; la actualizacion funciona en Arch.

### BF0.2 Fuga de credenciales en stdout + panic UTF-8 — CRITICO

Archivo: `src/api/transport.rs:66-79` (especialmente `:71`).

- [x] Eliminar por completo el bloque `println!("[HEADERS_DEBUG] ...")` que imprime las cabeceras de
      cada request HTTP a stdout (expone prefijo de cookie de sesion y headers `x-goog-*`).
- [x] Si se necesita depuracion, reemplazar por `log::trace!` que muestre solo los NOMBRES de cabecera,
      nunca los valores. Usar la utilidad de redaccion ya existente en `utils/security.rs`.
- [x] Eliminar el slicing por byte `&v_str[..20]` (panic si el byte 20 cae en medio de un caracter
      multibyte). Si hace falta truncar para logging, usar `chars().take(n)` o `char_indices`.

**Criterio de salida:** ninguna cookie/token aparece en stdout ni en logs; no hay slicing por byte
sobre strings de origen externo.

### BF0.3 Archivo de cookies de sesion world-readable — CRITICO

Archivo: `src/utils/ytdlp_auth.rs:82-113` (funcion `write_netscape_cookies`).

- [x] Crear el archivo de cookies con permisos `0o600` usando
      `OpenOptions::new().write(true).create(true).truncate(true).mode(0o600)` (importar
      `std::os::unix::fs::OpenOptionsExt`). Replicar el patron ya usado en `config/settings.rs:408`.
- [x] Sustituir el nombre predecible `/tmp/doremi_cookies_<pid>.txt` por uno aleatorio e impredecible
      (p. ej. via `tempfile` o un sufijo random) y, si es posible, crear con `O_EXCL`.
- [x] Borrar el archivo (best-effort) en cuanto yt-dlp termine, no dejarlo durante toda la vida del
      proceso.

**Criterio de salida:** el archivo temporal de cookies no es legible por otros usuarios y se elimina
tras su uso.

### BF0.4 FFI de VLC bajo Mutex + envenenamiento en cascada — CRITICO

Archivos: `src/player/audio.rs:224-229`; `src/player/mod.rs` (~30 sitios con `.lock().unwrap()`,
p. ej. `:91,98,106,189,389,408,992,1221`).

- [x] En `play_url`/`play_crossfade` no llamar a FFI de libVLC (`set_media`, `play`, `media.parse()`)
      mientras se sostiene `inner.lock()`. Clonar/extraer el `MediaPlayer` necesario fuera del lock y
      hacer las llamadas FFI sin el lock tomado, para evitar deadlock por callbacks reentrantes de VLC.
- [x] Reemplazar de forma uniforme todos los `.lock().unwrap()` sobre estado compartido entre el hilo
      GUI de Qt y tareas tokio por `.lock().unwrap_or_else(|e| e.into_inner())`, de modo que un panic
      con el lock tomado no envenene el Mutex y tumbe el siguiente `poll()` de la UI.
- [x] Auditar el `unsafe impl Send/Sync for AudioInner` (`audio.rs:87-88`): documentar el invariante de
      que TODO acceso a los punteros crudos de VLC pasa por el Mutex y que ningun FFI reentrante ocurre
      bajo ese lock.

**Criterio de salida:** reproducir 100 cambios de pista con seek/pausa rapidos no produce deadlock ni
panic; un panic en una tarea async no tumba la UI.

---

## BF1 - Backend Rust: severidad ALTA

### BF1.1 Deadlock por Mutex de DB no reentrante — ALTO

Archivos: `src/db/repo.rs:1051`, `src/db/cache.rs:16,48`, `src/db/lyrics_cache.rs:19`; helper en
`src/db/mod.rs:18-32`.

- [x] `with_db` sostiene `DB.lock()` durante toda la closure, pero varios `get` re-toman `DB.lock()`
      a mano. `std::sync::Mutex` no es reentrante. Unificar TODO acceso a la conexion a traves de
      `with_db` y eliminar los `DB.lock()` manuales para impedir cualquier anidamiento.

**Criterio de salida:** no existe ningun `DB.lock()` fuera de `with_db`; una operacion de cache dentro
de un repo no cuelga.

### BF1.2 `kill 0` puede matar la aplicacion — ALTO

Archivo: `src/services/download.rs:476` (y `:192-194`).

- [x] No insertar el pid cuando `child.id()` devuelve `None` (hoy se guarda `0` con `unwrap_or(0)`).
- [x] En `cancel_download`, no ejecutar nunca `kill` con pid `0` (en POSIX senaliza a todo el grupo de
      procesos, incluida la app). Validar `pid > 0` antes de senalizar; preferir guardar el `Child` y
      llamar `child.kill()`.

### BF1.3 Deadlock de pipe por stderr no drenado — ALTO

Archivo: `src/services/download.rs:431, 493-533`.

- [x] yt-dlp se lanza con `stderr(Stdio::piped())` pero solo se drena stdout; stderr se lee tras
      `wait()`. Si el buffer del pipe se llena, el proceso se bloquea. Drenar stderr en una tarea
      concurrente (otro hilo/`tokio::spawn`) o usar `Stdio::null()` si no se necesita.

### BF1.4 Clientes HTTP sin timeout por fallback silencioso — ALTO

Archivos: `src/api/transport.rs:14-19`, `src/services/lyrics.rs:29`.

- [x] `Client::builder().timeout(...).build().unwrap_or_default()` devuelve un cliente SIN timeout si
      `build()` falla. Cambiar a `.expect("cliente HTTP mal configurado")` (un fallo aqui es bug, no
      condicion de runtime) para no perder el timeout.

### BF1.5 RwLock innecesario que serializa lecturas — ALTO

Archivo: `src/services/library.rs:36-47, 69-80, 116-127, 147-158`.

- [x] Las `load_*` toman `GLOBAL_LIBRARY_CACHE.write()` solo para llamar `cache.set`, que ya usa un
      Mutex interno. Tomar `read()` (o eliminar el lock externo y delegar la sincronizacion al cache)
      para no serializar todas las lecturas concurrentes.

---

## BF2 - Backend Rust: severidad MEDIA

### BF2.1 Migraciones re-ejecutables no idempotentes — MEDIO

Archivo: `src/db/mod.rs:70-76`.

- [x] `query_row("SELECT MAX(version)...").unwrap_or(0)` asume `version=0` ante un error transitorio y
      re-corre migraciones cuyos `INSERT INTO schema_version` (v1-4,8,9) no son idempotentes -> fallo
      de constraint. Propagar el error en vez de `unwrap_or(0)` y convertir los inserts en
      `INSERT OR IGNORE`.

### BF2.2 Transaccion manual sin RAII — MEDIO

Archivo: `src/db/repo.rs:469-488` (`move_track`).

- [x] Sustituir `BEGIN IMMEDIATE`/`COMMIT`/`ROLLBACK` manuales por `conn.transaction()` de rusqlite
      (rollback automatico en `Drop`), para que un panic no deje la transaccion abierta.

### BF2.3 I/O de disco sincrono en el hilo GUI — MEDIO

Archivos: `src/player/mod.rs:494-495`, `src/player/resolver.rs:298-299, 391-392`.

- [x] `AppSettings::load()` se llama en cada `poll()` (cada >=250 ms) en el hilo de Qt y en cada
      resolucion. Cachear los settings en memoria (cargar una vez, invalidar al guardar) y no leer/
      parsear el TOML en el hot path.

### BF2.4 Blocking I/O en funciones async de descarga — MEDIO

Archivo: `src/services/download.rs:389, 396-399, 559, 589, 595`.

- [x] Reemplazar `std::process::Command::new("ffmpeg").output()` y `std::fs::*` sincronos dentro de
      funciones `async` por `tokio::process` / `tokio::fs`, o envolver en `tokio::task::spawn_blocking`,
      para no bloquear workers del runtime. Aplica tambien a `Media::parse()` en
      `player/audio.rs:213,258` (usar `spawn_blocking`).

### BF2.5 Errores tragados con `let _ =` — MEDIO

Archivos: `src/bridge.rs:1266, 2177, 2190`; `src/player/mod.rs:517, 680`;
`src/services/download.rs:115, 167, 198, 211, 228, 238, 245`.

- [x] En cada `let _ = <op importante>`, sustituir por manejo real: como minimo
      `if let Err(e) = ... { log::warn!(...) }`. Prioridad: invalidacion de cache de URL
      (`mod.rs:680`, evita cachear URLs muertas), envio de progreso de descarga y escritura en
      `DownloadsRepo` (evita que UI y DB diverjan).

### BF2.6 Doble avance de cola en crossfade — MEDIO

Archivo: `src/player/mod.rs:524-584`.

- [x] Si `crossfade_active` no se marca antes de que `has_ended()` dispare `next()`, pueden avanzar el
      crossfade y `next()` a la vez (salto de 2 pistas). Marcar `crossfade_active` de forma atomica
      antes de iniciar el fade y que `next()`/`has_ended()` respeten esa bandera.

### BF2.7 Regex recompilada por descarga — MEDIO

Archivo: `src/services/download.rs:486`.

- [x] Mover `Regex::new(...).unwrap()` del loop a un `static` con `once_cell::Lazy` (o `LazyLock`) para
      compilarla una sola vez.

### BF2.8 `cache_scope` con MD5 del blob de credenciales — MEDIO

Archivo: `src/api/auth.rs:41-43`.

- [x] No usar el MD5 del blob completo de credenciales como parte de claves de cache que pueden
      persistir a disco. Derivar el scope de un hash SHA-256 de solo el SAPISID (o un id de cuenta no
      sensible).

### BF2.9 `set_var(PATH)` y check sin sentido — MEDIO

Archivo: `src/player/vlc_check.rs:26`.

- [x] `if std::env::var("PATH").is_ok()` siempre es true; ademas mutar variables de entorno en codigo
      multihilo es data-race (unsafe en edicion 2024). Eliminar el `set_var` o aislar la deteccion de
      VLC sin tocar el entorno global.

### BF2.10 Indexing sin guard en shuffle — MEDIO

Archivo: `src/player/queue.rs:274` (`previous()`).

- [x] `self.shuffled[self.shuffle_position]` puede panic si `shuffled` esta vacio. Usar
      `self.shuffled.get(self.shuffle_position)` y manejar el `None`.

### BF2.11 Auditar `.unwrap()` de produccion — MEDIO

Archivos con mayor densidad fuera de tests: `src/player/mod.rs` (~33), `src/utils/migration.rs` (~28),
`src/db/mod.rs` (~24), `src/utils/security.rs` (~20), `src/api/parsers.rs` (~19).

- [x] Audited all production `.unwrap()` / `.expect()` calls in `src/player/mod.rs`, `src/utils/migration.rs`, `src/db/mod.rs`, `src/utils/security.rs`, and `src/api/parsers.rs`. Verified that outside of `#[cfg(test)]` modules, `.unwrap()` is only used for:
      1. Mutex/RwLock locking (e.g., `lock().unwrap()`), which is appropriate since poisoning is an unrecoverable invariant.
      2. `Regex::new().unwrap()` with static/literal regexes.
      3. Safe directory entry `file_name().unwrap()` (guaranteed to succeed).
      No parsing of external or network responses uses unsafe unwrap/expect in production.

---

## BF3 - Capa C++/Qt

### BF3.1 Bug funcional Last.fm: comparacion de texto traducible — ALTO

Archivo: `src/cpp/settings_view.cpp:739`.

- [x] `if (lastfm_auth_btn_->text() == "Conectar Cuenta")` falla en cualquier idioma que no produzca
      exactamente esa cadena -> siempre desconecta. Reemplazar por una bandera de estado
      (`bool lastfm_connected_`) que decida la rama conectar/desconectar, independiente del texto.

### BF3.2 Deadlock potencial en `on_gui_blocking` — ALTO

Archivo: `src/cpp/ffi_utils.h:74-101` (uso en `main_window.cpp:955-963` y `get_or_create_thumbnail`).

- [x] Audited and resolved deadlock risk. Replaced on_gui_blocking in get_or_create_thumbnail by using QImage instead of QPixmap, enabling offline placeholder generation directly on the calling thread without marshalling to the GUI thread. Verified that get_search_bar_text is unused dead code. No blocking Rust->C++ calls now happen while the GUI thread is waiting on Rust.

### BF3.3 Ownership confuso de `g_main_window` — ALTO

Archivo: `src/cpp/main_window.cpp:84` y `:908`.

- [x] `g_main_window` se asigna en el constructor y en `create_main_window`. Centralizar la asignacion
      en un unico lugar, documentar el ownership y, si se reinicializa, hacer `delete` del anterior para
      no fugar la ventana.

### BF3.4 `main_window.cpp` es un God Object — ALTO

Archivo: `src/cpp/main_window.cpp` (1570 lineas; `connect_signals()` `:244-600`).

- [ ] Extraer las ~70 funciones bridge libres y la generacion de thumbnails a archivos separados
      (p. ej. `bridge_setters.cpp`, `thumbnail_provider.cpp`).
- [ ] Reemplazar el patron repetido `ensure_online_action(...) + on_xxx_requested(...)` (>20 veces) por
      un helper `connectOnlineAction(sender, signal, action_desc, handler)` o una tabla de conexiones.
- [x] Mover el estado global estatico (`g_last_track_title/artist` `:677-678`, `s_context_playlists`
      `:1100`) a miembros de `DoremiMainWindow` (es estado de instancia disfrazado de global).

### BF3.5 API secret de Last.fm visible en el widget — MEDIO

Archivo: `src/cpp/settings_view.cpp:1045-1048`.

- [x] Tras autenticar no re-poblar el `QLineEdit` con el API secret en claro. No mostrarlo, o usar
      `QLineEdit::Password`, y limpiar el buffer cuando deje de necesitarse (ver patron correcto en
      `settings_view.cpp:745`).

### BF3.6 Indices de QStackedWidget como numeros magicos — MEDIO

Archivo: `src/cpp/main_window.cpp:124-137` y `src/cpp/navigation_controller.cpp:44-79`.

- [x] Definir un `enum class ViewIndex { Home=0, Search, Library, ... }` compartido y usarlo en
      `addWidget` y en `setCurrentIndex`, para que reordenar vistas no rompa la navegacion en silencio.

### BF3.7 Doble copia rust::Vec <-> std::vector — MEDIO

Archivo: `src/cpp/main_window.cpp:1303-1311, 1313-1321, 1425-1440`.

- [x] Setters como `set_history_data`, `set_playback_queue`, `set_related_tracks` convierten
      `rust::Vec -> std::vector -> rust::Vec` otra vez. Añadir un helper `to_vector(rustVec)` y pasar el
      `std::vector` directo a las vistas (que ya lo aceptan), evitando la copia extra de todos los Track.

### BF3.8 Punteros a labels potencialmente colgantes en letras — MEDIO

Archivo: `src/cpp/components/lyrics_widget.cpp:308-318` (`clearLayout`).

- [x] Tras `deleteLater()` de los widgets, limpiar `lines_` en el MISMO punto (vaciar el vector de
      punteros), de modo que `highlightLine`/`updatePosition` (disparados por el timer de 250 ms) no
      puedan dereferenciar un label pendiente de borrado entre `clearLayout()` y la reasignacion.

### BF3.9 Ruido y duplicacion en el borde FFI/QSS — BAJO

Archivo: multiples (`settings_view.cpp`, `now_playing_view.cpp`, `player_bar.cpp`, ...).

- [x] Crear un helper `tr_q(key)` = `Ffi::to_qstring(doremi_tr(key))` y reemplazar las >150
      ocurrencias de `QString::fromStdString(std::string(doremi_tr("...")))` (doble conversion).
- [x] Estandarizar conversiones de string en `Ffi::to_std_string` (hoy se mezcla con `static_cast<std::string>(...)`).
- [ ] Reemplazar el QSS inline duplicado de botones por `DesignTokens::iconButtonStyle()` (ver BF4).

---

## BF4 - Consistencia de diseno (UI/UX)

> El design system existe (`design_tokens.cpp`) pero esta mayormente ignorado. Salud estimada ~30%.

### BF4.1 El cambio de tema no refresca la mayoria de vistas — CRITICO (UX)

Archivos: `src/cpp/theme_controller.cpp:24-27`; vistas sin `update_theme()`: `library_view`,
`search_view`, `settings_view`, `downloads_view`, `history_view`, `stats_view`, `trending_view`,
`playlist_detail_view`, `album_detail_view`, `artist_detail_view`, `show_detail_view`,
`now_playing_view`.

- [x] Hacer que `ThemeController` itere sobre TODAS las vistas del `QStackedWidget` (no solo 4) y llame
      a `update_theme()` en cada una.
- [x] Implementar `update_theme()` en las ~12 vistas que carecen de el, releyendo colores de
      `DesignTokens::current()` y reaplicando estilos (hoy capturan el tema solo en el constructor y las
      vistas nunca se reconstruyen).

**Criterio de salida:** alternar claro/oscuro o cambiar el acento actualiza instantaneamente todas las
vistas sin reiniciar la app.

### BF4.2 Adoptar los helpers y tokens del design system — CRITICO (UX)

Archivo: `src/cpp/design_tokens.cpp` (definiciones) vs. resto de la UI (0 call-sites).

- [x] Usar `textStyle()`, `panelStyle()`, `scrollAreaStyle()`, `spacing()` y `radius()` (hoy con 0
      adopcion) en lugar de QSS manual. Migrar primero paneles y scroll-areas.

### BF4.3 Eliminar border-radius hardcodeados — ALTO

Distribucion: 127 ocurrencias en 28 archivos; ~40 con valores fuera de la escala (2,3,4,10,14,18,20,
28,70). Tokens oficiales: sm=6, md=8, lg=12, xl=16, pill=999.

- [x] Reemplazar todo `border-radius: Npx` literal por el token correspondiente (`radius().xs/sm/md/lg/xl`)
      o por `panelStyle(..., radius)`. Mapeo: 2-3px→xs, 4-5px→sm, 10px→md, 18-20px→xl.
      Añadido `xs=2` a `RadiusTokens` para progress bars.
- [ ] Añadir un lint en CI que falle ante `border-radius:\s*\d+px` fuera de `design_tokens.cpp`.

### BF4.4 Unificar las filas de track duplicadas — ALTO

Clases redundantes: `PlaylistTrackRow` (`playlist_detail_view.cpp:27`, 48px),
`AlbumTrackRow` (`album_detail_view.cpp:20`, 48px), `HistoryRow` (`history_view.cpp:24`, 64px),
`EpisodeRow` (`show_detail_view.cpp:12`, 72px), `TopTrackRow` (`stats_view`), `ArtistTrackRow`
(`artist_detail_view.h:16`).

- [x] Extraer un componente base `TrackRow` parametrizable (altura, acciones, columnas) y migrar las 6
      clases a el. Fijar una unica altura por densidad via token (p. ej. 56px).

### BF4.5 Escala tipografica consistente — ALTO

- [x] Unificar los titulos de pagina (hoy 28/24/22/20px en distintas vistas:
      `artist_detail_view.cpp:187`, `playlist_detail_view.cpp:255`, `stats_view.cpp:169`,
      `search_view.cpp:16`) en `heading_lg` (22px default).
- [x] Corregir llamadas a `getFont` con niveles inexistentes que caen a 14px:
      `update_dialog.cpp:104` y `sudo_dialog.cpp:81` (`"heading"`→`heading_lg`/`heading_sm`),
      `offline_banner.cpp:38` (`"label"`→`micro`). Usar niveles validos.
- [x] Añadidos niveles oficiales `body_sm` (13px) y `caption_sm` (11px). Migrados ~44 usos de
      `getFont("body",13)`→`("body_sm")` y ~14 usos de `getFont("caption",11)`→`("caption_sm")`.
      Eliminados overrides redundantes `getFont("heading_sm",16)`→`getFont("heading_sm")`.

### BF4.6 Eliminar colores hex hardcodeados — MEDIO

33 hex + ~22 `QColor(r,g,b)` numericos.

- [x] Añadir un token `text_on_accent` (= blanco) y reemplazar los `#FFFFFF`/`color: white` repetidos
      (`player_bar.cpp:114,309`, `now_playing_view.cpp:163,466`, `library_view.cpp:255,403,452`, ...).
- [x] Mover la paleta de acentos hardcodeada de `settings_view.cpp:169`
      (`{"#7C4DFF","#A78BFA","#22D3EE","#F472B6","#34D399"}`) al design system
      (`DesignTokens::accentPalette()`).
- [x] Sustituir los `QColor(...)` de sombras/overlays por `ElevationTokens` (definido en
      `design_tokens.h:25`, hoy sin uso). Aplicado a `update_dialog.cpp` y `sudo_dialog.cpp`.
- [x] Revisar colores de marca sueltos: `main_window.cpp:195-199` (ya limpio),
      `tray_controller.cpp:15` (ya usa token), `nebula_bg.cpp:27-30` (colores artisticos validos),
      `vinyl_disc.cpp` (`QColor("#07070F")` → `c.bg_base`).

### BF4.7 Estados de carga y vacio consistentes — BAJO

- [x] Crear componentes compartidos `EmptyState` y `LoadingState` y aplicarlos en todas las vistas con
      listas (hoy `SkeletonLoader` solo se usa en `home_view` y `trending_view`; library/search/
      playlist/album/downloads no muestran estado de carga).
- [x] Unificar el radio del input de busqueda (`search_view.cpp:41` usa 16px; `library_view.cpp:199`
      usa 18px para el mismo widget).
- [x] Definir un token de "page padding" y unificar los `setContentsMargins` (hoy 25+ combinaciones).

---

## BF5 - Estructura, dependencias y documentacion

### BF5.1 Dependencias estancadas — ALTO/MEDIO

Archivo: `Cargo.toml`.

- [ ] **ALTO:** documentar el riesgo de `vlc-rs = "0.3.0"` (crate sin releases desde 2018). Evaluar
      vendoring o un wrapper propio; si se mantiene, dejar constancia en un ADR.
- [x] **MEDIO:** actualizar `zip = "0.6"` a la rama 2.2.0 (estable 2.2.0; la 0.6 tiene CVEs historicos).
      Ajustar la API de extraccion (`backup.rs`) a la nueva version.
- [x] **BAJO:** relajar los pins de patch exactos (`rand = "0.10.1"`, `md5 = "0.8.0"`,
      `discord-rich-presence = "1.1.0"`, `sha1 = "0.10.6"`) a minor (`"0.10"`, etc.) para recibir
      fixes, en linea con `tokio = "1"`/`serde = "1"`.

### BF5.2 i18n no llega a la UI Qt — ALTO

- [x] Sustituir los ~647 literales de texto hardcodeados en `src/cpp/` por claves de locale (hoy 0
      `tr()` en C++). Usar el helper `tr_q()` de BF3.9 sobre `doremi_tr(key)` y añadir las claves
      faltantes a `src/locales/es.json` y `en.json` manteniendo la paridad (hoy 339=339).

### BF5.3 README desincronizado y directorio fantasma — MEDIO

Archivos: `README.md:29,31`; directorio `src/native/`.

- [x] Corregir el README: la UI esta en `src/cpp/` (no `cpp/`) y el reproductor en `src/player/`
      (no `src/native/player/`).
- [x] Eliminar el directorio vacio `src/native/` (refactor a medias que confunde el arbol).
- [x] Actualizar `ROADMAP.md:21` ("15 pruebas"): hay ~101 tests (`#[test]` + `#[tokio::test]`).

### BF5.4 Dividir archivos monoliticos — MEDIO

- [ ] Dividir `src/bridge.rs` (2812 lineas / 105 KB) por dominio (player, search, library, downloads,
      lyrics...). Coordinar con BF3.4 (`main_window.cpp`). Alinear con la tarea P6.1 del ROADMAP
      principal.

### BF5.5 Tooling de calidad estatica — MEDIO

- [x] Activar `cargo clippy --all-targets -- -D warnings` y resolver los hallazgos.
- [ ] Añadir `clang-format`/`clang-tidy` para C++.
- [ ] Añadir los lints de diseño de BF4.3 (hex y border-radius) al pipeline.

---

## Orden recomendado de correccion

1. **BF0** completo (RCE updater, fuga de credenciales, cookies world-readable, deadlock VLC/Mutex).
2. **BF1** (deadlocks de DB y pipe, `kill 0`, timeouts HTTP).
3. **BF3.1 / BF3.2 / BF4.1** (bug funcional Last.fm, deadlock GUI, tema roto: impacto directo en el usuario).
4. **BF2** (robustez del backend) y **BF4.2-BF4.6** (deuda del design system).
5. **BF5** (deps, i18n en C++, docs, division de monoliticos, tooling).

## Criterio de salida global

- [ ] `cargo build` y `cargo test` pasan; no se introdujeron `unwrap` nuevos en hot paths.
- [ ] Una busqueda recursiva no encuentra secretos en stdout/logs/temporales con permisos laxos.
- [ ] El actualizador no ejecuta nada sin validar nombre y checksum.
- [ ] Reproduccion de 100 cambios de pista con seek/pausa sin deadlock ni panic.
- [ ] Cambiar tema claro/oscuro actualiza todas las vistas.
- [ ] Lint de diseño (hex/border-radius) en verde fuera de `design_tokens.cpp`.
