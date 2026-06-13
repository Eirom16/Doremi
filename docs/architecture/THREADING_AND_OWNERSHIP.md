# Ownership y threading del bridge

Este documento define el contrato de ownership, afinidad de hilos y callbacks permitidos entre Rust y Qt/C++.

## Hilos principales

### Hilo GUI de Qt

- Crea `QApplication`, `DoremiMainWindow` y todos sus widgets.
- Ejecuta `QApplication::exec()` mediante `run_event_loop()`.
- Es el unico hilo autorizado para crear, destruir o mutar `QObject` y widgets.
- Los callbacks C++ conectados a signals de widgets comienzan en este hilo, salvo workers creados explicitamente con `QThread`.

### Runtime Tokio

- Se crea en `src/main.rs` y permanece activo mientras corre Qt.
- Ejecuta HTTP, resolucion de streams, descargas, actualizaciones, MPRIS y otras tareas asincronas.
- Sus workers no poseen objetos Qt y no pueden acceder directamente a `g_main_window` ni a hijos de la ventana.

### Hilos auxiliares

- Discord puede usar un hilo `std::thread` propio.
- El dialogo sudo valida credenciales en un `QThread` y vuelve al GUI mediante `QMetaObject::invokeMethod`.
- Estos hilos siguen la misma regla: ningun acceso directo a widgets.

## Ownership

### Rust

- `DoremiApp` posee configuracion y controla el ciclo de vida de arranque/cierre.
- `Arc<PlayerService>` se registra una vez en `PLAYER`; Rust conserva su ownership.
- `SearchService` se registra una vez en `SEARCH`.
- Los DTOs `Track`, `Album`, `Artist`, `Playlist` y `StatsData` cruzan el bridge por valor. Ningun lado conserva referencias a memoria temporal del otro.
- `rust::Str` es una vista prestada valida solo durante la llamada. C++ debe copiarla si el valor sobrevivira al callback.
- `rust::Vec<T>` y `rust::String` recibidos por valor pertenecen al frame de la llamada. Los lambdas encolados deben capturar copias C++ propias.

### Qt/C++

- Qt posee los `QObject` mediante parent ownership y `deleteLater()`.
- `g_main_window` es un puntero no propietario a la ventana principal; solo es valido entre `create_main_window()` y su destruccion.
- Rust nunca recibe punteros a widgets ni administra memoria Qt.
- Los lambdas encolados no deben capturar referencias a `rust::Str`, `rust::Vec` o variables locales.

## Callbacks Qt hacia Rust

Permitidos desde el hilo GUI:

- Controles del reproductor, seek, volumen, shuffle y repeat.
- Navegacion, busqueda, acciones sobre DTOs y cambios de ajustes.
- Login/logout, backup/restore y solicitud de actualizaciones.
- Lecturas cortas de estado como version, traduccion y autenticacion.

Permitidos desde workers:

- Solo funciones documentadas como thread-safe y que no reentren sincronicamente en widgets.
- Actualmente `on_validate_sudo_password` se invoca desde `QThread`; devuelve un valor y la UI se actualiza despues mediante una llamada encolada.

Restricciones:

- Un callback UI no debe bloquear esperando HTTP, procesos, filesystem pesado ni una tarea Tokio.
- Passwords, cookies y tokens no se registran ni se conservan fuera de la operacion necesaria.
- Un callback que necesite trabajo lento debe copiar sus argumentos y delegarlo a Tokio.

## Callbacks Rust hacia Qt

### Creacion y event loop

Estas operaciones deben ejecutarse en el hilo que crea Qt:

- `create_main_window()` crea `QApplication`, la ventana y sus widgets.
- `run_event_loop()` ejecuta el event loop de Qt.

No deben invocarse desde workers Tokio.

### Permitidos desde cualquier hilo

Los entrypoints de mutacion copian sus argumentos antes de retornar del frame FFI y pasan por `Ffi::on_gui`. El helper ejecuta directamente si ya esta en el hilo GUI y usa `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` en cualquier otro hilo. Esto cubre la ventana principal, vistas, dialogo de actualizacion y setters de estado.

Las lecturas sincronas imprescindibles, como el texto del buscador y la generacion de thumbnails con `QPixmap`, pasan por `Ffi::on_gui_blocking`. No se debe usar este helper para HTTP, filesystem pesado ni procesos.

Regla para nuevos callbacks:

1. Copiar todo `rust::Str`, `rust::String`, DTO o vector antes de retornar de la funcion FFI.
2. Encolar la mutacion sobre el `QObject` propietario o `g_main_window`.
3. Comprobar que el objeto sigue vivo antes de usarlo.
4. No bloquear el worker esperando al hilo GUI salvo que una lectura sincrona sea imprescindible y este documentada.
5. Usar las conversiones y guards de `ffi_utils.h`.

## Reentrancia y errores

- C++ no debe llamar a Rust mientras mantiene locks de Qt o referencias temporales del bridge.
- Rust no debe mantener `MutexGuard` del player, cola o DB mientras llama a C++.
- Las excepciones C++ se detienen en `Ffi::guard`; no pueden cruzar `cxx`.
- Panics Rust no deben usarse como errores de dominio; los callbacks deben registrar o propagar estados tipados.

## Verificacion actual

Al `2026-06-13`, los entrypoints Rust→Qt que mutan widgets pasan por `Ffi::on_gui`. Los accesos directos restantes a `g_main_window` corresponden a creacion/event loop o estan dentro de callbacks ya despachados al hilo GUI.
