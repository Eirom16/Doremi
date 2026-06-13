# Casos manuales de referencia

Version: `1.0.0`

Estos casos fijan el comportamiento esperado de Pyrolist. Deben ejecutarse primero en Pyrolist y luego en Doremi con la misma cuenta, region y contenido. No deben usarse datos privados en capturas, fixtures o logs.

## Formato de evidencia

Para cada ejecucion registrar fecha, commit, entorno de escritorio, backend grafico, resultado, tiempo aproximado y enlaces a capturas/logs redactados. Un resultado manual no convierte el flujo en `verificado` si no es reproducible.

## MC-LOGIN-01 Login y logout

Precondicion: perfil temporal sin credenciales y keyring disponible.

1. Abrir la aplicacion y seleccionar login.
2. Verificar que solo se navega por dominios esperados de Google/YouTube.
3. Completar login y comprobar nombre/avatar y contenido autenticado.
4. Reiniciar la aplicacion y comprobar restauracion de sesion.
5. Cerrar sesion y reiniciar.

Resultado esperado: la sesion se restaura desde keyring; logout elimina credenciales, cookies/perfil temporal y vuelve limpiamente a modo anonimo. Ningun secreto aparece en TOML, DB, backup o logs.

## MC-SEARCH-01 Busqueda, filtros y detalle

Precondicion: conectividad disponible.

1. Buscar una consulta con canciones, artistas, albums y playlists.
2. Cambiar cada filtro y verificar que el contenido corresponde al tipo.
3. Abrir un resultado de cada tipo y volver atras.
4. Repetir una busqueda reciente y probar una consulta sin resultados.
5. Desconectar la red y reintentar.

Resultado esperado: IDs y tipos controlan las acciones, no el texto visible; volver atras conserva consulta y scroll; empty, error y offline son distinguibles.

## MC-PLAY-01 Reproduccion y recuperacion

Precondicion: VLC disponible y una pista reproducible.

1. Reproducir desde search y esperar estado playing.
2. Pausar, reanudar, hacer seek y cambiar volumen.
3. Pulsar rapidamente otra pista durante resolucion.
4. Forzar una URL expirada/403 o desconectar temporalmente la red.
5. Cerrar y reabrir con resume habilitado y deshabilitado.

Resultado esperado: la ultima accion gana; callbacks obsoletos no reemplazan la pista actual; errores transitorios se explican y no saltan silenciosamente.

## MC-QUEUE-01 Cola, shuffle y repeat

Precondicion: cola de al menos cinco pistas identificables por ID.

1. Usar reproducir despues y agregar al final.
2. Mover, borrar, saltar y limpiar elementos.
3. Activar/desactivar shuffle varias veces.
4. Probar repeat off/all/one en primer elemento, ultimo y cola vacia.
5. Reiniciar la aplicacion con una cola activa.

Resultado esperado: no se pierden ni duplican pistas; desactivar shuffle recupera el orden original; repeat respeta bordes; cola e indice se restauran.

## MC-LYRICS-01 Letras y sincronizacion

Precondicion: una pista con letra sincronizada y otra sin letra.

1. Reproducir la pista con letra y abrir Now Playing.
2. Hacer seek hacia delante y atras.
3. Cambiar delay, alineacion, tamano, auto-scroll y glow.
4. Pasar a la siguiente pista y volver a la anterior sin red.
5. Probar una pista sin resultados.

Resultado esperado: la linea activa sigue la posicion y los ajustes se aplican; cache/prefetch evita una nueva espera; ausencia de letra muestra un estado explicito.

## MC-DOWNLOAD-01 Descarga individual

Precondicion: `yt-dlp` disponible y carpeta de destino escribible.

1. Iniciar una descarga desde search o un detalle.
2. Observar estado y progreso.
3. Cancelar una segunda descarga y reintentarla.
4. Reproducir el archivo local y borrar la descarga.
5. Eliminar manualmente un archivo y refrescar la vista.

Resultado esperado: estados queued/downloading/completed/failed/cancelled son visibles; no quedan registros falsos; errores permiten reintento.

## MC-DOWNLOAD-02 Album y playlist

Precondicion: album y playlist con pistas disponibles y no disponibles.

1. Iniciar descarga completa.
2. Cerrar y reabrir durante el proceso.
3. Cancelar elementos individuales y el conjunto.
4. Reintentar fallos.

Resultado esperado: expansion y progreso total son correctos; se preserva metadata del conjunto; pistas no disponibles se reportan sin abortar todo.

## MC-LIBRARY-01 Biblioteca y playlists

Precondicion: cuenta con likes y playlists remotas.

1. Dar y quitar like desde search, detalle y Now Playing.
2. Verificar estado en biblioteca local y remota.
3. Crear, renombrar y borrar una playlist.
4. Agregar y eliminar pistas.
5. Simular un fallo remoto y reabrir.

Resultado esperado: estado local/remoto converge por ID; conflictos y fallos no producen favoritos o playlists fantasma.

## MC-CLOSE-01 Cierre, tray y limpieza

Precondicion: reproduccion, descarga y tareas de red activas.

1. Cerrar con `stop_on_close` desactivado y tray habilitado.
2. Restaurar desde tray y usar play/pause, next y previous.
3. Salir explicitamente desde tray.
4. Repetir con `stop_on_close` activado.

Resultado esperado: minimizar y salir son acciones distintas; al salir se cancelan workers, se guarda estado y se cierran VLC, DB, MPRIS, Discord y tareas Tokio sin panic ni proceso residual.
