# Referencias visuales de Pyrolist

Version: `1.0.0`

Capturadas el `2026-06-13` desde Pyrolist `ce9958f` usando Qt `offscreen`, un perfil XDG temporal, sin credenciales y sin datos privados.

| Referencia | Archivo | Comportamiento fijado |
|---|---|---|
| Home cargando | [01-home-loading.png](reference/pyrolist/01-home-loading.png) | Titulo visible y skeleton de siete filas durante carga. |
| Search vacio | [02-search-empty.png](reference/pyrolist/02-search-empty.png) | Chips de canciones, albums y playlists; empty state por categoria. |
| Settings/Apariencia | [03-settings-appearance.png](reference/pyrolist/03-settings-appearance.png) | Navegacion lateral, selector de acento, toggles, tema e idioma. |

## Limitaciones de esta captura

Las vistas se renderizaron aisladas para evitar usar cuenta, red, audio o keyring reales. Por eso no representan el shell completo, contenido remoto, artwork, mini player ni estados autenticados. Los comportamientos interactivos se especifican en [MANUAL_CASES.md](MANUAL_CASES.md) y deben capturarse de nuevo durante las ejecuciones manuales en un entorno GUI completo.

## Reproduccion

La captura usa `PYTHONPATH` dirigido al `src` auditado, directorios XDG temporales, `QT_QPA_PLATFORM=offscreen` y widgets de Pyrolist a `1300x820`. No se reutilizan configuracion ni secretos del usuario.
