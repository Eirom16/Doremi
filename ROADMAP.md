# Roadmap maestro: Pyrolist -> Doremi

> Documento de trabajo basado en una auditoria del codigo de Pyrolist y Doremi realizada el 12 de junio de 2026.

## Objetivo

Portear toda la funcionalidad util de Pyrolist a una aplicacion nativa mantenible, segura y medible:

- Rust: dominio, red, persistencia, reproduccion, concurrencia e integraciones.
- C++/Qt 6: interfaz, animaciones, accesibilidad e integracion de escritorio.
- `cxx`: contratos tipados entre Rust y C++, sin protocolos de strings ambiguos.
- Linux primero, con una arquitectura que no bloquee Windows o macOS en el futuro.

El port se considera terminado cuando Doremi cubra los flujos reales de Pyrolist sin mocks, pueda migrar los datos existentes, tenga pruebas automatizadas suficientes, paquetes instalables y metricas que demuestren la mejora de rendimiento.

## Estado actual resumido

### Base disponible

- [x] El proyecto compila con `cargo check --all-targets`.
- [x] Las 15 pruebas actuales pasan con `cargo test --all-targets`.
- [x] Qt 6 Widgets y WebEngine integrados mediante `build.rs` y `cxx`.
- [x] Reproductor VLC, cola, shuffle, repeat, seek y volumen.
- [x] MPRIS, atajos multimedia y bandeja del sistema.
- [x] SQLite con migraciones iniciales, favoritos, historial, playlists, cache y descargas.
- [x] Vistas base de inicio, busqueda, biblioteca, tendencias, descargas, historial, estadisticas, album, artista, playlist, ajustes y reproduccion actual.
- [x] Letras basicas y sincronizadas con LRCLIB.
- [x] Ecualizador VLC, temporizador de apagado, Last.fm, Discord RPC y actualizador.
- [x] Temas, componentes animados, i18n inicial ES/EN, backup y restore.

### Parcial o incompleto

- [~] API de YouTube Music: busqueda e inicio existen, pero dependen de parsing fragil y caen a datos mock.
- [~] Login: captura cookies mediante WebEngine, pero persiste headers en texto plano.
- [~] Pantallas de detalle: tienen presentacion basica, pero faltan varias acciones y datos de Pyrolist.
- [~] Descargas: funciona por proceso externo `yt-dlp`, sin cola observable, progreso, cancelacion, reintentos, albums o playlists completos.
- [~] Biblioteca: favoritos locales basicos; falta paridad con biblioteca remota y gestion completa de playlists.
- [~] Ajustes: varios controles existen, pero no todas las opciones afectan realmente al motor.
- [~] i18n: los locales de Doremi tienen menos cobertura y quedan numerosos textos C++ hardcodeados.
- [~] Estadisticas e historial: funcionalidad base, sin toda la semantica y riqueza visual de Pyrolist.
- [~] Actualizador: implementacion Linux inicial, pendiente de endurecimiento, firmas y cobertura de distribuciones.

### Ausente o con regresion respecto a Pyrolist

- [ ] Migracion automatica de configuracion, base de datos, cache y descargas de Pyrolist.
- [x] Hecho - Almacenamiento seguro en Secret Service/KWallet/keyring para YouTube Music y Last.fm.
- [ ] Cache real de artwork y cache persistente de letras.
- [ ] Prefetch de letras y precarga robusta del siguiente stream.
- [ ] Monitor de conectividad, banner offline y recuperacion automatica.
- [ ] Busqueda global avanzada, filtros completos, sugerencias e historial interactivo.
- [ ] Menus contextuales completos: reproducir despues, agregar a cola, playlist, favorito y descarga.
- [ ] Crear, editar y borrar playlists; sincronizacion con YouTube Music.
- [ ] Like/unlike remoto y reconciliacion con favoritos locales.
- [ ] Descarga y gestion completa de albums y playlists.
- [ ] Mini player flotante equivalente y modo compacto pulido.
- [ ] Notificaciones dentro de la app equivalentes al centro de notificaciones de Pyrolist.
- [ ] Instaladores, paquetes, CI/CD, pruebas GUI y documentacion operativa.

## Principios de priorizacion

