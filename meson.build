project('roms-manager-ns', ['c', 'cpp'],
    version : '0.2.0',
    default_options : ['cpp_std=c++17', 'warning_level=2']
)

# Importar borealis
subdir('library/library')

# Sources do app
app_sources = files(
    'src/main.cpp',
    'src/views/file_browser_tab.cpp',
    'src/views/sync_tab.cpp',
    'src/sync/config.cpp',
    'src/sync/http_client.cpp',
    'src/sync/sync_manager.cpp',
)

# Include do app
app_include = include_directories('src')

# Executável
executable('roms-manager-ns',
    app_sources,
    borealis_files,
    include_directories : [app_include, borealis_include],
    dependencies : borealis_dependencies,
    cpp_args : borealis_cpp_args + ['-DBRLS_RESOURCES="./resources/"', '-include', 'cstdint'],
    install : false,
)
