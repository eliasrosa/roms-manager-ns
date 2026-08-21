---
inclusion: fileMatch
fileMatchPattern: "src/**"
---

# Borealis — Referência Rápida

Fork `eliasrosa/borealis`, branch **`wiliwili`** (base: xfangfang). O submodule
tem patches locais — ver `borealis-fork.md` antes de suspeitar do upstream.

## API Principal

### Application lifecycle
```cpp
brls::Application::init()                    // inicializa
brls::Application::createWindow("titulo")    // cria janela
brls::Application::setGlobalQuit(true)       // + para sair
brls::Application::registerXMLView("N", C::create)  // registra view
brls::Application::pushActivity(new Act())   // push tela
brls::Application::mainLoop()                // loop (retorna false para sair)
brls::Application::giveFocus(view)           // foco manual
```

### Threading
```cpp
#include <borealis/core/thread.hpp>

brls::async([]() { /* fora da UI thread */ });   // enfileira no task loop
brls::sync([]()  { /* na UI thread */ });        // volta pra UI thread
brls::delay(500, []() { /* após 500ms */ });     // one-shot
```

- ⚠️ **NUNCA usar `std::thread` no Switch** — no devkitA64 ela lança
  `std::system_error(ENOSYS)` e o app morre por `abort()`. O próprio Borealis
  evita `std::thread`: usa `pthread_create` direto, a menos que
  `BOREALIS_USE_STD_THREAD` esteja definido.
- `brls::async` **não** cria uma thread por chamada — o Borealis mantém uma
  única task loop thread e enfileira nela. Tasks longas serializam.
- Borealis não é thread-safe: qualquer mudança de View precisa passar por
  `brls::sync`.
- Ao capturar `this` num callback assíncrono de uma View que pode ser
  destruída, usar um guard `std::shared_ptr<std::atomic<bool>>` e checá-lo no
  início do `sync` (ver `SyncTab::alive`).

### Logger
```cpp
brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
brls::Logger::info("valor: {}", x);          // usa fmt, não printf
brls::Logger::setLogOutput(FILE*);           // redireciona a saída
brls::Logger::getLogEvent()->subscribe(cb);  // espelha sem perder o stdout
```

- O formato é **fmt** (`{}`), não printf (`%s`). Formato inválido não crasha: cai
  num catch interno que imprime `! Invalid log format string`.
- `getLogEvent()` é o jeito de adicionar um sink extra (é assim que
  `src/debug_log.cpp` grava em arquivo mantendo o log no nxlink). O callback roda
  **dentro** do mutex do Logger, então não precisa de sincronização própria — mas
  não deve lançar exceção nem chamar o `Logger` de volta.
- `setThreadSafeLogging(bool)` controla esse mutex.

### Activity
```cpp
class MyActivity : public brls::Activity {
    CONTENT_FROM_XML_RES("activity/main.xml");  // carrega XML de resources/xml/
};
```

### Views disponíveis
- `brls::Box` — container flexbox (base para custom views)
- `brls::Label` — texto (setText, setFontSize, setTextColor)
- `brls::Button` — botão focável (setText, registerAction)
- `brls::Header` — cabeçalho de seção (setTitle, setSubtitle)
- `brls::Image` — imagem
- `brls::ScrollingFrame` — scroll vertical (setContentView)
- `brls::TabFrame` — tabs com sidebar (XML only)
- `brls::AppletFrame` — frame com título e ícone

### Custom View (padrão)
```cpp
class MyView : public brls::Box {
  public:
    MyView() {
        this->setAxis(brls::Axis::COLUMN);
        // ... addView()
    }
    static brls::View* create() { return new MyView(); }
};
```

### Métodos úteis de View
```cpp
view->setAxis(brls::Axis::COLUMN)        // layout vertical
view->setMargins(top, right, bottom, left)
view->setPadding(top, right, bottom, left)
view->setGrow(1.0f)                      // flex-grow
view->setHeight(300)                     // altura fixa
view->setBackgroundColor(nvgRGBA(r,g,b,a))
view->setCornerRadius(8.0f)
view->addView(child)
view->removeView(child)                  // faz delete!
view->getChildren()                      // vector<View*>&
```

### Cuidados
- `removeView()` faz `delete` no ponteiro — nunca usar view depois
- Não destruir views dentro de action callbacks (segfault)
- Para reload de conteúdo: pushActivity nova ou pending flag + RepeatingTask
- Header() construtor sem args — usar setTitle() depois
- Label não é focável — não dar giveFocus em Label
- `createFromXMLResource("file.xml")` → busca em `BRLS_RESOURCES/xml/file.xml`
- **Button actions disparam em repeat** se o botão for segurado. Usar flag `isBusy` para impedir re-entrada em operações demoradas (rede, I/O).
- **Copiar XML de outros projetos**: testar com atributos mínimos primeiro e ir adicionando um a um. Atributos como `iconInterpolation` só devem ser usados se houver ícone definido — caso contrário podem causar textos invisíveis.
- **AppletFrame**: aceita exatamente 1 child XML. O `title` do content view (ex: `TabFrame`) é exibido no header automaticamente via `getAppletFrameItem()`.

### XML
```xml
<brls:TabFrame title="App" icon="@res/img/icon.png">
    <brls:Tab label="Nome">
        <MinhaView />
    </brls:Tab>
    <brls:Separator />
</brls:TabFrame>
```

### Cores (NVG)
```cpp
nvgRGBA(255, 255, 255, 255)  // branco
nvgRGBA(0, 255, 128, 255)    // verde terminal
nvgRGBA(15, 15, 20, 250)     // fundo escuro
```