1. Seguridad y datos del usuario antes que apariencia.
2. Reproduccion real y estable antes que nuevas funciones.
3. Eliminar mocks antes de ampliar superficies que dependen de ellos.
4. Contratos tipados y pruebas antes de aumentar la complejidad del bridge.
5. Paridad con Pyrolist antes de declarar mejoras exclusivas de Doremi.
6. Medir rendimiento; no asumir que Rust/C++ es mas rapido por si solo.

## P0 - Seguridad, datos y cimientos bloqueantes

### P0.1 Inventario y contrato de paridad

- [x] Crear una matriz versionada de todos los flujos de Pyrolist y su equivalente en Doremi.
- [x] Clasificar cada flujo como `ausente`, `UI solamente`, `parcial`, `funcional` o `verificado`.
- [x] Definir casos manuales de referencia para login, busqueda, play, cola, letras, descarga, biblioteca y cierre.
- [x] Capturar screenshots y comportamiento de Pyrolist como referencia visual.
- [x] Definir una politica clara para funciones de Pyrolist que se eliminaran deliberadamente.

**Criterio de salida:** ningun modulo puede considerarse porteado solo porque existe un archivo o widget con el mismo nombre.

### P0.2 Secretos y autenticacion

- [x] Hecho - Integrar Secret Service mediante una abstraccion nativa por plataforma.
- [x] Hecho - Mover API key, API secret y session key de Last.fm fuera de `settings.toml`.
- [x] Hecho - Mover cookies/headers de YouTube Music fuera de `headers_auth.json`.
- [x] Hecho - Aplicar permisos `0600` durante cualquier migracion temporal y escritura de ajustes/perfil.
- [x] Hecho - Eliminar archivos legacy de secretos solo despues de confirmar la migracion.
- [x] Hecho - Evitar passwords de Last.fm en logs, errores, bridge o memoria mas tiempo del necesario.
- [x] Hecho - Redactar headers, cookies, tokens, URLs firmadas y passwords en logging.
- [x] Hecho - Añadir logout que borre credenciales, estado de sesion y artefactos temporales.
- [x] Hecho - Definir fallback seguro cuando no exista un keyring disponible: fallar cerrado sin persistir texto plano.
- [x] Hecho - Añadir pruebas de que TOML, DB, backups y logs no contienen secretos.

**Criterio de salida:** una busqueda recursiva en datos persistidos no encuentra secretos en texto plano.

### P0.3 Migracion de Pyrolist a Doremi

- [x] Hecho - Detectar las rutas XDG legacy de `pyrolist` y las nuevas de Doremi.
- [x] Hecho - Versionar el formato de migracion y hacerla idempotente.
- [x] Hecho - Migrar `settings.toml`, normalizando campos incompatibles.
- [x] Hecho - Migrar base de datos: canciones, likes, conteos, ultima reproduccion, historial y descargas.
- [x] Hecho - Migrar artwork, letras y archivos descargados sin duplicarlos.
- [x] Hecho - Migrar secretos desde keyring usando el servicio/cuenta legacy.
- [x] Hecho - Conservar timestamps y metadata cuando sea posible.
- [x] Hecho - Verificar espacio disponible antes de copiar archivos.
- [x] Hecho - Usar transaccion y rollback para datos estructurados.
- [x] Hecho - Crear backup automatico previo a la migracion.
- [x] Hecho - Mostrar resumen: importados, omitidos, duplicados y errores.
- [x] Hecho - Permitir reintentar sin corromper el destino.
- [x] Hecho - Probar migracion desde una instalacion vacia, parcial y dañada.

**Criterio de salida:** un usuario existente abre Doremi y conserva biblioteca, historial, ajustes, descargas y sesiones sin intervencion manual.

### P0.4 Contratos Rust/C++

- [x] Sustituir payloads concatenados como `title - artist` o strings delimitados por structs compartidos.
- [x] Definir IDs estables para track, album, artist, playlist y descarga.
- [x] Separar IDs de presentacion y texto traducido.
- [x] Introducir DTOs tipados para resultados, detalles, cola, historial, stats y progreso.
- [x] No resolver acciones buscando por titulo; usar IDs.
- [x] Añadir version o compatibilidad al contrato si el bridge crece.
- [x] Centralizar conversiones QString/string y manejo de errores FFI.
- [x] Documentar ownership, thread affinity de Qt y callbacks permitidos.
- [x] Garantizar que toda mutacion Qt ocurra en el hilo de GUI.
- [x] Probar strings vacios, Unicode, listas grandes e IDs invalidos.

