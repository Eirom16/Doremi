# Convenciones FFI

Las conversiones de texto del bridge Rust/C++ se centralizan en `src/cpp/ffi_utils.h`.

## Texto

- Todo texto del bridge se interpreta como UTF-8.
- Usar `Ffi::to_qstring` para `rust::Str`, `rust::String` y `std::string`.
- Usar `Ffi::to_std_string` para `QString`, `rust::Str` y `rust::String`.
- Usar `Ffi::to_rust_string` al devolver un `QString` a Rust.
- No usar conversiones locales o dependientes del locale del sistema en limites FFI.

## Errores

- Una excepcion C++ nunca debe cruzar el ABI de `cxx`.
- Los entrypoints que ejecuten codigo susceptible de lanzar deben usar `Ffi::guard`.
- `Ffi::guard` registra la operacion, excepciones estandar y excepciones desconocidas.
- Los fallos esperables de dominio deben continuar como resultados o estados tipados desde Rust; el guard es la ultima barrera para fallos C++ inesperados.

## Datos sensibles

Las conversiones no registran contenido. Passwords, cookies y tokens solo se convierten para la llamada inmediata y deben limpiarse segun las reglas de almacenamiento seguro.

Las reglas de ownership y dispatch al hilo GUI se definen en [THREADING_AND_OWNERSHIP.md](THREADING_AND_OWNERSHIP.md).
