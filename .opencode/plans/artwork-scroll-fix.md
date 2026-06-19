# Plan: Artwork completo + Scroll general del body

## Problemas identificados

1. **Carátulas cortadas**: El `HorizontalCarousel` no reporta su `sizeHint()`, por lo que Qt Layout no le asigna la altura suficiente. Como el `QScrollArea` interno tiene `setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff)`, el contenido se recorta verticalmente.

2. **Scroll por vista, no general**: Cada vista dentro del `FadeStack` tiene su propio `QScrollArea`. El usuario quiere un **solo scroll** a nivel del contenedor `body` (envolviendo el `FadeStack`), no scrolls individuales por vista.

---

## Cambios necesarios

### 1. HorizontalCarousel — sizeHint() (1 archivo .h + 1 .cpp)

**`src/cpp/components/horizontal_carousel.h`**
- Agregar `int m_minContentHeight = 0;` como campo privado
- Agregar `QSize sizeHint() const override;` en protected

**`src/cpp/components/horizontal_carousel.cpp`**
- En `init()`: inicializar `m_minContentHeight = 0`
- En `addWidget()`: después de insertar widget, calcular:
  ```cpp
  int h = widget->sizeHint().height() + 24; // 12px top + 12px bottom margins
  if (h > m_minContentHeight) {
      m_minContentHeight = h;
      updateGeometry();
  }
  ```
- Implementar `sizeHint()`:
  ```cpp
  QSize HorizontalCarousel::sizeHint() const {
      return QSize(width(), m_minContentHeight);
  }
  ```

### 2. main_window — Scroll general del body (1 archivo .cpp + 1 .h)

**`src/cpp/main_window.h`**
- Agregar `QScrollArea *body_scroll_ = nullptr;` como campo privado

**`src/cpp/main_window.cpp`**
- En el constructor, después de crear el `body` layout, reemplazar:
  ```cpp
  body->addWidget(stack_, 1);
  ```
  con:
  ```cpp
  body_scroll_ = new QScrollArea(central);
  body_scroll_->setWidgetResizable(true);
  body_scroll_->setFrameShape(QFrame::NoFrame);
  body_scroll_->setStyleSheet("background: transparent; border: none;");
  body_scroll_->setWidget(stack_);
  body->addWidget(body_scroll_, 1);
  ```

- En `navigate_to()`: **eliminar** todos los reseteos de scroll como:
  ```cpp
  auto *sa = search_view_->findChild<QScrollArea *>();
  if (sa) sa->verticalScrollBar()->setValue(0);
  ```
  Ya no son necesarios porque el scroll se resetea automáticamente al cambiar el contenido del `body_scroll_`.

### 3. Cada View — Remover QScrollArea interno

Para cada uno de los siguientes archivos, hay que:
- Eliminar el campo `QScrollArea *scroll_` (o `scroll_area_`)
- Eliminar la creación del `QScrollArea` en el constructor
- Hacer que el contenido interior sea el widget principal del view directamente
- Ajustar el layout raíz para que use el contenido directamente
- Actualizar el `.h` correspondiente

#### 3a. HomeView (`src/cpp/home_view.cpp` + `.h`)
- Eliminar `QScrollArea *scroll_` del `.h`
- En `.cpp` constructor: en lugar de crear `scroll_` y poner `inner` dentro, usar `inner` directamente como el widget del view.
  - El `QVBoxLayout *root` se conecta directo a `inner`
  - Mantener la señal `load_more_requested` conectada al `verticalScrollBar()` del padre (? - esto hay que pensarlo, el scroll ya no está en HomeView)
  - Mejor opción: mover `load_more_requested` al body_scroll_ (opcional, puede quitarse temporalmente)
  