**Criterio de salida:** las acciones del usuario no dependen de parsear texto visible.

### P0.5 Modelo de concurrencia y errores

- [x] Documentar el runtime Tokio y el hilo principal Qt.
- [x] Eliminar operaciones HTTP, filesystem o procesos bloqueantes del hilo GUI.
- [x] Sustituir `Mutex` globales propensos a bloqueo por ownership/canales donde convenga.
- [x] Añadir cancelacion al cerrar la app y al cambiar de pantalla.
- [x] Definir timeouts, reintentos con backoff y limites de concurrencia por servicio.
- [x] Propagar errores tipados desde Rust hasta mensajes accionables en Qt.
- [x] Separar errores recuperables, autenticacion, red, dependencia externa y bug interno.
- [x] Añadir panic hook y cierre controlado de VLC, DB, Discord, MPRIS y workers.
- [x] Revisar todos los `unwrap`, locks contaminados y tareas `spawn` sin seguimiento.

## P1 - Reproduccion real y confiable

### P1.1 Extraccion de streams

- [x] Definir un `StreamResolver` desacoplado del reproductor.
- [x] Resolver audio de forma robusta para videos musicales, uploads, age-restricted y contenido regional.
- [x] Respetar calidad configurada y seleccionar codec compatible con VLC.
- [x] Guardar expiracion de URL y renovar antes de reproducir.
- [x] Reintentar resolucion cuando una URL expira o devuelve 403.
- [x] Usar autenticacion cuando sea necesaria y modo anonimo cuando no.
- [x] Añadir fallback controlado a `yt-dlp`, sin convertirlo en dependencia invisible.
- [x] Cachear respuestas brevemente, nunca URLs firmadas mas alla de su expiracion.
- [x] Añadir telemetria local: tiempo de resolucion, fallos y fallback usado.
- [x] Probar una matriz de formatos y tipos de contenido.

### P1.2 Estado del reproductor

- [x] Convertir playback en una maquina de estados explicita.
- [x] Manejar `idle`, `resolving`, `buffering`, `playing`, `paused`, `ended`, `failed` y `offline`.
- [x] Evitar carreras entre doble click, next rapido, seek y resoluciones tardias.
- [x] Asociar cada carga a un `play_id` para ignorar callbacks obsoletos.
- [x] Distinguir stop solicitado, fin natural y error VLC.
- [x] Reintentar errores transitorios sin saltar silenciosamente de pista.
- [x] Persistir volumen, pista, posicion y cola de sesion.
- [x] Restaurar sesion segun `resume_on_startup`.
- [x] Implementar `stop_on_close` de forma consistente con tray.
- [x] Sincronizar estado entre player bar, now playing, tray, MPRIS y Discord.

### P1.3 Cola y autoplay

- [ ] Completar add next, add to end, remove, move, clear y jump.
- [ ] Implementar drag and drop en el panel de cola.
- [ ] Preservar correctamente cola original al activar/desactivar shuffle.
- [ ] Probar repeat off/all/one en bordes y cola vacia.
- [ ] Construir auto queue usando radio/related real de YouTube Music.
- [ ] Evitar duplicados excesivos y contenido no reproducible.
- [ ] Precargar metadata, artwork, letras y stream de la siguiente pista.
- [ ] Cancelar precarga cuando cambia la cola.
- [ ] Persistir y restaurar la cola.

### P1.4 Audio avanzado

- [ ] Verificar ecualizador de 10 bandas en todos los builds VLC soportados.
- [ ] Exponer preamp, cada banda, presets y reset desde Qt.
- [ ] Implementar crossfade real entre dos pipelines; el fade de volumen simple no es crossfade.
- [ ] Verificar gapless y documentar limitaciones de VLC.
- [ ] Implementar normalizacion de volumen o retirar temporalmente el toggle.
- [ ] Implementar skip silence o retirar temporalmente el toggle.
- [ ] Aplicar cambios de audio sin reiniciar la pista cuando sea posible.
- [ ] Evitar clipping al combinar preamp, normalizacion y EQ.
- [ ] Añadir pruebas y mediciones de CPU para EQ/crossfade.

