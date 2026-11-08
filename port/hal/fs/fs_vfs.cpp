/**
 * @file fs_vfs.cpp
 * @brief Native Virtual Filesystem (VFS) implementation streaming ROM assets from files/.
 */

#include "port_vfs.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <mutex>
#include <sys/stat.h>

namespace {

std::string gBaseAssetDir = "files";
std::mutex gVfsMutex;
std::unordered_map<const FSFile*, FILE*> gOpenFiles;
thread_local char tResolvedPathBuffer[1024];

bool FileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
}

} // anonymous namespace

extern "C" {

bool NativeVFS_Init(const char* baseAssetDir) {
    std::lock_guard<std::mutex> lock(gVfsMutex);
    if (baseAssetDir && baseAssetDir[0] != '\0') {
        gBaseAssetDir = baseAssetDir;
    } else {
        gBaseAssetDir = "files";
    }

    // Normalize: strip trailing slash
    while (gBaseAssetDir.length() > 1 && gBaseAssetDir.back() == '/') {
        gBaseAssetDir.pop_back();
    }
    return true;
}

const char* NativeVFS_ResolvePath(const char* ndsPath) {
    if (!ndsPath || ndsPath[0] == '\0') {
        tResolvedPathBuffer[0] = '\0';
        return tResolvedPathBuffer;
    }

    std::string path(ndsPath);

    // Strip common NitroFS prefixes
    const char* prefixes[] = {"rom:/", "data:/", "fat:/", "nitro:/", "narc:/"};
    for (const char* prefix : prefixes) {
        size_t len = strlen(prefix);
        if (path.rfind(prefix, 0) == 0) {
            path = path.substr(len);
            break;
        }
    }

    // Strip leading slash
    while (!path.empty() && path[0] == '/') {
        path.erase(0, 1);
    }

    // Check if path already starts with base directory
    std::string candidate;
    std::lock_guard<std::mutex> lock(gVfsMutex);
    if (path.rfind(gBaseAssetDir + "/", 0) == 0) {
        candidate = path;
    } else {
        candidate = gBaseAssetDir + "/" + path;
    }

    // If candidate doesn't exist, try alternative paths (e.g. "../files")
    if (!FileExists(candidate)) {
        std::string fallback = "../" + candidate;
        if (FileExists(fallback)) {
            candidate = fallback;
        } else if (FileExists(path)) {
            candidate = path;
        }
    }

    strncpy(tResolvedPathBuffer, candidate.c_str(), sizeof(tResolvedPathBuffer) - 1);
    tResolvedPathBuffer[sizeof(tResolvedPathBuffer) - 1] = '\0';
    return tResolvedPathBuffer;
}

void NativeVFS_Shutdown(void) {
    std::lock_guard<std::mutex> lock(gVfsMutex);
    for (auto& pair : gOpenFiles) {
        if (pair.second) {
            fclose(pair.second);
        }
    }
    gOpenFiles.clear();
}

void FS_Init(u32 default_dma_no) {
    (void)default_dma_no;
    NativeVFS_Init("files");
}

BOOL FS_IsAvailable(void) {
    return TRUE;
}

void FS_InitFile(FSFile* p_file) {
    if (!p_file) return;
    memset(p_file, 0, sizeof(FSFile));
}

BOOL FS_OpenFile(FSFile* p_file, const char* path) {
    if (!p_file || !path) return FALSE;

    const char* resolved = NativeVFS_ResolvePath(path);
    FILE* f = fopen(resolved, "rb");
    if (!f) {
        return FALSE;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    p_file->prop.file.top = 0;
    p_file->prop.file.bottom = (size >= 0) ? static_cast<u32>(size) : 0;
    p_file->prop.file.pos = 0;

    std::lock_guard<std::mutex> lock(gVfsMutex);
    gOpenFiles[p_file] = f;
    return TRUE;
}

int FS_ReadFile(FSFile* p_file, void* dst, s32 len) {
    if (!p_file || !dst || len <= 0) return 0;

    FILE* f = nullptr;
    {
        std::lock_guard<std::mutex> lock(gVfsMutex);
        auto it = gOpenFiles.find(p_file);
        if (it != gOpenFiles.end()) {
            f = it->second;
        }
    }

    if (!f) return -1;

    size_t bytesRead = fread(dst, 1, static_cast<size_t>(len), f);
    p_file->prop.file.pos += static_cast<u32>(bytesRead);
    return static_cast<int>(bytesRead);
}

BOOL FS_SeekFile(FSFile* p_file, s32 offset, FSSeekFileMode origin) {
    if (!p_file) return FALSE;

    FILE* f = nullptr;
    {
        std::lock_guard<std::mutex> lock(gVfsMutex);
        auto it = gOpenFiles.find(p_file);
        if (it != gOpenFiles.end()) {
            f = it->second;
        }
    }

    if (!f) return FALSE;

    int whence = SEEK_SET;
    if (origin == FS_SEEK_CUR) whence = SEEK_CUR;
    else if (origin == FS_SEEK_END) whence = SEEK_END;

    if (fseek(f, offset, whence) != 0) {
        return FALSE;
    }

    long pos = ftell(f);
    p_file->prop.file.pos = (pos >= 0) ? static_cast<u32>(pos) : 0;
    return TRUE;
}

BOOL FS_CloseFile(FSFile* p_file) {
    if (!p_file) return FALSE;

    FILE* f = nullptr;
    {
        std::lock_guard<std::mutex> lock(gVfsMutex);
        auto it = gOpenFiles.find(p_file);
        if (it != gOpenFiles.end()) {
            f = it->second;
            gOpenFiles.erase(it);
        }
    }

    if (f) {
        fclose(f);
    }
    return TRUE;
}

u32 FS_GetLength(const FSFile* p_file) {
    if (!p_file) return 0;
    return p_file->prop.file.bottom - p_file->prop.file.top;
}

} // extern "C"
