#!/usr/bin/env python3
"""
ROMs Manager NS - Servidor de Sync

Serve arquivos (ROMs, covers, saves) via HTTP e gera/atualiza
automaticamente o manifest.json com lista de arquivos + hashes.

Uso:
    python3 serve.py [--port 8080] [--dir ./data]

O diretório deve ter a estrutura:
    data/
    ├── roms/        (.nsp, .xci, .nro)
    ├── covers/      (.jpg, .png)
    └── saves/       (qualquer)

Endpoints:
    GET /manifest.json   → lista de todos os arquivos com hash/tamanho
    GET /roms/file.nsp   → download direto
    GET /covers/file.jpg → download direto
    GET /saves/file      → download direto
    GET /health          → status do servidor
"""

import argparse
import hashlib
import json
import os
import time
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from threading import Thread


def compute_md5(filepath: str, chunk_size: int = 8192) -> str:
    """Calcula MD5 de um arquivo."""
    h = hashlib.md5()
    with open(filepath, "rb") as f:
        while chunk := f.read(chunk_size):
            h.update(chunk)
    return h.hexdigest()


def generate_manifest(base_dir: str) -> dict:
    """Gera manifest com todos os arquivos do diretório."""
    manifest = {
        "version": 1,
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "files": []
    }

    base = Path(base_dir)
    exclude = {"._", ".DS_Store", "Thumbs.db", "manifest.json"}

    for root, dirs, files in os.walk(base):
        # Ignorar diretórios ocultos
        dirs[:] = [d for d in dirs if not d.startswith(".")]

        for filename in sorted(files):
            # Ignorar arquivos excluídos
            if any(filename.startswith(e) or filename == e for e in exclude):
                continue

            filepath = Path(root) / filename
            rel_path = filepath.relative_to(base)

            stat = filepath.stat()

            entry = {
                "path": "/" + str(rel_path),
                "size": stat.st_size,
                "md5": compute_md5(str(filepath)),
                "modified": time.strftime(
                    "%Y-%m-%dT%H:%M:%S%z", time.localtime(stat.st_mtime)
                ),
            }

            manifest["files"].append(entry)

    manifest["total_files"] = len(manifest["files"])
    manifest["total_size"] = sum(f["size"] for f in manifest["files"])

    return manifest


def save_manifest(base_dir: str):
    """Gera e salva manifest.json no diretório."""
    manifest = generate_manifest(base_dir)
    manifest_path = os.path.join(base_dir, "manifest.json")

    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"[manifest] Gerado: {manifest['total_files']} arquivos, "
          f"{manifest['total_size'] / 1024 / 1024:.1f} MB")
    return manifest


class SyncHandler(SimpleHTTPRequestHandler):
    """Handler HTTP customizado com CORS e logging."""

    def end_headers(self):
        # CORS para debug
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def do_GET(self):
        if self.path == "/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            response = json.dumps({"status": "ok", "time": time.time()})
            self.wfile.write(response.encode())
            return

        if self.path == "/manifest.json":
            # Regenerar manifest a cada request (garantir atualizado)
            save_manifest(self.directory)

        super().do_GET()

    def log_message(self, format, *args):
        print(f"[{time.strftime('%H:%M:%S')}] {args[0]}")


def watch_and_regenerate(base_dir: str, interval: int = 60):
    """Thread que regenera manifest periodicamente."""
    while True:
        time.sleep(interval)
        try:
            save_manifest(base_dir)
        except Exception as e:
            print(f"[erro] Falha ao regenerar manifest: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="ROMs Manager NS - Servidor de Sync"
    )
    parser.add_argument(
        "--port", "-p", type=int, default=8080, help="Porta HTTP (default: 8080)"
    )
    parser.add_argument(
        "--dir", "-d", type=str, default="./data",
        help="Diretório base com roms/covers/saves (default: ./data)"
    )
    parser.add_argument(
        "--regenerate-interval", type=int, default=60,
        help="Intervalo em segundos para regenerar manifest (default: 60)"
    )

    args = parser.parse_args()

    # Validar diretório
    base_dir = os.path.abspath(args.dir)
    if not os.path.isdir(base_dir):
        print(f"[!] Diretório não encontrado: {base_dir}")
        print(f"    Criando estrutura...")
        os.makedirs(os.path.join(base_dir, "roms"), exist_ok=True)
        os.makedirs(os.path.join(base_dir, "covers"), exist_ok=True)
        os.makedirs(os.path.join(base_dir, "saves"), exist_ok=True)
        print(f"    Coloque seus arquivos em: {base_dir}")

    # Gerar manifest inicial
    print(f"")
    print(f"╔══════════════════════════════════════════════╗")
    print(f"║   ROMs Manager NS - Servidor de Sync        ║")
    print(f"╠══════════════════════════════════════════════╣")
    print(f"║  Diretório: {base_dir:<32}║")
    print(f"║  Porta:     {args.port:<32}║")
    print(f"╚══════════════════════════════════════════════╝")
    print(f"")

    manifest = save_manifest(base_dir)
    print(f"")
    print(f"Endpoints:")
    print(f"  GET http://0.0.0.0:{args.port}/manifest.json")
    print(f"  GET http://0.0.0.0:{args.port}/roms/<arquivo>")
    print(f"  GET http://0.0.0.0:{args.port}/covers/<arquivo>")
    print(f"  GET http://0.0.0.0:{args.port}/health")
    print(f"")
    print(f"Configure no Switch: host = <seu IP>, port = {args.port}")
    print(f"Ctrl+C para parar.")
    print(f"")

    # Thread de regeneração periódica
    watcher = Thread(
        target=watch_and_regenerate,
        args=(base_dir, args.regenerate_interval),
        daemon=True,
    )
    watcher.start()

    # Iniciar servidor
    os.chdir(base_dir)
    server = HTTPServer(("0.0.0.0", args.port), SyncHandler)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[!] Servidor encerrado.")
        server.shutdown()


if __name__ == "__main__":
    main()