### P1.5 Controles del sistema

- [ ] Emitir cambios MPRIS de propiedades y metadata, no solo responder getters.
- [ ] Implementar correctamente `Raise`, `Quit`, `OpenUri`, `CanSeek` y `SetPosition`.
- [ ] Respetar el toggle `mpris_enabled` al iniciar y detener el servicio.
- [ ] Verificar media keys en GNOME, KDE y Wayland.
- [ ] Completar acciones de tray: play/pause, anterior, siguiente, mostrar y salir.
- [ ] Definir comportamiento de cierre: minimizar al tray o salir.
- [ ] Añadir instancia unica y forwarding de argumentos/URLs.

## P2 - API de YouTube Music sin mocks

### P2.1 Cliente y robustez

- [x] Hecho - Eliminar resultados mock del flujo de produccion.
- [x] Hecho - Mostrar estados vacio, offline y error reales en UI.
- [ ] Separar transporte HTTP, autenticacion, endpoints y parsing.
- [ ] Reutilizar `reqwest::Client`; no crear uno por request.
- [ ] Usar API async en servicios y evitar `reqwest::blocking`.
- [ ] Añadir headers/contexto por locale, region y sesion.
- [ ] Manejar continuations y paginacion.
- [ ] Añadir cache con TTL e invalidacion por usuario/locale.
- [ ] Conservar fixtures JSON anonimizados para tests de parsers.
- [ ] Detectar cambios de schema con errores descriptivos.
- [ ] Añadir rate limiting y backoff.

### P2.2 Endpoints necesarios

- [ ] Search con filtros all/songs/videos/albums/artists/playlists.
- [ ] Search suggestions y busquedas recientes.
- [ ] Home autenticado y anonimo.
- [ ] Trending/charts reales por region.
- [ ] Album detail y tracks.
- [ ] Artist detail, top songs, albums, singles y related.
- [ ] Playlist detail, continuations y disponibilidad.
- [ ] Related/radio/watch playlist para autoplay.
- [ ] Library songs, albums, artists, playlists y subscriptions.
- [ ] Like/unlike y estado de like.
- [ ] Crear, editar, borrar playlists.
- [ ] Añadir y eliminar canciones de playlists.
- [ ] Historial remoto cuando la cuenta lo permita.
- [ ] Perfil y estado de autenticacion.

### P2.3 Login y sesion

- [ ] Endurecer captura de cookies en WebEngine.
- [ ] Limitar navegacion a dominios esperados durante login.
- [ ] Detectar exito, cancelacion, expiracion y challenge.
- [ ] Construir headers SAPISIDHASH correctamente cuando aplique.
- [ ] Renovar sesion sin pedir login mientras sea posible.
- [ ] Detectar sesion revocada y degradar limpiamente a modo anonimo.
- [ ] Limpiar perfil/cookies WebEngine al cerrar sesion.
- [ ] Añadir tests de serializacion segura de sesion.

## P3 - Paridad de pantallas y flujos

### P3.1 Inicio y tendencias

- [ ] Renderizar cards por tipo real, no labels planos.
- [ ] Cargar thumbnails asincronamente con cache y placeholders.
- [ ] Abrir song, album, artist y playlist segun su tipo.
- [ ] Implementar skeleton, error, retry y empty state.
- [ ] Añadir continuations o carga incremental.
- [ ] Conservar scroll y contenido al navegar atras.
- [ ] Actualizar contenido por sesion, locale y region.

### P3.2 Busqueda

- [ ] Portar chips de filtros y categorias completas.
- [ ] Añadir debounce y cancelacion de consultas anteriores.
- [ ] Mostrar top result y secciones diferenciadas.
- [ ] Portar sugerencias y busqueda global.
- [ ] Integrar historial: abrir, eliminar una entrada y limpiar todo.
- [ ] Navegacion completa desde cada resultado.
- [ ] Menus contextuales por tipo.
- [ ] Accesibilidad y navegacion total con teclado.

### P3.3 Biblioteca

