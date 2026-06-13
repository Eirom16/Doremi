# Contrato Rust/C++

Version actual: `1.0`

El bridge `cxx` declara versiones independientes en Rust y C++. Doremi compara ambas antes de crear servicios, widgets o iniciar el event loop de Qt.

## Compatibilidad

- El `major` debe coincidir exactamente.
- El `minor` ofrecido por C++ debe ser igual o superior al requerido por Rust.
- Una incompatibilidad detiene el arranque y se registra como error; no se intenta ejecutar con un ABI o una semantica desconocida.

## Cuando incrementar

- Incrementar `minor` al añadir funciones, campos o capacidades que mantengan compatibles los consumidores existentes.
- Incrementar `major` al eliminar o renombrar funciones/campos, cambiar tipos o modificar la semantica de una operacion existente.
- Actualizar primero el lado proveedor, despues el consumidor y finalmente el minimo requerido.

## Ubicaciones

- Rust: `CONTRACT_MAJOR`, `CONTRACT_MINOR` y `verify_contract()` en `src/bridge.rs`.
- C++: `kBridgeContractMajor` y `kBridgeContractMinor` en `src/cpp/main_window.cpp`.

Todo cambio de contrato debe actualizar estas constantes, las pruebas de compatibilidad y este documento en el mismo cambio.

Ownership, callbacks y afinidad de hilos se especifican en [THREADING_AND_OWNERSHIP.md](THREADING_AND_OWNERSHIP.md).
