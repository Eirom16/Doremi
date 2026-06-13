# Politica de paridad y eliminacion deliberada

Version: `1.0.0`

## Regla por defecto

Toda funcion util y accesible al usuario en Pyrolist permanece dentro del alcance de Doremi. Una funcion no puede declararse eliminada solo porque sea costosa, fragil o tenga una implementacion incompleta.

## Requisitos para eliminar una funcion

Una eliminacion deliberada necesita:

1. Identificar el flujo de la matriz y su uso real.
2. Explicar el problema de seguridad, mantenimiento, plataforma o producto.
3. Documentar impacto, alternativa y ruta de migracion de datos.
4. Obtener una decision explicita del mantenedor.
5. Marcar la fila como `eliminada deliberadamente` con fecha y enlace a la decision.
6. Retirar UI, ajustes, datos muertos y documentacion asociada en el mismo cambio.

No se acepta dejar un toggle que no haga nada. Si el motor aun no existe, el control debe ocultarse, deshabilitarse con explicacion o permanecer clasificado como `UI solamente`.

## Decisiones actuales

No hay funciones de Pyrolist aprobadas para eliminacion al `2026-06-13`.

Los toggles de normalizacion, skip silence, gapless y crossfade siguen dentro del alcance. Deben implementarse de verdad o someterse a esta politica antes de retirarlos.