- [ ] Separar biblioteca local, remota y descargas.
- [ ] Portar tabs de canciones, albums, artistas y playlists.
- [ ] Sincronizar favoritos locales y likes remotos.
- [ ] Crear playlist con titulo, descripcion y privacidad.
- [ ] Editar metadata y borrar playlists con confirmacion.
- [ ] Añadir/eliminar/reordenar tracks en playlist.
- [ ] Implementar cache por tab e invalidacion despues de mutaciones.
- [ ] Añadir sort, filter y busqueda dentro de biblioteca.
- [ ] Estados de no autenticado y vacio con acciones claras.

### P3.4 Album, artista y playlist

- [ ] Mostrar metadata completa, artwork, duracion y disponibilidad.
- [ ] Play all y shuffle all construyendo una cola tipada.
- [ ] Like/favorite y estado persistente.
- [ ] Descargar track, album o playlist con progreso agregado.
- [ ] Menus contextuales por track.
- [ ] Navegar entre artista, album y playlist.
- [ ] Artist: top tracks, albums, singles y related.
- [ ] Playlist: owner, descripcion, privacy y continuations.
- [ ] Manejar tracks eliminados, privados o no disponibles.

### P3.5 Now playing, letras y relacionados

- [ ] Mantener player bar y full player perfectamente sincronizados.
- [ ] Portar tabs de lyrics, queue y related.
- [ ] Parsear LRC con timestamps mejorados y metadatos.
- [ ] Aplicar delay manual, alineacion, spacing, colores y animacion elegida.
- [ ] Auto scroll configurable y click para seek a una linea.
- [ ] Fallback entre letra sincronizada, plana y no encontrada.
- [ ] Cache persistente de letras con TTL/version.
- [ ] Prefetch de la siguiente pista y cancelacion.
- [ ] Mostrar related real y permitir agregar/reproducir.
- [ ] Mejorar fondo dinamico desde artwork sin bloquear UI.
- [ ] Exponer like, download, repeat, shuffle y queue actions.

### P3.6 Historial y estadisticas

- [ ] Unificar semantica de una reproduccion contabilizada.
- [ ] Guardar progreso escuchado y evitar contar skips accidentales.
- [ ] Conservar play count, last played, album y thumbnail.
- [ ] Agrupar historial por fecha y permitir limpiar/eliminar.
- [ ] Reproducir y descargar desde historial usando ID.
- [ ] Stats: tiempo total, plays, artistas unicos, actividad semanal y top tracks.
- [ ] Añadir rangos 7 dias, 30 dias, año y todo.
- [ ] Optimizar queries e indices para historiales grandes.
- [ ] Exportar estadisticas a JSON/CSV opcionalmente.

## P4 - Descargas y experiencia offline

### P4.1 Gestor de descargas

- [ ] Modelar estados queued/resolving/downloading/postprocessing/completed/failed/cancelled.
- [ ] Exponer progreso, velocidad, ETA y error a Qt.
- [ ] Implementar pause/cancel/retry cuando la herramienta lo permita.
- [ ] Controlar concurrencia mediante semaphore, no polling de mutex.
- [ ] Capturar stdout/stderr estructuradamente de `yt-dlp`.
- [ ] Detectar ausencia/version incompatible de `yt-dlp` y `ffmpeg`.
- [ ] Permitir ubicacion, formato y calidad configurables.
- [ ] Sanitizar nombres de forma portable y resolver colisiones.
- [ ] Escribir a archivo temporal y hacer rename atomico al completar.
- [ ] Limpiar parciales tras cancelacion o crash segun politica.
- [ ] Guardar metadata y artwork local.
- [ ] Verificar existencia/integridad antes de marcar completado.

### P4.2 Albums y playlists

- [ ] Crear tareas padre e hijas para albums/playlists.
- [ ] Mostrar progreso agregado y por pista.
- [ ] Reanudar lote parcialmente descargado.
- [ ] Evitar redescargas y ofrecer reparar metadata.
- [ ] Preservar orden original.
- [ ] Borrar track o coleccion, con opcion de mantener archivos.
- [ ] Reconciliar DB cuando archivos son movidos o borrados externamente.

### P4.3 Offline

