# Limitaciones del Motor de Audio VLC y Gapless Playback

Este documento describe las limitaciones de libVLC como motor de audio para Doremi y el diseño adoptado para mitigar estas limitaciones.

---

## 1. Limitaciones Inherentes de libVLC

libVLC es un motor de reproducción multimedia potente y versátil, pero está estructurado en base a pipelines de reproducción discretos para cada archivo o URL. Esto conlleva ciertas limitaciones al usarse en reproductores de audio de alta fidelidad:

### A. Gapless Playback (Reproducción sin Pausas)
* **El Problema**: VLC no soporta de forma nativa la transición de latencia cero (gapless) entre dos elementos multimedia separados. Al iniciar un nuevo medio (`MediaPlayer::set_media`), VLC debe destruir el hilo del decodificador actual, solicitar y resolver la nueva URL o archivo, inicializar el demuxer y los códecs de audio correspondientes, y rellenar el buffer de audio. Este proceso suele tomar entre **100ms y 500ms** dependiendo de la red y el formato, produciendo un breve silencio perceptible (gap) entre canciones.
* **Mitigación**: Doremi implementa la precarga/prefetching en segundo plano (resolviendo la URL y cacheando el artwork y las letras de la siguiente pista). Sin embargo, incluso con la URL ya resuelta, la inicialización del pipeline en VLC produce un gap menor pero aún presente.

### B. Ausencia de Mezclador de Audio de Múltiples Pistas
* **El Problema**: Una única instancia de `MediaPlayer` sólo puede decodificar y reproducir un flujo de audio a la vez. No cuenta con métodos para cargar dos pistas simultáneamente y mezclarlas con ganancia variable de manera nativa.

---

## 2. Solución de Doremi: Crossfade Real y Solapamiento Asíncrono

Para solventar las limitaciones anteriores y brindar una transición totalmente fluida y placentera entre canciones, Doremi implementa un **crossfade real a nivel de reproductor**:

### A. Arquitectura de Dos Reproductores
En lugar de depender de un solo pipeline, el motor `AudioEngine` gestiona dos instancias independientes de `MediaPlayer` (`player` y `secondary_player`) creadas a partir de la misma instancia global de libVLC.

### B. Flujo de Transición con Crossfade
1. **Precarga Activa**: A falta de pocos segundos de terminar la canción actual, Doremi consulta la cola de reproducción y obtiene el siguiente enlace.
2. **Inicialización Asíncrona**: En lugar de esperar a que termine la pista actual, se inicia la reproducción de la nueva pista en el `secondary_player`. Ambos reproductores suenan en paralelo, mezclándose a nivel del servidor de audio del sistema (ej. PulseAudio o PipeWire).
3. **Desvanecimiento Cruzado (Fading)**: De manera progresiva y lineal (durante los segundos configurados en los Ajustes), Doremi decrementa el volumen del `player` principal de `100% -> 0%` y aumenta el del `secondary_player` de `0% -> 100%`.
4. **Intercambio (Promotion)**: Una vez concluida la transición o habiendo terminado la pista previa, se detiene y libera el `player` principal anterior, se promueve el `secondary_player` a `player` principal, y se restaura el volumen de reproducción configurado.

Este enfoque no sólo proporciona un efecto de crossfade profesional, sino que **elimina por completo el gap de libVLC**, ofreciendo una experiencia auditiva fluida y continua sin interrupciones.
