
# 📂 Organizador Automático de Downloads

Um organizador automático de arquivos desenvolvido em **linguagem C**, que monitora a pasta `Downloads` e organiza os arquivos em pastas de acordo com suas extensões.

O programa funciona em segundo plano, identificando novos arquivos adicionados à pasta e movendo-os automaticamente para suas respectivas categorias.

## ✨ Funcionalidades

- 🔍 Monitoramento automático da pasta Downloads.
- 📁 Organização de arquivos por categorias.
- ⚡ Identificação automática de extensões.
- 🛡️ Proteção contra sobrescrita de arquivos existentes.
- 🖥️ Compatibilidade com Windows e Linux.
- 🚀 Execução contínua em segundo plano no terminal.
- 🔄 Detecção de arquivos baixados ou movidos para a pasta.

## 📦 Categorias de organização

| Categoria | Extensões |
|---|---|
| 🖼️ Imagens | PNG, JPG, JPEG, GIF, WEBP, BMP |
| 📄 Documentos | PDF, DOC, DOCX, TXT, XLS, XLSX, PPT, PPTX |
| 🎬 Vídeos | MP4, MKV, AVI, MOV, WEBM |
| 🎵 Músicas | MP3, WAV, FLAC, OGG |
| 📦 Compactados | ZIP, RAR, 7Z, TAR, GZ |
| 💻 Programas | EXE, MSI, DEB, RPM, APPIMAGE |
| 💻 Códigos | C, CPP, H, PY, JS, HTML, CSS, JAVA, JSON |
| 📂 Outros | Extensões não reconhecidas |

## ⚙️ Tecnologias utilizadas

- Linguagem C.
- Windows API para monitoramento de diretórios no Windows.
- `inotify` para monitoramento de diretórios no Linux.
- Manipulação de arquivos e diretórios utilizando bibliotecas nativas do sistema operacional.

## 💻 Compatibilidade

| Sistema operacional | Suporte |
|---|---|
| Windows | ✅ |
| Linux | ✅ |
| macOS | ❌ Não implementado |

## 🔧 Requisitos

Para compilar o projeto, você precisa de:

- GCC ou outro compilador C compatível.
- Windows com MinGW ou ambiente GCC compatível.
- Linux com suporte a `inotify`.

## 📥 Instalação

Clone o repositório:

```bash
git clone https://github.com/SEU-USUARIO/organizador-downloads.git
```

Entre na pasta:

```bash
cd organizador-downloads
```

## 🛠️ Compilação

### Windows

Utilizando MinGW:

```bash
gcc main.c -o organizador.exe -lshell32
```

### Linux

Utilizando GCC:

```bash
gcc main.c -o organizador
```

## ▶️ Execução

### Windows

```bash
organizador.exe
```

### Linux

```bash
./organizador
```

Após iniciar, o programa localizará automaticamente a pasta Downloads e começará a monitorá-la.

Quando um novo arquivo for detectado, ele será movido para a pasta correspondente.

## 📁 Exemplo de organização

Antes:

```text
Downloads/
├── imagem.png
├── documento.pdf
├── musica.mp3
├── video.mp4
├── projeto.zip
└── script.py
```

Depois:

```text
Downloads/
├── Imagens/
│   └── imagem.png
│
├── Documentos/
│   └── documento.pdf
│
├── Musicas/
│   └── musica.mp3
│
├── Videos/
│   └── video.mp4
│
├── Compactados/
│   └── projeto.zip
│
└── Codigos/
    └── script.py
```

## 🛡️ Segurança e funcionamento

O projeto possui algumas medidas para evitar problemas durante a organização:

- Não move diretórios, apenas arquivos regulares.
- Não sobrescreve arquivos que já existem no destino.
- No Windows, aguarda um pequeno intervalo após detectar um arquivo para permitir que o download termine.
- No Linux, utiliza eventos `IN_CLOSE_WRITE` e `IN_MOVED_TO` para detectar arquivos finalizados ou movidos.

**Importante:** o programa move os arquivos originais. Portanto, eles deixam de ficar diretamente na raiz de Downloads e passam para suas respectivas pastas.

Arquivos com nomes já existentes no destino não são sobrescritos.

## 📌 Observações

- O programa precisa permanecer em execução para continuar monitorando a pasta.
- A pasta Downloads é identificada automaticamente.
- Arquivos com extensões não cadastradas são classificados como `Outros`.
- A organização acontece localmente, sem necessidade de conexão com a internet.
- O encerramento do programa interrompe o monitoramento.

## 📄 Licença

Este projeto pode ser disponibilizado sob a licença MIT.

Consulte o arquivo `LICENSE` para obter os termos de utilização.

---

Desenvolvido em **C** ❤️

Um projeto simples para automatizar e facilitar a organização dos seus downloads.