- [ ] Monitor de conectividad con debounce.
- [ ] Banner offline y notificaciones de reconexion.
- [ ] Resolver automaticamente a archivo local antes de red.
- [ ] Permitir navegar biblioteca, descargas, historial y cache sin conexion.
- [ ] Desactivar o explicar acciones que necesitan red.
- [ ] Reintentar cargas pendientes al recuperar conexion.
- [ ] Probar inicio completamente offline.

## P5 - Calidad visual, UX y accesibilidad

### P5.1 Sistema de diseño

- [ ] Consolidar tokens de color, tipografia, spacing, radius, elevacion y motion.
- [ ] Eliminar estilos duplicados y colores hardcodeados.
- [ ] Definir componentes oficiales y variantes de estado.
- [ ] Portar los componentes faltantes de Pyrolist solo cuando tengan uso real.
- [ ] Añadir estados hover, pressed, focused, disabled, loading y selected.
- [ ] Verificar dark/light y colores de acento con contraste adecuado.
- [ ] Soportar escala HiDPI y distintos device pixel ratios.
- [ ] Adaptar layouts a ventanas pequeñas y ultrawide.
- [ ] Reducir movimiento cuando el sistema lo solicite.

### P5.2 Assets e imagenes

- [ ] Implementar `ArtworkCache` en Rust con deduplicacion y limites.
- [ ] Descargar, decodificar y escalar fuera del hilo GUI.
- [ ] Usar formatos/tamaños apropiados para cada widget.
- [ ] Añadir cache LRU y limpieza por cuota.
- [ ] Evitar regenerar placeholders en disco innecesariamente.
- [ ] Extraer colores dominantes con cache por artwork.
- [ ] Medir blur, escalado y fondos animados; usar GPU solo si aporta.
- [ ] Empaquetar iconos y assets sin depender del directorio de desarrollo.

### P5.3 Interaccion

- [ ] Mini player flotante y modo compacto.
- [ ] Navegacion atras/adelante con historial de rutas.
- [ ] Restaurar scroll y foco al regresar.
- [ ] Tooltips consistentes y shortcuts visibles.
- [ ] Drag and drop para cola y playlists.
- [ ] Toasts accionables con cola y deduplicacion.
- [ ] Centro de notificaciones opcional.
- [ ] Confirmaciones solo en acciones destructivas.
- [ ] Errores con retry y detalle tecnico desplegable.

### P5.4 Accesibilidad e i18n

- [ ] Sustituir todos los textos C++ hardcodeados por claves de locale.
- [ ] Alcanzar paridad de claves ES/EN y testear claves faltantes.
- [ ] Soportar pluralizacion y parametros sin concatenar frases.
- [ ] Preparar layout para traducciones mas largas y RTL futuro.
- [ ] Definir accessible names/descriptions para botones de icono.
- [ ] Orden de tab correcto y foco visible.
- [ ] Operacion completa sin mouse.
- [ ] Contraste WCAG AA para texto y controles esenciales.
- [ ] Probar lectores de pantalla via AT-SPI.

## P6 - Arquitectura, pruebas y rendimiento

### P6.1 Arquitectura mantenible

- [ ] Dividir `bridge.rs` y `main_window.cpp` por dominio.
- [ ] Separar use cases de detalles de Qt, VLC, HTTP y SQLite.
- [ ] Introducir traits para API, stream resolver, secrets, downloads y clock.
- [ ] Evitar singletons globales salvo recursos de proceso justificados.
- [ ] Añadir estado de aplicacion central con eventos tipados.
- [ ] Definir politica de cache y fuente de verdad por entidad.
- [ ] Mantener C++ enfocado en presentacion; reglas de negocio en Rust.
- [ ] Añadir ADRs para decisiones importantes.

### P6.2 Pruebas

- [ ] Unit tests de cola: shuffle, repeat, remove, move y limites.
- [ ] Unit tests de state machine del player.
- [ ] Unit tests de parsing Innertube usando fixtures.
- [ ] Unit tests de LRC, limpieza de metadata y seleccion de letras.
- [ ] Unit tests de settings, secretos y migraciones.
- [ ] Unit tests de sanitizacion y estado de descargas.
- [ ] Integration tests de SQLite con migraciones desde cada version.
- [ ] Integration tests del bridge usando un adapter/mock de UI.
- [ ] Integration tests HTTP con servidor local mock.
- [ ] Tests de procesos para `yt-dlp`/`ffmpeg` simulados.
- [ ] Tests Qt con `QTest` para navegacion y señales principales.
- [ ] Smoke test headless de arranque y cierre.
- [ ] E2E manual automatizable: login, search, play, next, lyrics y download.
- [ ] Sanitizers para C++ y FFI en CI.
- [ ] Miri/loom donde aporte valor al codigo Rust concurrente.
- [ ] Cobertura reportada por modulo, sin perseguir un porcentaje vacio.

