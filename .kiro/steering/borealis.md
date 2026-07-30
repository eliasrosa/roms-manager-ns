---
inclusion: fileMatch
fileMatchPattern: "src/**"
---

# Borealis — Referência Rápida (branch main)

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
