
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <errno.h>
#endif

// =====================================
// CONFIGURAÇÃO
// =====================================

typedef struct {
    const char *extensoes;
    const char *pasta;
} Tipo;

Tipo tipos[] = {
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
wchar_t downloads[MAX_PATH];
#else
char downloads[PATH_MAX];
#endif

// =====================================
// DIRETÓRIO DOWNLOADS
// =====================================

int obter_downloads() {
#ifdef _WIN32
    return SUCCEEDED(SHGetKnownFolderPath(
        &FOLDERID_Downloads, 0, NULL,
        (PWSTR *)&downloads
    ));
#else
    const char *home = getenv("HOME");

    if (!home) return 0;

    snprintf(downloads, sizeof(downloads),
             "%s/Downloads", home);

    return 1;
#endif
}

// =====================================
// IDENTIFICAR EXTENSÃO
// =====================================

const char *identificar(const char *nome) {
    const char *ext = strrchr(nome, '.');

    if (!ext) return "Outros";

    char extensao[32];

    snprintf(extensao, sizeof(extensao), "%s", ext);

    for (int i = 0; extensao[i]; i++)
        extensao[i] = tolower((unsigned char)extensao[i]);

    for (int i = 0; i < TOTAL; i++) {
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

// =====================================
// MOVER ARQUIVO
// =====================================

#ifdef _WIN32

void mover(const wchar_t *nome) {
    wchar_t origem[MAX_PATH];
    wchar_t pasta[MAX_PATH];
    wchar_t destino[MAX_PATH];

    swprintf(origem, MAX_PATH, L"%ls\\%ls",
             downloads, nome);

    DWORD atributos = GetFileAttributesW(origem);

    if (atributos == INVALID_FILE_ATTRIBUTES ||
        (atributos & FILE_ATTRIBUTE_DIRECTORY))
        return;

    wchar_t *ext = wcsrchr(nome, L'.');
    if (!ext) return;

    char extensao[32] = {0};

    WideCharToMultiByte(CP_UTF8, 0, ext, -1,
                        extensao, sizeof(extensao),
                        NULL, NULL);

    for (int i = 0; extensao[i]; i++)
        extensao[i] = tolower((unsigned char)extensao[i]);

    const char *categoria = "Outros";

    for (int i = 0; i < TOTAL; i++) {
        char lista[256];

        snprintf(lista, sizeof(lista), "%s",
                 tipos[i].extensoes);

        char *item = strtok(lista, " ");

        while (item) {
            if (!strcmp(item, extensao))
                categoria = tipos[i].pasta;

            item = strtok(NULL, " ");
        }

        if (strcmp(categoria, "Outros"))
            break;
    }

    wchar_t categoria_w[64];

    MultiByteToWideChar(CP_UTF8, 0, categoria, -1,
                        categoria_w, 64);

    swprintf(pasta, MAX_PATH, L"%ls\\%ls",
             downloads, categoria_w);

    CreateDirectoryW(pasta, NULL);

    swprintf(destino, MAX_PATH, L"%ls\\%ls",
             pasta, nome);

    // Nunca sobrescrever arquivos.
    if (GetFileAttributesW(destino) != INVALID_FILE_ATTRIBUTES)
        return;

    if (MoveFileW(origem, destino))
        wprintf(L"Organizado: %ls\n", nome);
}

#else

void mover(const char *nome) {
    char origem[PATH_MAX];
    char pasta[PATH_MAX];
    char destino[PATH_MAX];

    snprintf(origem, sizeof(origem), "%s/%s",
             downloads, nome);

    struct stat st;

    if (stat(origem, &st) != 0 || !S_ISREG(st.st_mode))
        return;

    const char *categoria = identificar(nome);

    snprintf(pasta, sizeof(pasta), "%s/%s",
             downloads, categoria);

    if (mkdir(pasta, 0755) != 0 && errno != EEXIST)
        return;

    snprintf(destino, sizeof(destino), "%s/%s",
             pasta, nome);

    // rename não deve sobrescrever arquivos.
    // link + unlink evita sobrescrita no mesmo filesystem.
    if (link(origem, destino) == 0) {
        if (unlink(origem) == 0)
            printf("Organizado: %s\n", nome);
        else
            unlink(destino);
    }
}

#endif

// =====================================
// MONITORAMENTO
// =====================================

#ifdef _WIN32

void monitorar() {
    HANDLE dir = CreateFileW(
        downloads,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (dir == INVALID_HANDLE_VALUE) return;

    char buffer[4096];

    printf("Monitorando Downloads...\n");

    while (1) {
        DWORD bytes;

        if (!ReadDirectoryChangesW(
            dir, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytes, NULL, NULL
        )) continue;

        FILE_NOTIFY_INFORMATION *evento =
            (FILE_NOTIFY_INFORMATION *)buffer;

        do {
            if (evento->Action == FILE_ACTION_ADDED ||
                evento->Action == FILE_ACTION_RENAMED_NEW_NAME) {

                int n = evento->FileNameLength / sizeof(wchar_t);

                if (n >= MAX_PATH) break;

                wchar_t nome[MAX_PATH];

                wmemcpy(nome, evento->FileName, n);
                nome[n] = L'\0';

                // Aguarda o arquivo terminar de ser gravado.
                Sleep(1500);

                mover(nome);
            }

            if (!evento->NextEntryOffset) break;

            evento = (FILE_NOTIFY_INFORMATION *)
                ((char *)evento + evento->NextEntryOffset);

        } while (1);
    }

    CloseHandle(dir);
}

#else

void monitorar() {
    int fd = inotify_init1(IN_CLOEXEC);

    if (fd < 0) return;

    if (inotify_add_watch(fd, downloads,
        IN_CLOSE_WRITE | IN_MOVED_TO) < 0) {
        close(fd);
        return;
    }

    char buffer[4096];

    printf("Monitorando Downloads...\n");

    while (1) {
        int tamanho = read(fd, buffer, sizeof(buffer));

        if (tamanho < 0) {
            if (errno == EINTR) continue;
            break;
        }

        int pos = 0;

        while (pos < tamanho) {
            struct inotify_event *evento =
                (struct inotify_event *)(buffer + pos);

            if (!(evento->mask & IN_ISDIR) &&
                evento->len > 0) {
                mover(evento->name);
            }

            pos += sizeof(struct inotify_event) + evento->len;
        }
    }

    close(fd);
}

#endif

// =====================================
// MAIN
// =====================================

int main() {
    if (!obter_downloads()) {
        printf("Erro ao localizar Downloads.\n");
        return 1;
    }

    printf("Organizador de Downloads iniciado.\n");

    monitorar();

    return 0;
}