### P6.3 Calidad estatica

- [ ] `cargo fmt --check` obligatorio.
- [ ] `cargo clippy --all-targets -- -D warnings` obligatorio y estable.
- [ ] Formateador y linter C++ (`clang-format`, `clang-tidy`).
- [ ] Detectar dependencias vulnerables y licencias incompatibles.
- [ ] Revisar `unsafe` y documentar invariantes de VLC/FFI.
- [ ] Evitar warnings de Qt, moc y compilador C++.
- [ ] Pre-commit opcional con checks rapidos.

### P6.4 Benchmarks y presupuestos

- [ ] Medir baseline Pyrolist y Doremi en el mismo equipo.
- [ ] Medir cold start y warm start.
- [ ] Medir tiempo hasta primera ventana interactiva.
- [ ] Medir busqueda hasta primer resultado.
- [ ] Medir click-to-audio y cambio de pista.
- [ ] Medir RAM idle, reproduciendo y con biblioteca grande.
- [ ] Medir CPU idle, audio, letras animadas y fondos.
- [ ] Medir tamaño instalado y del binario.
- [ ] Medir scroll con 1000 items y cache de artwork.
- [ ] Añadir benchmarks de parser, DB y color extraction.
- [ ] Definir presupuestos de regresion para CI/release.

## P7 - Distribucion, operaciones y lanzamiento

### P7.1 Empaquetado Linux
- [ ] Crear un workflow en Githib actions para compilar y lanzar los relases.
- [ ] Crear icono y metadata finales de Doremi.
- [ ] Archivo `.desktop`, AppStream y MIME/scheme si aplica.
- [ ] Paquete Debian/Ubuntu.
- [ ] PKGBUILD para Arch.
- [ ] Flatpak con permisos minimos y portales.
- [ ] Evaluar AppImage para distribucion portable.
- [ ] Empaquetar Qt plugins, WebEngine resources, VLC y assets correctamente.
- [ ] Detectar dependencias al arrancar con mensajes accionables.
- [ ] Probar instalacion, actualizacion y desinstalacion limpia.

### P7.2 CI/CD

- [ ] Pipeline de fmt, clippy, tests Rust y build C++/Qt.
- [ ] Cache de Cargo sin ocultar errores de generacion moc/cxx.
- [ ] Matriz Ubuntu LTS y una distribucion rolling.
- [ ] Tests headless con Xvfb/Wayland virtual.
- [ ] Sanitizers en job separado.
- [ ] Build de paquetes reproducibles.
- [ ] Generacion de checksums y SBOM.
- [ ] Firma de tags, artefactos y metadata de actualizacion.
- [ ] Publicacion de draft release y promocion manual.
- [ ] Canal estable y prerelease.

### P7.3 Actualizador seguro

- [ ] No instalar paquetes solo por URL y password.
- [ ] Verificar firma criptografica y checksum antes de instalar.
- [ ] Validar repositorio, asset, arquitectura y formato esperado.
- [ ] Usar PackageKit/pkexec o mecanismo nativo cuando sea posible.
- [ ] No transportar password sudo por el bridge ni argumentos de proceso.
- [ ] Soportar cancelacion, rollback o instrucciones claras si falla.
- [ ] Adaptar comportamiento a deb, rpm, Arch, Flatpak y AppImage.
- [ ] Probar downgrade bloqueado y versiones prerelease.

### P7.4 Observabilidad y soporte

- [ ] Logging estructurado con rotacion y niveles configurables.
- [ ] Redaccion automatica de datos sensibles.
- [ ] Dialogo para exportar diagnostico sin secretos.
- [ ] Incluir version, commit, Qt, VLC, distro y backend grafico.
- [ ] Crash reports opt-in, nunca obligatorios.
- [ ] Plantillas de issues y guia de reproduccion.
- [ ] Changelog mantenido y notas de migracion.

