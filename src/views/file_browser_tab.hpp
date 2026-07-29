#pragma once

/**
 * FileBrowserTab - Tab customizada para navegação de arquivos
 *
 * Mostra o conteúdo de um diretório com botões para navegar.
 * Navegação é feita via push de novas Activities para evitar
 * destruir views durante callbacks (causa segfault).
 */

#include <borealis.hpp>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include "../platform.hpp"

struct FileEntry
{
    std::string name;
    std::string full_path;
    bool is_directory;
    size_t size;
};

class FileBrowserTab : public brls::Box
{
  public:
    FileBrowserTab();

    void setPath(const std::string& path);

    static brls::View* create();

  private:
    std::string current_path;

    void buildUI();
    std::vector<FileEntry> readDirectory(const std::string& path);
    std::string formatSize(size_t bytes);
    std::string getExtension(const std::string& filename);
    std::string getFilePrefix(const FileEntry& entry);
};
