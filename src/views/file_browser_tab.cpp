/**
 * FileBrowserTab - Implementação do navegador de arquivos
 *
 * Exibe o conteúdo do diretório uma única vez (sem reload dinâmico).
 * Para navegar entre diretórios, cada botão de pasta faz push de uma
 * nova Activity com o FileBrowser apontando para o subdiretório.
 * Botão B (Escape no PC) faz pop e volta ao diretório anterior.
 */

#include "file_browser_tab.hpp"

// Activity wrapper para navegação de subdiretórios
class FileBrowserActivity : public brls::Activity
{
  public:
    FileBrowserActivity(const std::string& path)
        : path(path) {}

    brls::View* createContentView() override
    {
        brls::AppletFrame* frame = new brls::AppletFrame();
        frame->setTitle("ROMs Manager NS");

        FileBrowserTab* tab = new FileBrowserTab();
        tab->setPath(path);
        frame->setContentView(tab);

        return frame;
    }

    void setPath(const std::string& p) { this->path = p; }

  private:
    std::string path;
};

// Método auxiliar para setar path sem reconstruir no construtor
void fileBrowserSetPath(FileBrowserTab* tab, const std::string& path);

FileBrowserTab::FileBrowserTab()
{
    this->setAxis(brls::Axis::COLUMN);
    this->current_path = platform::sdRoot();
    this->buildUI();
}

void FileBrowserTab::setPath(const std::string& path)
{
    this->current_path = path;

    // Limpar e rebuild
    auto children = this->getChildren();
    while (!children.empty())
    {
        this->removeView(children[children.size() - 1]);
        children = this->getChildren();
    }

    this->buildUI();
}

void FileBrowserTab::buildUI()
{
    // Label com caminho atual
    brls::Label* pathLabel = new brls::Label();
    pathLabel->setText("Caminho: " + this->current_path);
    pathLabel->setFontSize(18);
    pathLabel->setMargins(16, 16, 8, 16);
    this->addView(pathLabel);

    // Ler diretório
    std::vector<FileEntry> entries = this->readDirectory(this->current_path);

    // Separar e ordenar
    std::vector<FileEntry> dirs;
    std::vector<FileEntry> files;

    for (auto& entry : entries)
    {
        if (entry.is_directory)
            dirs.push_back(entry);
        else
            files.push_back(entry);
    }

    auto sortByName = [](const FileEntry& a, const FileEntry& b) {
        return a.name < b.name;
    };
    std::sort(dirs.begin(), dirs.end(), sortByName);
    std::sort(files.begin(), files.end(), sortByName);

    // Diretórios
    if (!dirs.empty())
    {
        brls::Header* dirHeader = new brls::Header();
        dirHeader->setTitle("Diretorios (" + std::to_string(dirs.size()) + ")");
        this->addView(dirHeader);

        for (auto& dir : dirs)
        {
            brls::Button* btn = new brls::Button();
            btn->setText("[DIR] " + dir.name);
            btn->setMargins(2, 16, 2, 16);

            std::string dir_path = dir.full_path;
            btn->registerAction("Abrir", brls::BUTTON_A, [dir_path](brls::View* view) {
                // Push nova activity para o subdiretório
                brls::Application::pushActivity(new FileBrowserActivity(dir_path));
                return true;
            });

            this->addView(btn);
        }
    }

    // Arquivos
    if (!files.empty())
    {
        brls::Header* fileHeader = new brls::Header();
        fileHeader->setTitle("Arquivos (" + std::to_string(files.size()) + ")");
        this->addView(fileHeader);

        for (auto& file : files)
        {
            std::string prefix = this->getFilePrefix(file);
            std::string sizeStr = this->formatSize(file.size);
            std::string text = prefix + file.name;
            if (!sizeStr.empty())
                text += " (" + sizeStr + ")";

            brls::Label* label = new brls::Label();
            label->setText(text);
            label->setFontSize(20);
            label->setMargins(6, 24, 6, 24);
            this->addView(label);
        }
    }

    // Vazio
    if (dirs.empty() && files.empty())
    {
        brls::Label* emptyLabel = new brls::Label();
        emptyLabel->setText("(Diretorio vazio)");
        emptyLabel->setFontSize(20);
        emptyLabel->setMargins(16, 16, 16, 16);
        this->addView(emptyLabel);
    }
}

std::vector<FileEntry> FileBrowserTab::readDirectory(const std::string& path)
{
    std::vector<FileEntry> result;

    if (path.empty())
    {
        brls::Logger::warning("readDirectory chamado com path vazio");
        return result;
    }

    DIR* dir = opendir(path.c_str());

    if (!dir)
    {
        brls::Logger::warning("Nao foi possivel abrir: {}", path);
        return result;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        std::string name = ent->d_name;

        if (name == "." || name == "..")
            continue;

        if (name[0] == '.')
            continue;

        FileEntry entry;
        entry.name = name;
        entry.is_directory = (ent->d_type == DT_DIR);
        entry.full_path = path + name + (entry.is_directory ? "/" : "");
        entry.size = 0;

        if (!entry.is_directory)
        {
            struct stat st;
            if (stat(entry.full_path.c_str(), &st) == 0)
                entry.size = st.st_size;
        }

        result.push_back(entry);
    }

    closedir(dir);
    return result;
}

std::string FileBrowserTab::formatSize(size_t bytes)
{
    if (bytes == 0)
        return "";

    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit < 3)
    {
        size /= 1024.0;
        unit++;
    }

    char buf[32];
    if (unit == 0)
        snprintf(buf, sizeof(buf), "%d B", (int)size);
    else
        snprintf(buf, sizeof(buf), "%.1f %s", size, units[unit]);

    return std::string(buf);
}

std::string FileBrowserTab::getExtension(const std::string& filename)
{
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(pos);
    return "";
}

std::string FileBrowserTab::getFilePrefix(const FileEntry& entry)
{
    std::string ext = this->getExtension(entry.name);

    if (ext == ".nsp" || ext == ".xci")
        return "[ROM] ";
    else if (ext == ".nro")
        return "[HBW] ";
    else
        return "";
}

brls::View* FileBrowserTab::create()
{
    return new FileBrowserTab();
}