## P8 - Mejoras exclusivas de Doremi despues de la paridad

Estas tareas no deben desplazar P0-P4.

- [ ] Smart playlists locales por reglas.
- [ ] Recomendaciones locales basadas en historial, sin subir datos.
- [ ] Cross-device sync opcional y cifrado.
- [ ] Vista de letras tipo karaoke con word timing cuando exista.
- [ ] Normalizacion ReplayGain/loudness scan para archivos locales.
- [ ] Importacion de biblioteca local y carpetas vigiladas.
- [ ] Edicion de metadata local y artwork.
- [ ] Cola colaborativa en LAN opcional.
- [ ] Scrobbling a servicios adicionales mediante plugins.
- [ ] Sistema de plugins con API versionada y sandbox considerada.
- [ ] Command palette y acciones globales.
- [ ] Temas exportables/importables con validacion.
- [ ] Visualizador de audio opcional con presupuesto estricto de CPU.
- [ ] Soporte Windows y macOS tras aislar integraciones de plataforma.

## Orden recomendado de entregas

### Hito 1 - Base segura

P0 completo: secretos, migracion, bridge tipado, errores y concurrencia documentada.

### Hito 2 - Reproductor confiable

P1 completo y smoke tests: reproducir 100 cambios de pista, seek, pause, resume, cola, cierre y restauracion sin bloqueos.

### Hito 3 - Catalogo real

P2 completo para search, home, detalles, related y biblioteca. Cero mocks visibles en produccion.

### Hito 4 - Paridad funcional

P3 y P4: todos los flujos diarios de Pyrolist disponibles, incluidos offline, letras, playlists y descargas por lotes.

### Hito 5 - Calidad profesional

P5 y P6: UX consistente, accesibilidad, cobertura automatizada y objetivos de rendimiento medidos.

### Hito 6 - Release publica

P7: paquetes firmados, actualizacion segura, CI/CD y documentacion. Lanzar primero como beta y despues estable.

### Hito 7 - Diferenciacion

Seleccionar P8 usando impacto, complejidad y datos de uso; no por cantidad de features.

## Definition of Done por tarea

Una tarea solo se marca completa cuando:

- [ ] Tiene comportamiento implementado, no solo UI.
- [ ] Maneja loading, exito, vacio, error, cancelacion y offline cuando aplica.
- [ ] No bloquea el hilo Qt.
- [ ] Usa IDs y contratos tipados.
- [ ] Tiene pruebas proporcionales al riesgo.
- [ ] Esta traducida en ES/EN.
- [ ] Es usable con teclado y tiene foco accesible.
- [ ] No persiste ni registra secretos.
- [ ] Tiene logging util sin ruido.
- [ ] Fue verificada en build debug y release.
- [ ] La documentacion relevante fue actualizada.

## Checklist de release beta

- [ ] Migracion real probada con copia de datos de Pyrolist.
- [ ] Cero secretos en texto plano.
- [ ] Cero mocks en experiencia de usuario.
- [ ] Reproduccion continua de al menos 8 horas sin crecimiento anormal de memoria.
- [ ] Login, logout y expiracion de sesion probados.
- [ ] Busqueda, home, album, artista, playlist y library probados.
- [ ] Cola, shuffle, repeat, seek, MPRIS, media keys y tray probados.
- [ ] Letras, stats, historial, Last.fm y Discord probados.
- [ ] Descargas individuales y por lote, offline y borrado probados.
- [ ] Backup y restore probados entre versiones.
- [ ] Paquete probado en GNOME/Wayland y KDE/Wayland o X11.
- [ ] Sanitizers y CI sin fallos.
- [ ] Actualizacion firmada o actualizador deshabilitado hasta que sea seguro.
- [ ] Known issues publicados.

## Siguiente bloque de trabajo recomendado

1. Implementar almacenamiento seguro y eliminar persistencia plana de secretos.
2. Crear migrador idempotente Pyrolist -> Doremi con pruebas.
3. Introducir DTOs tipados en `cxx` para tracks y acciones por ID.
4. Eliminar mocks de API en produccion y añadir estados de error/retry.
5. Endurecer resolucion de streams y maquina de estados del reproductor.
