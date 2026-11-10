Encontrei o problema. Seu código tinha um `#endif` faltando logo depois dos includes específicos do Linux. Além disso, identifiquei outro problema importante na implementação Windows: a função `SHGetKnownFolderPath()` estava sendo utilizada incorretamente, podendo corromper o conteúdo da variável `downloads`.

Vou corrigir a estrutura e também melhorar a inicialização da pasta Downloads.

Abaixo está uma versão reestruturada, mantendo as funcionalidades originais e a compatibilidade com Windows e Linux.

## 1\. Novo `main.c` completo

Substitua TODO o conteúdo do seu arquivo `main.c` pelo código abaixo.

c

```c

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <shlobj.h>
#include <objbase.h>

#else

#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#endif

// =====================================
// CONFIGURACAO
// =====================================

typedef struct {
    const char *extensoes;
    const char *pasta;
} Tipo;

static const Tipo tipos[] = {
    {".png .jpg .jpeg .gif .webp .bmp", "Imagens"},
    {".pdf .doc .docx .txt .xls .xlsx .ppt .pptx", "Documentos"},
    {".mp4 .mkv .avi .mov .webm", "Videos"},
    {".mp3 .wav .flac .ogg", "Musicas"},
    {".zip .rar .7z .tar .gz", "Compactados"},
    {".exe .msi .deb .rpm .appimage", "Programas"},
    {".c .cpp .h .py .js .html .css .java .json", "Codigos"}
};

#define TOTAL (sizeof(tipos) / sizeof(tipos[0]))

#ifdef _WIN32
static wchar_t downloads[MAX_PATH];
#else
static char downloads[PATH_MAX];
#endif

// =====================================
// IDENTIFICAR EXTENSAO
// =====================================

#ifndef _WIN32

static const char *identificar(const char *nome) {

    const char *ext = strrchr(nome, '.');

    if (!ext)
        return "Outros";

    char extensao[32];

    snprintf(extensao, sizeof(extensao), "%s", ext);

    for (size_t i = 0; extensao[i]; i++) {
        extensao[i] = (char)tolower((unsigned char)extensao[i]);
    }

    for (size_t i = 0; i < TOTAL; i++) {

        char lista[256];

        snprintf(lista, sizeof(lista), "%s",
                 tipos[i].extensoes);

        char *item = strtok(lista, " ");

        while (item) {

            if (strcmp(item, extensao) == 0)
                return tipos[i].pasta;

            item = strtok(NULL, " ");
        }
    }

    return "Outros";
}

#endif

// =====================================
// DIRETORIO DOWNLOADS
// =====================================

static int obter_downloads(void) {

#ifdef _WIN32

    PWSTR caminho = NULL;

    HRESULT resultado = SHGetKnownFolderPath(
        &FOLDERID_Downloads,
        0,
        NULL,
        &caminho
    );

    if (FAILED(resultado) || caminho == NULL)
        return 0;

    size_t tamanho = wcslen(caminho);

    if (tamanho >= MAX_PATH) {
        CoTaskMemFree(caminho);
        return 0;
    }

    wcscpy(downloads, caminho);

    CoTaskMemFree(caminho);

    return 1;

#else

    const char *home = getenv("HOME");

    if (!home)
        return 0;

    int tamanho = snprintf(
        downloads,
        sizeof(downloads),
        "%s/Downloads",
        home
    );

    if (tamanho < 0 || (size_t)tamanho >= sizeof(downloads))
        return 0;

    return 1;

#endif
}

// =====================================
// MOVER ARQUIVO - WINDOWS
// =====================================

#ifdef _WIN32

static void mover(const wchar_t *nome) {

    wchar_t origem[MAX_PATH];
    wchar_t pasta[MAX_PATH];
    wchar_t destino[MAX_PATH];

    if (swprintf(
        origem,
        MAX_PATH,
        L"%ls\\%ls",
        downloads,
        nome
    ) < 0)
        return;

    DWORD atributos = GetFileAttributesW(origem);

    if (atributos == INVALID_FILE_ATTRIBUTES)
        return;

    if (atributos & FILE_ATTRIBUTE_DIRECTORY)
        return;

    wchar_t *ext = wcsrchr(nome, L'.');

    if (!ext)
        return;

    char extensao[32] = {0};

    if (!WideCharToMultiByte(
        CP_UTF8,
        0,
        ext,
        -1,
        extensao,
        sizeof(extensao),
        NULL,
        NULL
    ))
        return;

    for (size_t i = 0; extensao[i]; i++) {
        extensao[i] = (char)tolower((unsigned char)extensao[i]);
    }

    const char *categoria = "Outros";

    for (size_t i = 0; i < TOTAL; i++) {

        char lista[256];

        snprintf(lista, sizeof(lista), "%s",
                 tipos[i].extensoes);

        char *item = strtok(lista, " ");

        while (item) {

            if (strcmp(item, extensao) == 0) {
                categoria = tipos[i].pasta;
                break;
            }

            item = strtok(NULL, " ");
        }

        if (strcmp(categoria, "Outros") != 0)
            break;
    }

    wchar_t categoria_w[64];

    if (!MultiByteToWideChar(
        CP_UTF8,
        0,
        categoria,
        -1,
        categoria_w,
        64
    ))
        return;

    if (swprintf(
        pasta,
        MAX_PATH,
        L"%ls\\%ls",
        downloads,
        categoria_w
    ) < 0)
        return;

    CreateDirectoryW(pasta, NULL);

    if (swprintf(
        destino,
        MAX_PATH,
        L"%ls\\%ls",
        pasta,
        nome
    ) < 0)
        return;

    // Nunca sobrescrever arquivos existentes.

    if (GetFileAttributesW(destino) != INVALID_FILE_ATTRIBUTES)
        return;

    if (MoveFileW(origem, destino)) {
        wprintf(L"Organizado: %ls\n", nome);
    }
}

// =====================================
// MOVER ARQUIVO - LINUX
// =====================================

#else

static void mover(const char *nome) {

    char origem[PATH_MAX];
    char pasta[PATH_MAX];
    char destino[PATH_MAX];

    int tamanho = snprintf(
        origem,
        sizeof(origem),
        "%s/%s",
        downloads,
        nome
    );

    if (tamanho < 0 || (size_t)tamanho >= sizeof(origem))
        return;

    struct stat st;

    if (stat(origem, &st) != 0)
        return;

    if (!S_ISREG(st.st_mode))
        return;

    const char *categoria = identificar(nome);

    tamanho = snprintf(
        pasta,
        sizeof(pasta),
        "%s/%s",
        downloads,
        categoria
    );

    if (tamanho < 0 || (size_t)tamanho >= sizeof(pasta))
        return;

    if (mkdir(pasta, 0755) != 0 && errno != EEXIST)
        return;

    tamanho = snprintf(
        destino,
        sizeof(destino),
        "%s/%s",
        pasta,
        nome
    );

    if (tamanho < 0 || (size_t)tamanho >= sizeof(destino))
        return;

    // link cria o destino sem sobrescrever arquivos.
    // unlink remove a origem somente após o sucesso.

    if (link(origem, destino) == 0) {

        if (unlink(origem) == 0) {
            printf("Organizado: %s\n", nome);
        } else {
            unlink(destino);
        }
    }
}

#endif

// =====================================
// MONITORAMENTO - WINDOWS
// =====================================

#ifdef _WIN32

static void monitorar(void) {

    HANDLE dir = CreateFileW(
        downloads,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (dir == INVALID_HANDLE_VALUE) {
        printf("Erro ao abrir pasta Downloads.\n");
        return;
    }

    // Buffer alinhado para FILE_NOTIFY_INFORMATION.
    DWORD buffer[1024];

    printf("Monitorando Downloads...\n");

    while (1) {

        DWORD bytes = 0;

        BOOL resultado = ReadDirectoryChangesW(
            dir,
            buffer,
            sizeof(buffer),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytes,
            NULL,
            NULL
        );

        if (!resultado || bytes == 0)
            continue;

        FILE_NOTIFY_INFORMATION *evento =
            (FILE_NOTIFY_INFORMATION *)buffer;

        do {

            if (evento->Action == FILE_ACTION_ADDED ||
                evento->Action == FILE_ACTION_RENAMED_NEW_NAME) {

                int n = (int)(
                    evento->FileNameLength / sizeof(wchar_t)
                );

                if (n > 0 && n < MAX_PATH) {

                    wchar_t nome[MAX_PATH];

                    wmemcpy(nome, evento->FileName, (size_t)n);

                    nome[n] = L'\0';

                    // Aguarda a finalizacao do download.
                    Sleep(1500);

                    mover(nome);
                }
            }

            if (!evento->NextEntryOffset)
                break;

            evento = (FILE_NOTIFY_INFORMATION *)(
                (char *)evento + evento->NextEntryOffset
            );

        } while (1);
    }

    CloseHandle(dir);
}

// =====================================
// MONITORAMENTO - LINUX
// =====================================

#else

static void monitorar(void) {

    int fd = inotify_init1(IN_CLOEXEC);

    if (fd < 0) {
        perror("Erro ao iniciar inotify");
        return;
    }

    int watch = inotify_add_watch(
        fd,
        downloads,
        IN_CLOSE_WRITE | IN_MOVED_TO
    );

    if (watch < 0) {
        perror("Erro ao monitorar Downloads");
        close(fd);
        return;
    }

    char buffer[4096];

    printf("Monitorando Downloads...\n");

    while (1) {

        ssize_t tamanho = read(
            fd,
            buffer,
            sizeof(buffer)
        );

        if (tamanho < 0) {

            if (errno == EINTR)
                continue;

            perror("Erro durante monitoramento");
            break;
        }

        if (tamanho == 0)
            continue;

        ssize_t pos = 0;

        while (pos + (ssize_t)sizeof(struct inotify_event) <= tamanho) {

            struct inotify_event *evento =
                (struct inotify_event *)(buffer + pos);

            size_t tamanho_evento =
                sizeof(struct inotify_event) + evento->len;

            if (pos + (ssize_t)tamanho_evento > tamanho)
                break;

            if (!(evento->mask & IN_ISDIR) &&
                evento->len > 0 &&
                evento->name[0] != '\0') {

                mover(evento->name);
            }

            pos += (ssize_t)tamanho_evento;
        }
    }

    inotify_rm_watch(fd, watch);

    close(fd);
}

#endif

// =====================================
// MAIN
// =====================================

int main(void) {

    if (!obter_downloads()) {

        printf("Erro ao localizar Downloads.\n");

        return 1;
    }

    printf("Organizador de Downloads iniciado.\n");

    printf("Pressione Ctrl+C para encerrar.\n");

    monitorar();

    return 0;
}
```