#### 3b. SearchView (`src/cpp/search_view.cpp` + `.h`)
#### 3c. LibraryView (`src/cpp/library_view.cpp` + `.h`)
#### 3d. SettingsView (`src/cpp/settings_view.cpp` + `.h`)
#### 3e. TrendingView (`src/cpp/trending_view.cpp` + `.h`)
#### 3f. DownloadsView (`src/cpp/downloads_view.cpp` + `.h`)
#### 3g. StatsView (`src/cpp/stats_view.cpp` + `.h`)
#### 3h. HistoryView (`src/cpp/history_view.cpp` + `.h`)
#### 3i. AlbumDetailView (`src/cpp/album_detail_view.cpp` + `.h`)
#### 3j. ArtistDetailView (`src/cpp/artist_detail_view.cpp` + `.h`)
#### 3k. PlaylistDetailView (`src/cpp/playlist_detail_view.cpp` + `.h`)

**Patrón de refactor para cada view:**

Antes (ej. HomeView):
```cpp
// .h
class HomeView : public QWidget {
    // ...
private:
    QScrollArea *scroll_;
    QVBoxLayout *content_;
};

// .cpp constructor
scroll_ = new QScrollArea(this);
scroll_->setWidgetResizable(true);
scroll_->setFrameShape(QFrame::NoFrame);
scroll_->setStyleSheet("background: transparent; border: none;");
auto *inner = new QWidget();
inner->setStyleSheet("background: transparent;");
content_ = new QVBoxLayout(inner);
// ... add widgets to content_ ...
scroll_->setWidget(inner);
auto *root = new QVBoxLayout(this);
root->addWidget(scroll_);
```

Después:
```cpp
// .h
class HomeView : public QWidget {
    // ...
private:
    QVBoxLayout *content_;  // contenido directo
};

// .cpp constructor
setStyleSheet("background: transparent;");
content_ = new QVBoxLayout(this);
// ... same widgets directly in content_ ...
```

### 4. Consideraciones adicionales

- **WelcomeView** no tiene QScrollArea, se queda igual.
- **NowPlayingView** es un overlay fuera del FadeStack, no se toca.
- La función `load_more_requested` de HomeView estaba conectada al scroll. Sin scroll interno, se puede reconectar al `body_scroll_` o simplemente eliminar temporalmente (el infinite scroll es una feature menor).
- Los `#include <QScrollArea>` innecesarios deben eliminarse de cada view.

---

## Archivos a modificar (total: ~25 archivos)

| Archivo | Cambio |
|---------|--------|
| `src/cpp/components/horizontal_carousel.h` | + `sizeHint()`, + `m_minContentHeight` |
| `src/cpp/components/horizontal_carousel.cpp` | Implementar `sizeHint()`, actualizar en `addWidget()` |
| `src/cpp/main_window.h` | + `body_scroll_` |
| `src/cpp/main_window.cpp` | Agregar QScrollArea envolviendo stack, eliminar scroll resets |
| `src/cpp/home_view.h` | Eliminar `scroll_` |
| `src/cpp/home_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/search_view.h` | Eliminar scroll |
| `src/cpp/search_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/library_view.h` | Eliminar scroll |
| `src/cpp/library_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/settings_view.h` | Eliminar scroll |
| `src/cpp/settings_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/trending_view.h` | Eliminar scroll |
| `src/cpp/trending_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/downloads_view.h` | Eliminar scroll |
| `src/cpp/downloads_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/stats_view.h` | Eliminar `scroll_area_` |
| `src/cpp/stats_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/history_view.h` | Eliminar `scroll_area_` |
| `src/cpp/history_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/album_detail_view.h` | Eliminar `scroll_area_` |
| `src/cpp/album_detail_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/artist_detail_view.h` | Eliminar `scroll_area_` |
| `src/cpp/artist_detail_view.cpp` | Refactor sin QScrollArea |
| `src/cpp/playlist_detail_view.h` | Eliminar `scroll_area_` |
| `src/cpp/playlist_detail_view.cpp` | Refactor sin QScrollArea |

---

## Verificación

```bash
cargo check --lib 2>&1 | grep "^error"
cargo test -- --test-threads=1 2>&1 | grep "test result:"
```
