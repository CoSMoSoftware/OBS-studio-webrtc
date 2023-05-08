/*
 * Copyright (c) 2017-2018 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "updater.hpp"

#include <psapi.h>

#include <util/windows/CoTaskMemPtr.hpp>

#include <future>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <queue>

using namespace std;
using namespace json11;

/* ----------------------------------------------------------------------- */

HANDLE cancelRequested = nullptr;
HANDLE updateThread = nullptr;
HINSTANCE hinstMain = nullptr;
HWND hwndMain = nullptr;
HCRYPTPROV hProvider = 0;

static bool bExiting = false;
static bool updateFailed = false;

static bool downloadThreadFailure = false;

int totalFileSize = 0;
int completedFileSize = 0;
static int completedUpdates = 0;

static wchar_t tempPath[MAX_PATH];
static wchar_t obs_base_directory[MAX_PATH];

struct LastError {
	DWORD code;
	inline LastError() { code = GetLastError(); }
};

void FreeWinHttpHandle(HINTERNET handle)
{
	WinHttpCloseHandle(handle);
}

/* ----------------------------------------------------------------------- */

static inline bool HasVS2019Redist2()
{
	wchar_t base[MAX_PATH];
	wchar_t path[MAX_PATH];
	WIN32_FIND_DATAW wfd;
	HANDLE handle;

	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, base);

#define check_dll_installed(dll)                                    \
	do {                                                        \
		StringCbCopyW(path, sizeof(path), base);            \
		StringCbCatW(path, sizeof(path), L"\\" dll ".dll"); \
		handle = FindFirstFileW(path, &wfd);                \
		if (handle == INVALID_HANDLE_VALUE) {               \
			return false;                               \
		} else {                                            \
			FindClose(handle);                          \
		}                                                   \
	} while (false)

	check_dll_installed(L"msvcp140");
	check_dll_installed(L"vcruntime140");
	check_dll_installed(L"vcruntime140_1");

#undef check_dll_installed

	return true;
}

static bool HasVS2019Redist()
{
	PVOID old = nullptr;
	bool redirect = !!Wow64DisableWow64FsRedirection(&old);
	bool success = HasVS2019Redist2();
	if (redirect)
		Wow64RevertWow64FsRedirection(old);
	return success;
}

static void Status(const wchar_t *fmt, ...)
{
	wchar_t str[512];

	va_list argptr;
	va_start(argptr, fmt);

	StringCbVPrintf(str, sizeof(str), fmt, argptr);

	SetDlgItemText(hwndMain, IDC_STATUS, str);

	va_end(argptr);
}

static void CreateFoldersForPath(const wchar_t *path)
{
	wchar_t *p = (wchar_t *)path;

	while (*p) {
		if (*p == '\\' || *p == '/') {
			*p = 0;
			CreateDirectory(path, nullptr);
			*p = '\\';
		}
		p++;
	}
}

static bool MyCopyFile(const wchar_t *src, const wchar_t *dest)
try {
	WinHandle hSrc;
	WinHandle hDest;

	hSrc = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, nullptr,
			  OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (!hSrc.Valid())
		throw LastError();

	hDest = CreateFile(dest, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0,
			   nullptr);
	if (!hDest.Valid())
		throw LastError();

	BYTE buf[65536];
	DWORD read, wrote;

	for (;;) {
		if (!ReadFile(hSrc, buf, sizeof(buf), &read, nullptr))
			throw LastError();

		if (read == 0)
			break;

		if (!WriteFile(hDest, buf, read, &wrote, nullptr))
			throw LastError();

		if (wrote != read)
			return false;
	}

	return true;

} catch (LastError error) {
	SetLastError(error.code);
	return false;
}

static void MyDeleteFile(const wstring &filename)
{
	/* Try straightforward delete first */
	if (DeleteFile(filename.c_str()))
		return;

	DWORD err = GetLastError();
	if (err == ERROR_FILE_NOT_FOUND)
		return;

	/* If all else fails, schedule the file to be deleted on reboot */
	MoveFileEx(filename.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

static bool IsSafeFilename(const wchar_t *path)
{
	const wchar_t *p = path;

	if (!*p)
		return false;

	if (wcsstr(path, L".."))
		return false;

	if (*p == '/')
		return false;

	while (*p) {
		if (!isalnum(*p) && *p != '.' && *p != '/' && *p != '_' &&
		    *p != '-')
			return false;
		p++;
	}

	return true;
}

static string QuickReadFile(const wchar_t *path)
{
	string data;

	WinHandle handle = CreateFileW(path, GENERIC_READ, 0, nullptr,
				       OPEN_EXISTING, 0, nullptr);
	if (!handle.Valid()) {
		return string();
	}

	LARGE_INTEGER size;

	if (!GetFileSizeEx(handle, &size)) {
		return string();
	}

	data.resize((size_t)size.QuadPart);

	DWORD read;
	if (!ReadFile(handle, &data[0], (DWORD)data.size(), &read, nullptr)) {
		return string();
	}
	if (read != size.QuadPart) {
		return string();
	}

	return data;
}

/* ----------------------------------------------------------------------- */

enum state_t {
	STATE_INVALID,
	STATE_PENDING_DOWNLOAD,
	STATE_DOWNLOADING,
	STATE_DOWNLOADED,
	STATE_ALREADY_DOWNLOADED,
	STATE_INSTALL_FAILED,
	STATE_INSTALLED,
};

struct update_t {
	wstring sourceURL;
	wstring outputPath;
	wstring tempPath;
	wstring previousFile;
	wstring basename;
	string packageName;

	DWORD fileSize = 0;
	BYTE hash[BLAKE2_HASH_LENGTH];
	BYTE downloadhash[BLAKE2_HASH_LENGTH];
	BYTE my_hash[BLAKE2_HASH_LENGTH];
	state_t state = STATE_INVALID;
	bool has_hash = false;
	bool patchable = false;
	bool compressed = false;

	inline update_t() {}
	inline update_t(const update_t &from)
		: sourceURL(from.sourceURL),
		  outputPath(from.outputPath),
		  tempPath(from.tempPath),
		  previousFile(from.previousFile),
		  basename(from.basename),
		  packageName(from.packageName),
		  fileSize(from.fileSize),
		  state(from.state),
		  has_hash(from.has_hash),
		  patchable(from.patchable),
		  compressed(from.compressed)
	{
		memcpy(hash, from.hash, sizeof(hash));
		memcpy(downloadhash, from.downloadhash, sizeof(downloadhash));
		memcpy(my_hash, from.my_hash, sizeof(my_hash));
	}

	inline update_t(update_t &&from)
		: sourceURL(std::move(from.sourceURL)),
		  outputPath(std::move(from.outputPath)),
		  tempPath(std::move(from.tempPath)),
		  previousFile(std::move(from.previousFile)),
		  basename(std::move(from.basename)),
		  packageName(std::move(from.packageName)),
		  fileSize(from.fileSize),
		  state(from.state),
		  has_hash(from.has_hash),
		  patchable(from.patchable),
		  compressed(from.compressed)
	{
		from.state = STATE_INVALID;

		memcpy(hash, from.hash, sizeof(hash));
		memcpy(downloadhash, from.downloadhash, sizeof(downloadhash));
		memcpy(my_hash, from.my_hash, sizeof(my_hash));
	}

	void CleanPartialUpdate()
	{
		if (state == STATE_INSTALL_FAILED || state == STATE_INSTALLED) {
			if (!previousFile.empty()) {
				DeleteFile(outputPath.c_str());
				MyCopyFile(previousFile.c_str(),
					   outputPath.c_str());
				DeleteFile(previousFile.c_str());
			} else {
				DeleteFile(outputPath.c_str());
			}
			if (state == STATE_INSTALL_FAILED)
				DeleteFile(tempPath.c_str());
		} else if (state == STATE_DOWNLOADED) {
			DeleteFile(tempPath.c_str());
		}
	}

	inline update_t &operator=(const update_t &from)
	{
		sourceURL = from.sourceURL;
		outputPath = from.outputPath;
		tempPath = from.tempPath;
		previousFile = from.previousFile;
		basename = from.basename;
		packageName = from.packageName;
		fileSize = from.fileSize;
		state = from.state;
		has_hash = from.has_hash;
		patchable = from.patchable;
		compressed = from.compressed;

		memcpy(hash, from.hash, sizeof(hash));
		memcpy(downloadhash, from.downloadhash, sizeof(downloadhash));
		memcpy(my_hash, from.my_hash, sizeof(my_hash));

		return *this;
	}
};

struct deletion_t {
	wstring originalFilename;
	wstring deleteMeFilename;

	void UndoRename()
	{
		if (!deleteMeFilename.empty())
			MoveFile(deleteMeFilename.c_str(),
				 originalFilename.c_str());
	}
};

static unordered_map<string, wstring> hashes;
static vector<update_t> updates;
static vector<deletion_t> deletions;
static mutex updateMutex;

static inline void CleanupPartialUpdates()
{
	for (update_t &update : updates)
		update.CleanPartialUpdate();

	for (deletion_t &deletion : deletions)
		deletion.UndoRename();
}

/* ----------------------------------------------------------------------- */

bool DownloadWorkerThread()
{
	const DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
				   WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

	const DWORD enableHTTP2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;

	const DWORD compressionFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

	HttpHandle hSession = WinHttpOpen(L"OBS Studio Updater/3.0",
					  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
					  WINHTTP_NO_PROXY_NAME,
					  WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		downloadThreadFailure = true;
		Status(L"Update failed: Couldn't open obsproject.com");
		return false;
	}

	WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS,
			 (LPVOID)&tlsProtocols, sizeof(tlsProtocols));

	WinHttpSetOption(hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
			 (LPVOID)&enableHTTP2Flag, sizeof(enableHTTP2Flag));

	WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION,
			 (LPVOID)&compressionFlags, sizeof(compressionFlags));

	HttpHandle hConnect = WinHttpConnect(hSession,
					     L"cdn-fastly.obsproject.com",
					     INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) {
		downloadThreadFailure = true;
		Status(L"Update failed: Couldn't connect to cdn-fastly.obsproject.com");
		return false;
	}

	ZSTDDCtx zCtx;

	for (;;) {
		bool foundWork = false;

		unique_lock<mutex> ulock(updateMutex);

		for (update_t &update : updates) {
			int responseCode;

			DWORD waitResult =
				WaitForSingleObject(cancelRequested, 0);
			if (waitResult == WAIT_OBJECT_0) {
				return false;
			}

			if (update.state != STATE_PENDING_DOWNLOAD)
				continue;

			update.state = STATE_DOWNLOADING;

			ulock.unlock();

			foundWork = true;

			if (downloadThreadFailure) {
				return false;
			}

			Status(L"Downloading %s", update.outputPath.c_str());

			if (!HTTPGetFile(hConnect, update.sourceURL.c_str(),
					 update.tempPath.c_str(),
					 L"Accept-Encoding: gzip",
					 &responseCode)) {

				downloadThreadFailure = true;
				DeleteFile(update.tempPath.c_str());
				Status(L"Update failed: Could not download "
				       L"%s (error code %d)",
				       update.outputPath.c_str(), responseCode);
				return 1;
			}

			if (responseCode != 200) {
				downloadThreadFailure = true;
				DeleteFile(update.tempPath.c_str());
				Status(L"Update failed: Could not download "
				       L"%s (error code %d)",
				       update.outputPath.c_str(), responseCode);
				return 1;
			}

			BYTE downloadHash[BLAKE2_HASH_LENGTH];
			if (!CalculateFileHash(update.tempPath.c_str(),
					       downloadHash)) {
				downloadThreadFailure = true;
				DeleteFile(update.tempPath.c_str());
				Status(L"Update failed: Couldn't verify "
				       L"integrity of %s",
				       update.outputPath.c_str());
				return 1;
			}

			if (memcmp(update.downloadhash, downloadHash, 20)) {
				downloadThreadFailure = true;
				DeleteFile(update.tempPath.c_str());
				Status(L"Update failed: Integrity check "
				       L"failed on %s",
				       update.outputPath.c_str());
				return 1;
			}

			if (update.compressed && !update.patchable) {
				int res = DecompressFile(
					zCtx, update.tempPath.c_str(),
					update.fileSize);
				if (res) {
					downloadThreadFailure = true;
					DeleteFile(update.tempPath.c_str());
					Status(L"Update failed: Decompression "
					       L"failed on %s (error code %d)",
					       update.outputPath.c_str(), res);
					return 1;
				}
			}

			ulock.lock();

			update.state = STATE_DOWNLOADED;
			completedUpdates++;
		}

		if (!foundWork) {
			break;
		}
		if (downloadThreadFailure) {
			return false;
		}
	}

	return true;
}

static bool RunDownloadWorkers(int num)
try {
	vector<future<bool>> thread_success_results;
	thread_success_results.resize(num);

	for (future<bool> &result : thread_success_results) {
		result = async(DownloadWorkerThread);
	}
	for (future<bool> &result : thread_success_results) {
		if (!result.get()) {
			return false;
		}
	}

	return true;

} catch (...) {
	return false;
}

/* ----------------------------------------------------------------------- */

#define WAITIFOBS_SUCCESS 0
#define WAITIFOBS_WRONG_PROCESS 1
#define WAITIFOBS_CANCELLED 2

static inline DWORD WaitIfOBS(DWORD id, const wchar_t *expected)
{
	wchar_t path[MAX_PATH];
	wchar_t *name;
	DWORD path_len = _countof(path);

	*path = 0;

	WinHandle proc = OpenProcess(PROCESS_QUERY_INFORMATION |
					     PROCESS_VM_READ | SYNCHRONIZE,
				     false, id);
	if (!proc.Valid())
		return WAITIFOBS_WRONG_PROCESS;

	if (!QueryFullProcessImageNameW(proc, 0, path, &path_len))
		return WAITIFOBS_WRONG_PROCESS;

	// check it's actually our exe that's running
	size_t len = wcslen(obs_base_directory);
	if (wcsncmp(path, obs_base_directory, len))
		return WAITIFOBS_WRONG_PROCESS;

	name = wcsrchr(path, L'\\');
	if (name)
		name += 1;
	else
		name = path;

	if (_wcsnicmp(name, expected, 5) == 0) {
		HANDLE hWait[2];
		hWait[0] = proc;
		hWait[1] = cancelRequested;

		int i = WaitForMultipleObjects(2, hWait, false, INFINITE);
		if (i == WAIT_OBJECT_0 + 1)
			return WAITIFOBS_CANCELLED;

		return WAITIFOBS_SUCCESS;
	}

	return WAITIFOBS_WRONG_PROCESS;
}

static bool WaitForOBS()
{
	DWORD proc_ids[1024], needed, count;

	if (!EnumProcesses(proc_ids, sizeof(proc_ids), &needed)) {
		return true;
	}

	count = needed / sizeof(DWORD);

	for (DWORD i = 0; i < count; i++) {
		DWORD id = proc_ids[i];
		if (id != 0) {
			switch (WaitIfOBS(id, L"obs64")) {
			case WAITIFOBS_SUCCESS:
				return true;
			case WAITIFOBS_WRONG_PROCESS:
				break;
			case WAITIFOBS_CANCELLED:
				return false;
			}
		}
	}

	return true;
}

/* ----------------------------------------------------------------------- */

static inline bool UTF8ToWide(wchar_t *wide, int wideSize, const char *utf8)
{
	return !!MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, wideSize);
}

static inline bool WideToUTF8(char *utf8, int utf8Size, const wchar_t *wide)
{
	return !!WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8Size,
				     nullptr, nullptr);
}

#define UTF8ToWideBuf(wide, utf8) UTF8ToWide(wide, _countof(wide), utf8)
#define WideToUTF8Buf(utf8, wide) WideToUTF8(utf8, _countof(utf8), wide)

/* ----------------------------------------------------------------------- */

queue<string> hashQueue;

void HasherThread()
{
	bool hasherThreadFailure = false;
	unique_lock<mutex> ulock(updateMutex, defer_lock);

	while (true) {
		ulock.lock();
		if (hashQueue.empty())
			return;

		auto fileName = hashQueue.front();
		hashQueue.pop();

		ulock.unlock();

		wchar_t updateFileName[MAX_PATH];

		if (!UTF8ToWideBuf(updateFileName, fileName.c_str()))
			continue;
		if (!IsSafeFilename(updateFileName))
			continue;

		BYTE existingHash[BLAKE2_HASH_LENGTH];
		wchar_t fileHashStr[BLAKE2_HASH_STR_LENGTH];

		if (CalculateFileHash(updateFileName, existingHash)) {
			HashToString(existingHash, fileHashStr);
			ulock.lock();
			hashes.emplace(fileName, fileHashStr);
			ulock.unlock();
		}
	}
}

static void RunHasherWorkers(int num, const Json &packages)
try {

	for (const Json &package : packages.array_items()) {
		for (const Json &file : package["files"].array_items()) {
			if (!file["name"].is_string())
				continue;
			hashQueue.push(file["name"].string_value());
		}
	}

	vector<future<void>> futures;
	futures.resize(num);

	for (auto &result : futures) {
		result = async(launch::async, HasherThread);
	}
	for (auto &result : futures) {
		result.wait();
	}
} catch (...) {
}

/* ----------------------------------------------------------------------- */

static inline bool FileExists(const wchar_t *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;

	hFind = FindFirstFileW(path, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	return hFind != INVALID_HANDLE_VALUE;
}

static bool NonCorePackageInstalled(const char *name)
{
	if (strcmp(name, "obs-browser") == 0)
		return FileExists(L"obs-plugins\\64bit\\obs-browser.dll");

	return false;
}

#define UPDATE_URL L"https://cdn-fastly.obsproject.com/update_studio"

static bool AddPackageUpdateFiles(const Json &root, size_t idx,
				  const wchar_t *tempPath,
				  const wchar_t *branch)
{
	const Json &package = root[idx];
	const Json &name = package["name"];
	const Json &files = package["files"];

	if (!files.is_array())
		return true;
	if (!name.is_string())
		return true;

	wchar_t wPackageName[512];
	const string &packageName = name.string_value();
	size_t fileCount = files.array_items().size();

	if (!UTF8ToWideBuf(wPackageName, packageName.c_str()))
		return false;

	if (packageName != "core" &&
	    !NonCorePackageInstalled(packageName.c_str()))
		return true;

	for (size_t j = 0; j < fileCount; j++) {
		const Json &file = files[j];
		const Json &fileName = file["name"];
		const Json &hash = file["hash"];
		const Json &dlHash = file["compressed_hash"];
		const Json &size = file["size"];

		if (!fileName.is_string())
			continue;
		if (!hash.is_string())
			continue;
		if (!size.is_number())
			continue;

		const string &fileUTF8 = fileName.string_value();
		const string &hashUTF8 = hash.string_value();
		const string &dlHashUTF8 = dlHash.string_value();
		int fileSize = size.int_value();

		if (hashUTF8.size() != BLAKE2_HASH_LENGTH * 2)
			continue;

		/* The download hash may not exist if a file is uncompressed */

		bool compressed = false;
		if (dlHashUTF8.size() == BLAKE2_HASH_LENGTH * 2)
			compressed = true;

		/* convert strings to wide */

		wchar_t sourceURL[1024];
		wchar_t updateFileName[MAX_PATH];
		wchar_t updateHashStr[BLAKE2_HASH_STR_LENGTH];
		wchar_t downloadHashStr[BLAKE2_HASH_STR_LENGTH];
		wchar_t tempFilePath[MAX_PATH];

		if (!UTF8ToWideBuf(updateFileName, fileUTF8.c_str()))
			continue;
		if (!UTF8ToWideBuf(updateHashStr, hashUTF8.c_str()))
			continue;
		if (compressed &&
		    !UTF8ToWideBuf(downloadHashStr, dlHashUTF8.c_str()))
			continue;

		/* make sure paths are safe */

		if (!IsSafeFilename(updateFileName)) {
			Status(L"Update failed: Unsafe path '%s' found in "
			       L"manifest",
			       updateFileName);
			return false;
		}

		StringCbPrintf(sourceURL, sizeof(sourceURL), L"%s/%s/%s/%s",
			       UPDATE_URL, branch, wPackageName,
			       updateFileName);
		StringCbPrintf(tempFilePath, sizeof(tempFilePath), L"%s\\%s",
			       tempPath, updateHashStr);

		/* Check file hash */

		wstring fileHashStr;
		bool has_hash;

		/* We don't really care if this fails, it's just to avoid
		 * wasting bandwidth by downloading unmodified files */
		if (hashes.count(fileUTF8)) {
			fileHashStr = hashes[fileUTF8];
			if (fileHashStr == updateHashStr)
				continue;

			has_hash = true;
		} else {
			has_hash = false;
		}

		/* Add update file */

		update_t update;
		update.fileSize = fileSize;
		update.basename = updateFileName;
		update.outputPath = updateFileName;
		update.tempPath = tempFilePath;
		update.sourceURL = sourceURL;
		update.packageName = packageName;
		update.state = STATE_PENDING_DOWNLOAD;
		update.patchable = false;
		update.compressed = compressed;

		StringToHash(updateHashStr, update.hash);

		if (compressed) {
			update.sourceURL += L".zst";
			StringToHash(downloadHashStr, update.downloadhash);
		} else {
			memcpy(update.downloadhash, update.hash,
			       sizeof(update.downloadhash));
		}

		update.has_hash = has_hash;
		if (has_hash)
			StringToHash(fileHashStr.data(), update.my_hash);

		updates.push_back(move(update));

		totalFileSize += fileSize;
	}

	return true;
}

static void AddPackageRemovedFiles(const Json &package)
{
	const Json &removed_files = package["removed_files"];
	if (!removed_files.is_array())
		return;

	for (auto &item : removed_files.array_items()) {
		if (!item.is_string())
			continue;

		wchar_t removedFileName[MAX_PATH];
		if (!UTF8ToWideBuf(removedFileName,
				   item.string_value().c_str()))
			continue;

		/* Ensure paths are safe, also check if file exists */
		if (!IsSafeFilename(removedFileName))
			continue;
		/* Technically GetFileAttributes can fail for other reasons,
		 * so double-check by also checking the last error */
		if (GetFileAttributesW(removedFileName) ==
		    INVALID_FILE_ATTRIBUTES) {
			int err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND ||
			    err == ERROR_PATH_NOT_FOUND)
				continue;
		}

		deletion_t deletion;
		deletion.originalFilename = removedFileName;

		deletions.push_back(deletion);
	}
}

static bool RenameRemovedFile(deletion_t &deletion)
{
	_TCHAR deleteMeName[MAX_PATH];
	_TCHAR randomStr[MAX_PATH];

	BYTE junk[40];
	BYTE hash[BLAKE2_HASH_LENGTH];

	CryptGenRandom(hProvider, sizeof(junk), junk);
	blake2b(hash, sizeof(hash), junk, sizeof(junk), NULL, 0);
	HashToString(hash, randomStr);
	randomStr[8] = 0;

	StringCbCopy(deleteMeName, sizeof(deleteMeName),
		     deletion.originalFilename.c_str());

	StringCbCat(deleteMeName, sizeof(deleteMeName), L".");
	StringCbCat(deleteMeName, sizeof(deleteMeName), randomStr);
	StringCbCat(deleteMeName, sizeof(deleteMeName), L".deleteme");

	if (MoveFile(deletion.originalFilename.c_str(), deleteMeName)) {
		/* Only set this if the file was successfully renamed */
		deletion.deleteMeFilename = deleteMeName;
		return true;
	}

	return false;
}

static void UpdateWithPatchIfAvailable(const char *name, const char *hash,
				       const char *source, int size)
{
	wchar_t widePatchableFilename[MAX_PATH];
	wchar_t widePatchHash[MAX_PATH];
	wchar_t sourceURL[1024];
	wchar_t patchHashStr[BLAKE2_HASH_STR_LENGTH];

	if (strncmp(source, "https://cdn-fastly.obsproject.com/", 34) != 0)
		return;

	string patchPackageName = name;

	const char *slash = strchr(name, '/');
	if (!slash)
		return;

	patchPackageName.resize(slash - name);
	name = slash + 1;

	if (!UTF8ToWideBuf(widePatchableFilename, name))
		return;
	if (!UTF8ToWideBuf(widePatchHash, hash))
		return;
	if (!UTF8ToWideBuf(sourceURL, source))
		return;
	if (!UTF8ToWideBuf(patchHashStr, hash))
		return;

	for (update_t &update : updates) {
		if (update.packageName != patchPackageName)
			continue;
		if (update.basename != widePatchableFilename)
			continue;

		StringToHash(patchHashStr, update.downloadhash);

		/* Replace the source URL with the patch file, mark it as
		 * patchable, and re-calculate download size */
		totalFileSize -= (update.fileSize - size);
		update.sourceURL = sourceURL;
		update.fileSize = size;
		update.patchable = true;

		/* Since the patch depends on the previous version, we can
		 * no longer rely on the temp name being unique to the
		 * new file's hash */
		update.tempPath = tempPath;
		update.tempPath += L"\\";
		update.tempPath += patchHashStr;
		break;
	}
}

static bool MoveInUseFileAway(update_t &file)
{
	_TCHAR deleteMeName[MAX_PATH];
	_TCHAR randomStr[MAX_PATH];

	BYTE junk[40];
	BYTE hash[BLAKE2_HASH_LENGTH];

	CryptGenRandom(hProvider, sizeof(junk), junk);
	blake2b(hash, sizeof(hash), junk, sizeof(junk), NULL, 0);
	HashToString(hash, randomStr);
	randomStr[8] = 0;

	StringCbCopy(deleteMeName, sizeof(deleteMeName),
		     file.outputPath.c_str());

	StringCbCat(deleteMeName, sizeof(deleteMeName), L".");
	StringCbCat(deleteMeName, sizeof(deleteMeName), randomStr);
	StringCbCat(deleteMeName, sizeof(deleteMeName), L".deleteme");

	if (MoveFile(file.outputPath.c_str(), deleteMeName)) {

		if (MyCopyFile(deleteMeName, file.outputPath.c_str())) {
			MoveFileEx(deleteMeName, NULL,
				   MOVEFILE_DELAY_UNTIL_REBOOT);

			return true;
		} else {
			MoveFile(deleteMeName, file.outputPath.c_str());
		}
	}

	return false;
}

static bool UpdateFile(ZSTD_DCtx *ctx, update_t &file)
{
	wchar_t oldFileRenamedPath[MAX_PATH];

	if (file.patchable)
		Status(L"Updating %s...", file.outputPath.c_str());
	else
		Status(L"Installing %s...", file.outputPath.c_str());

	/* Check if we're replacing an existing file or just installing a new
	 * one */
	DWORD attribs = GetFileAttributes(file.outputPath.c_str());

	if (attribs != INVALID_FILE_ATTRIBUTES) {
		wchar_t *curFileName = nullptr;
		wchar_t baseName[MAX_PATH];

		StringCbCopy(baseName, sizeof(baseName),
			     file.outputPath.c_str());

		curFileName = wcsrchr(baseName, '/');
		if (curFileName) {
			curFileName[0] = '\0';
			curFileName++;
		} else
			curFileName = baseName;

		/* Backup the existing file in case a rollback is needed */
		StringCbCopy(oldFileRenamedPath, sizeof(oldFileRenamedPath),
			     file.outputPath.c_str());
		StringCbCat(oldFileRenamedPath, sizeof(oldFileRenamedPath),
			    L".old");

		if (!MyCopyFile(file.outputPath.c_str(), oldFileRenamedPath)) {
			int is_sharing_violation =
				(GetLastError() == ERROR_SHARING_VIOLATION);

			if (is_sharing_violation)
				Status(L"Update failed: %s is still in use.  "
				       L"Close all programs and try again.",
				       curFileName);
			else
				Status(L"Update failed: Couldn't backup %s "
				       L"(error %d)",
				       curFileName, GetLastError());
			return false;
		}

		file.previousFile = oldFileRenamedPath;

		int error_code;
		bool installed_ok;
		bool already_tried_to_move = false;

	retryAfterMovingFile:

		if (file.patchable) {
			error_code = ApplyPatch(ctx, file.tempPath.c_str(),
						file.outputPath.c_str());
			installed_ok = (error_code == 0);

			if (installed_ok) {
				BYTE patchedFileHash[BLAKE2_HASH_LENGTH];
				if (!CalculateFileHash(file.outputPath.c_str(),
						       patchedFileHash)) {
					Status(L"Update failed: Couldn't "
					       L"verify integrity of patched %s",
					       curFileName);

					file.state = STATE_INSTALL_FAILED;
					return false;
				}

				if (memcmp(file.hash, patchedFileHash,
					   BLAKE2_HASH_LENGTH) != 0) {
					Status(L"Update failed: Integrity "
					       L"check of patched "
					       L"%s failed",
					       curFileName);

					file.state = STATE_INSTALL_FAILED;
					return false;
				}
			}
		} else {
			installed_ok = MyCopyFile(file.tempPath.c_str(),
						  file.outputPath.c_str());
			error_code = GetLastError();
		}

		if (!installed_ok) {
			int is_sharing_violation =
				(error_code == ERROR_SHARING_VIOLATION);

			if (is_sharing_violation) {
				if (!already_tried_to_move) {
					already_tried_to_move = true;

					if (MoveInUseFileAway(file))
						goto retryAfterMovingFile;
				}

				Status(L"Update failed: %s is still in use.  "
				       L"Close all "
				       L"programs and try again.",
				       curFileName);
			} else {
				DWORD err = GetLastError();
				Status(L"Update failed: Couldn't update %s "
				       L"(error %d)",
				       curFileName, err ? err : error_code);
			}

			file.state = STATE_INSTALL_FAILED;
			return false;
		}

		file.state = STATE_INSTALLED;
	} else {
		if (file.patchable) {
			/* Uh oh, we thought we could patch something but it's
			 * no longer there! */
			Status(L"Update failed: Source file %s not found",
			       file.outputPath.c_str());
			return false;
		}

		/* We may be installing into new folders,
		 * make sure they exist */
		CreateFoldersForPath(file.outputPath.c_str());

		file.previousFile = L"";

		bool success = !!MyCopyFile(file.tempPath.c_str(),
					    file.outputPath.c_str());
		if (!success) {
			Status(L"Update failed: Couldn't install %s (error %d)",
			       file.outputPath.c_str(), GetLastError());
			file.state = STATE_INSTALL_FAILED;
			return false;
		}

		file.state = STATE_INSTALLED;
	}

	return true;
}

queue<reference_wrapper<update_t>> updateQueue;
static int lastPosition = 0;
static int installed = 0;
static bool updateThreadFailed = false;

static bool UpdateWorker()
{
	unique_lock<mutex> ulock(updateMutex, defer_lock);
	ZSTDDCtx zCtx;

	while (true) {
		ulock.lock();

		if (updateThreadFailed)
			return false;
		if (updateQueue.empty())
			break;

		auto update = updateQueue.front();
		updateQueue.pop();
		ulock.unlock();

		if (!UpdateFile(zCtx, update)) {
			updateThreadFailed = true;
			return false;
		} else {
			int position = (int)(((float)++installed /
					      (float)completedUpdates) *
					     100.0f);
			if (position > lastPosition) {
				lastPosition = position;
				SendDlgItemMessage(hwndMain, IDC_PROGRESS,
						   PBM_SETPOS, position, 0);
			}
		}
	}

	return true;
}

static bool RunUpdateWorkers(int num)
try {
	for (update_t &update : updates) {
		updateQueue.push(update);
	}

	vector<future<bool>> thread_success_results;
	thread_success_results.resize(num);

	for (future<bool> &result : thread_success_results) {
		result = async(launch::async, UpdateWorker);
	}
	for (future<bool> &result : thread_success_results) {
		if (!result.get()) {
			return false;
		}
	}

	return true;

} catch (...) {
	return false;
}

#define PATCH_MANIFEST_URL \
	L"https://obsproject.com/update_studio/getpatchmanifest"
#define HASH_NULL L"0000000000000000000000000000000000000000"

static bool UpdateVS2019Redists(const Json &root)
{
	/* ------------------------------------------ *
	 * Initialize session                         */

	const DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

	const DWORD compressionFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

	HttpHandle hSession = WinHttpOpen(L"OBS Studio Updater/3.0",
					  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
					  WINHTTP_NO_PROXY_NAME,
					  WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		Status(L"Update failed: Couldn't open obsproject.com");
		return false;
	}

	WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS,
			 (LPVOID)&tlsProtocols, sizeof(tlsProtocols));

	WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION,
			 (LPVOID)&compressionFlags, sizeof(compressionFlags));

	HttpHandle hConnect = WinHttpConnect(hSession,
					     L"cdn-fastly.obsproject.com",
					     INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) {
		Status(L"Update failed: Couldn't connect to cdn-fastly.obsproject.com");
		return false;
	}

	int responseCode;

	DWORD waitResult = WaitForSingleObject(cancelRequested, 0);
	if (waitResult == WAIT_OBJECT_0) {
		return false;
	}

	/* ------------------------------------------ *
	 * Download redist                            */

	Status(L"Downloading Visual C++ 2019 Redistributable");

	wstring sourceURL =
		L"https://cdn-fastly.obsproject.com/downloads/VC_redist.x64.exe";

	wstring destPath;
	destPath += tempPath;
	destPath += L"\\VC_redist.x64.exe";

	if (!HTTPGetFile(hConnect, sourceURL.c_str(), destPath.c_str(),
			 L"Accept-Encoding: gzip", &responseCode)) {

		DeleteFile(destPath.c_str());
		Status(L"Update failed: Could not download "
		       L"%s (error code %d)",
		       L"Visual C++ 2019 Redistributable", responseCode);
		return false;
	}

	/* ------------------------------------------ *
	 * Get expected hash                          */

	const Json &redistJson = root["vc2019_redist_x64"];
	if (!redistJson.is_string()) {
		Status(L"Update failed: Could not parse VC2019 redist json");
		return false;
	}

	const string &expectedHashUTF8 = redistJson.string_value();
	wchar_t expectedHashWide[BLAKE2_HASH_STR_LENGTH];
	BYTE expectedHash[BLAKE2_HASH_LENGTH];

	if (!UTF8ToWideBuf(expectedHashWide, expectedHashUTF8.c_str())) {
		DeleteFile(destPath.c_str());
		Status(L"Update failed: Couldn't convert Json for redist hash");
		return false;
	}

	StringToHash(expectedHashWide, expectedHash);

	wchar_t downloadHashWide[BLAKE2_HASH_STR_LENGTH];
	BYTE downloadHash[BLAKE2_HASH_LENGTH];

	/* ------------------------------------------ *
	 * Get download hash                          */

	if (!CalculateFileHash(destPath.c_str(), downloadHash)) {
		DeleteFile(destPath.c_str());
		Status(L"Update failed: Couldn't verify integrity of %s",
		       L"Visual C++ 2019 Redistributable");
		return false;
	}

	/* ------------------------------------------ *
	 * If hashes do not match, integrity failed   */

	HashToString(downloadHash, downloadHashWide);
	if (wcscmp(expectedHashWide, downloadHashWide) != 0) {
		DeleteFile(destPath.c_str());
		Status(L"Update failed: Couldn't verify integrity of %s",
		       L"Visual C++ 2019 Redistributable");
		return false;
	}

	/* ------------------------------------------ *
	 * If hashes match, install redist            */

	wchar_t commandline[MAX_PATH + MAX_PATH];
	StringCbPrintf(commandline, sizeof(commandline),
		       L"%s /install /quiet /norestart", destPath.c_str());

	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {};
	si.cb = sizeof(si);

	bool success = !!CreateProcessW(destPath.c_str(), commandline, nullptr,
					nullptr, false, CREATE_NO_WINDOW,
					nullptr, nullptr, &si, &pi);
	if (success) {
		Status(L"Installing %s...", L"Visual C++ 2019 Redistributable");

		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
	} else {
		Status(L"Update failed: Could not execute "
		       L"%s (error code %d)",
		       L"Visual C++ 2019 Redistributable", (int)GetLastError());
	}

	DeleteFile(destPath.c_str());

	waitResult = WaitForSingleObject(cancelRequested, 0);
	if (waitResult == WAIT_OBJECT_0) {
		return false;
	}

	return success;
}

extern "C" void UpdateHookFiles(void);

static bool Update(wchar_t *cmdLine)
{
	/* ------------------------------------- *
	 * Check to make sure OBS isn't running  */

	HANDLE hObsUpdateMutex =
		OpenMutexW(SYNCHRONIZE, false, L"OBSStudioUpdateMutex");
	if (hObsUpdateMutex) {
		HANDLE hWait[2];
		hWait[0] = hObsUpdateMutex;
		hWait[1] = cancelRequested;

		int i = WaitForMultipleObjects(2, hWait, false, INFINITE);

		if (i == WAIT_OBJECT_0)
			ReleaseMutex(hObsUpdateMutex);

		CloseHandle(hObsUpdateMutex);

		if (i == WAIT_OBJECT_0 + 1)
			return false;
	}

	if (!WaitForOBS())
		return false;

	/* ------------------------------------- *
	 * Init crypt stuff                      */

	CryptProvider hProvider;
	if (!CryptAcquireContext(&hProvider, nullptr, MS_ENH_RSA_AES_PROV,
				 PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		SetDlgItemTextW(hwndMain, IDC_STATUS,
				L"Update failed: CryptAcquireContext failure");
		return false;
	}

	::hProvider = hProvider;

	/* ------------------------------------- */

	SetDlgItemTextW(hwndMain, IDC_STATUS,
			L"Searching for available updates...");

	HWND hProgress = GetDlgItem(hwndMain, IDC_PROGRESS);
	LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
	SetWindowLongPtr(hProgress, GWL_STYLE, style | PBS_MARQUEE);

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETMARQUEE, 1, 0);

	/* ------------------------------------- *
	 * Check if updating portable build      */

	bool bIsPortable = false;
	wstring branch = L"stable";
	wstring appdata;

	if (cmdLine[0]) {
		int argc;
		LPWSTR *argv = CommandLineToArgvW(cmdLine, &argc);

		if (argv) {
			for (int i = 0; i < argc; i++) {
				if (wcscmp(argv[i], L"Portable") == 0) {
					// Legacy OBS
					bIsPortable = true;
					break;
				} else if (wcsncmp(argv[i], L"--branch=", 9) ==
					   0) {
					branch = argv[i] + 9;
				} else if (wcsncmp(argv[i], L"--appdata=",
						   10) == 0) {
					appdata = argv[i] + 10;
				} else if (wcscmp(argv[i], L"--portable") ==
					   0) {
					bIsPortable = true;
				} else if (wcsncmp(argv[i],
						   L"--portable--branch=",
						   19) == 0) {
					/* Versions pre-29.1 beta 2 produce broken parameters :( */
					bIsPortable = true;
					branch = argv[i] + 19;
				}
			}
			LocalFree((HLOCAL)argv);
		}
	}

	/* ------------------------------------- *
	 * Get config path                       */

	wchar_t lpAppDataPath[MAX_PATH];
	lpAppDataPath[0] = 0;

	if (bIsPortable) {
		StringCbCopy(lpAppDataPath, sizeof(lpAppDataPath),
			     obs_base_directory);
		StringCbCat(lpAppDataPath, sizeof(lpAppDataPath), L"\\config");
	} else {
		if (!appdata.empty()) {
			HRESULT hr = StringCbCopy(lpAppDataPath,
						  sizeof(lpAppDataPath),
						  appdata.c_str());
			if (hr != S_OK) {
				Status(L"Update failed: Could not determine AppData "
				       L"location");
				return false;
			}
		} else {
			CoTaskMemPtr<wchar_t> pOut;
			HRESULT hr = SHGetKnownFolderPath(
				FOLDERID_RoamingAppData, KF_FLAG_DEFAULT,
				nullptr, &pOut);
			if (hr != S_OK) {
				Status(L"Update failed: Could not determine AppData "
				       L"location");
				return false;
			}

			StringCbCopy(lpAppDataPath, sizeof(lpAppDataPath),
				     pOut);
		}
	}

	StringCbCat(lpAppDataPath, sizeof(lpAppDataPath), L"\\obs-studio");

	/* ------------------------------------- *
	 * Get download path                     */

	wchar_t manifestPath[MAX_PATH];
	wchar_t tempDirName[MAX_PATH];

	manifestPath[0] = 0;
	tempDirName[0] = 0;

	StringCbPrintf(manifestPath, sizeof(manifestPath),
		       L"%s\\updates\\manifest.json", lpAppDataPath);
	if (!GetTempPathW(_countof(tempDirName), tempDirName)) {
		Status(L"Update failed: Failed to get temp path: %ld",
		       GetLastError());
		return false;
	}
	if (!GetTempFileNameW(tempDirName, L"obs-studio", 0, tempPath)) {
		Status(L"Update failed: Failed to create temp dir name: %ld",
		       GetLastError());
		return false;
	}

	DeleteFile(tempPath);
	CreateDirectory(tempPath, nullptr);

	/* ------------------------------------- *
	 * Load manifest file                    */

	Json root;
	{
		string manifestFile = QuickReadFile(manifestPath);
		if (manifestFile.empty()) {
			Status(L"Update failed: Couldn't load manifest file");
			return false;
		}

		string error;
		root = Json::parse(manifestFile, error);

		if (!error.empty()) {
			Status(L"Update failed: Couldn't parse update "
			       L"manifest: %S",
			       error.c_str());
			return false;
		}
	}

	if (!root.is_object()) {
		Status(L"Update failed: Invalid update manifest");
		return false;
	}

	/* ------------------------------------- *
	 * Hash local files listed in manifest   */

	RunHasherWorkers(4, root["packages"]);

	/* ------------------------------------- *
	 * Parse current manifest update files   */

	const Json::array &packages = root["packages"].array_items();
	for (size_t i = 0; i < packages.size(); i++) {
		if (!AddPackageUpdateFiles(packages, i, tempPath,
					   branch.c_str())) {
			Status(L"Update failed: Failed to process update packages");
			return false;
		}

		/* Add removed files to deletion queue (if any) */
		AddPackageRemovedFiles(packages[i]);
	}

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETMARQUEE, 0, 0);
	SetWindowLongPtr(hProgress, GWL_STYLE, style);

	/* ------------------------------------- *
	 * Exit if updates already installed     */

	if (!updates.size()) {
		Status(L"All available updates are already installed.");
		SetDlgItemText(hwndMain, IDC_BUTTON, L"Launch OBS");
		return true;
	}

	/* ------------------------------------- *
	 * Check for VS2019 redistributables     */

	if (!HasVS2019Redist()) {
		if (!UpdateVS2019Redists(root)) {
			return false;
		}
	}

	/* ------------------------------------- *
	 * Generate file hash json               */

	Json::array files;

	for (update_t &update : updates) {
		wchar_t whash_string[BLAKE2_HASH_STR_LENGTH];
		char hash_string[BLAKE2_HASH_STR_LENGTH];
		char outputPath[MAX_PATH];

		if (!update.has_hash)
			continue;

		/* check hash */
		HashToString(update.my_hash, whash_string);
		if (wcscmp(whash_string, HASH_NULL) == 0)
			continue;

		if (!WideToUTF8Buf(hash_string, whash_string))
			continue;
		if (!WideToUTF8Buf(outputPath, update.basename.c_str()))
			continue;

		string package_path;
		package_path = update.packageName;
		package_path += "/";
		package_path += outputPath;

		files.emplace_back(Json::object{
			{"name", package_path},
			{"hash", hash_string},
		});
	}

	/* ------------------------------------- *
	 * Send file hashes                      */

	string newManifest;

	if (files.size() > 0) {
		string post_body;
		Json(files).dump(post_body);

		int responseCode;

		int len = (int)post_body.size();
		size_t compressSize = ZSTD_compressBound(len);
		string compressedJson;

		compressedJson.resize(compressSize);

		size_t result =
			ZSTD_compress(&compressedJson[0], compressedJson.size(),
				      post_body.data(), post_body.size(),
				      ZSTD_CLEVEL_DEFAULT);

		if (ZSTD_isError(result))
			return false;

		compressedJson.resize(result);

		wstring manifestUrl(PATCH_MANIFEST_URL);
		if (branch != L"stable")
			manifestUrl += L"?branch=" + branch;

		bool success = !!HTTPPostData(manifestUrl.c_str(),
					      (BYTE *)&compressedJson[0],
					      (int)compressedJson.size(),
					      L"Accept-Encoding: gzip",
					      &responseCode, newManifest);

		if (!success)
			return false;

		if (responseCode != 200) {
			Status(L"Update failed: HTTP/%d while trying to "
			       L"download patch manifest",
			       responseCode);
			return false;
		}
	} else {
		newManifest = "[]";
	}

	/* ------------------------------------- *
	 * Parse new manifest                    */

	string error;
	root = Json::parse(newManifest, error);
	if (!error.empty()) {
		Status(L"Update failed: Couldn't parse patch manifest: %S",
		       error.c_str());
		return false;
	}

	if (!root.is_array()) {
		Status(L"Update failed: Invalid patch manifest");
		return false;
	}

	size_t packageCount = root.array_items().size();

	for (size_t i = 0; i < packageCount; i++) {
		const Json &patch = root[i];

		if (!patch.is_object()) {
			Status(L"Update failed: Invalid patch manifest");
			return false;
		}

		const Json &name_json = patch["name"];
		const Json &hash_json = patch["hash"];
		const Json &source_json = patch["source"];
		const Json &size_json = patch["size"];

		if (!name_json.is_string())
			continue;
		if (!hash_json.is_string())
			continue;
		if (!source_json.is_string())
			continue;
		if (!size_json.is_number())
			continue;

		const string &name = name_json.string_value();
		const string &hash = hash_json.string_value();
		const string &source = source_json.string_value();
		int size = size_json.int_value();

		UpdateWithPatchIfAvailable(name.c_str(), hash.c_str(),
					   source.c_str(), size);
	}

	/* ------------------------------------- *
	 * Deduplicate Downloads                 */

	unordered_set<wstring> tempFiles;
	for (update_t &update : updates) {
		if (tempFiles.count(update.tempPath)) {
			update.state = STATE_ALREADY_DOWNLOADED;
			totalFileSize -= update.fileSize;
			completedUpdates++;
			continue;
		}

		tempFiles.insert(update.tempPath);
	}

	/* ------------------------------------- *
	 * Download Updates                      */

	if (!RunDownloadWorkers(4))
		return false;

	if ((size_t)completedUpdates != updates.size()) {
		Status(L"Update failed to download all files.");
		return false;
	}

	/* ------------------------------------- *
	 * Install updates                       */

	int updatesInstalled = 0;
	int lastPosition = 0;

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, 0, 0);

	if (!RunUpdateWorkers(4))
		return false;

	for (deletion_t &deletion : deletions) {
		if (!RenameRemovedFile(deletion)) {
			Status(L"Update failed: Couldn't remove "
			       L"obsolete files");
			return false;
		}
	}

	/* ------------------------------------- *
	 * Install virtual camera                */

	auto runcommand = [](wchar_t *cmd) {
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;
		bool success = !!CreateProcessW(nullptr, cmd, nullptr, nullptr,
						false, CREATE_NEW_CONSOLE,
						nullptr, nullptr, &si, &pi);
		if (success) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	};

	if (!bIsPortable) {
		Status(L"Installing Virtual Camera...");
		wchar_t regsvr[MAX_PATH];
		wchar_t src[MAX_PATH];
		wchar_t tmp[MAX_PATH];
		wchar_t tmp2[MAX_PATH];

		SHGetFolderPathW(nullptr, CSIDL_SYSTEM, nullptr,
				 SHGFP_TYPE_CURRENT, regsvr);
		StringCbCat(regsvr, sizeof(regsvr), L"\\regsvr32.exe");

		StringCbCopy(src, sizeof(src), obs_base_directory);
		StringCbCat(src, sizeof(src),
			    L"\\data\\obs-plugins\\win-dshow\\");

		StringCbCopy(tmp, sizeof(tmp), L"\"");
		StringCbCat(tmp, sizeof(tmp), regsvr);
		StringCbCat(tmp, sizeof(tmp), L"\" /s \"");
		StringCbCat(tmp, sizeof(tmp), src);
		StringCbCat(tmp, sizeof(tmp), L"obs-virtualcam-module");

		StringCbCopy(tmp2, sizeof(tmp2), tmp);
		StringCbCat(tmp2, sizeof(tmp2), L"32.dll\"");
		runcommand(tmp2);

		StringCbCopy(tmp2, sizeof(tmp2), tmp);
		StringCbCat(tmp2, sizeof(tmp2), L"64.dll\"");
		runcommand(tmp2);
	}

	/* ------------------------------------- *
	 * Update hook files and vulkan registry */

	Status(L"Updating Game Capture hooks...");
	UpdateHookFiles();

	/* ------------------------------------- *
	 * Finish                                */

	Status(L"Cleaning up...");
	/* If we get here, all updates installed successfully so we can purge
	 * the old versions */
	for (update_t &update : updates) {
		if (!update.previousFile.empty())
			DeleteFile(update.previousFile.c_str());

		/* We delete here not above in case of duplicate hashes */
		if (!update.tempPath.empty())
			DeleteFile(update.tempPath.c_str());
	}

	/* Delete all removed files mentioned in the manifest */
	for (deletion_t &deletion : deletions)
		MyDeleteFile(deletion.deleteMeFilename);

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, 100, 0);

	Status(L"Update complete.");
	SetDlgItemText(hwndMain, IDC_BUTTON, L"Launch OBS");
	return true;
}

static DWORD WINAPI UpdateThread(void *arg)
{
	wchar_t *cmdLine = (wchar_t *)arg;

	bool success = Update(cmdLine);

	if (!success) {
		/* This handles deleting temp files and rolling back and
		 * partially installed updates */
		CleanupPartialUpdates();

		if (tempPath[0])
			RemoveDirectory(tempPath);

		if (WaitForSingleObject(cancelRequested, 0) == WAIT_OBJECT_0)
			Status(L"Update aborted.");

		HWND hProgress = GetDlgItem(hwndMain, IDC_PROGRESS);
		LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
		SetWindowLongPtr(hProgress, GWL_STYLE, style & ~PBS_MARQUEE);
		SendMessage(hProgress, PBM_SETSTATE, PBST_ERROR, 0);

		SetDlgItemText(hwndMain, IDC_BUTTON, L"Exit");
		EnableWindow(GetDlgItem(hwndMain, IDC_BUTTON), true);

		updateFailed = true;
	} else {
		if (tempPath[0])
			RemoveDirectory(tempPath);
	}

	if (bExiting)
		ExitProcess(success);
	return 0;
}

static void CancelUpdate(bool quit)
{
	if (WaitForSingleObject(updateThread, 0) != WAIT_OBJECT_0) {
		bExiting = quit;
		SetEvent(cancelRequested);
	} else {
		PostQuitMessage(0);
	}
}

static void LaunchOBS(bool portable)
{
	wchar_t newCwd[MAX_PATH];
	wchar_t obsPath[MAX_PATH];

	StringCbCopy(obsPath, sizeof(obsPath), obs_base_directory);
	StringCbCat(obsPath, sizeof(obsPath), L"\\bin\\64bit");
	SetCurrentDirectory(obsPath);
	StringCbCopy(newCwd, sizeof(newCwd), obsPath);

	StringCbCat(obsPath, sizeof(obsPath), L"\\obs64.exe");

	if (!FileExists(obsPath)) {
		/* TODO: give user a message maybe? */
		return;
	}

	SHELLEXECUTEINFO execInfo;

	ZeroMemory(&execInfo, sizeof(execInfo));

	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = obsPath;
	execInfo.lpDirectory = newCwd;
	execInfo.nShow = SW_SHOWNORMAL;

	if (portable)
		execInfo.lpParameters = L"--portable";

	ShellExecuteEx(&execInfo);
}

static INT_PTR CALLBACK UpdateDialogProc(HWND hwnd, UINT message, WPARAM wParam,
					 LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG: {
		static HICON hMainIcon =
			LoadIcon(hinstMain, MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hMainIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hMainIcon);
		return true;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON) {
			if (HIWORD(wParam) == BN_CLICKED) {
				DWORD result =
					WaitForSingleObject(updateThread, 0);
				if (result == WAIT_OBJECT_0) {
					if (updateFailed)
						PostQuitMessage(0);
					else
						PostQuitMessage(1);
				} else {
					EnableWindow((HWND)lParam, false);
					CancelUpdate(false);
				}
			}
		}
		return true;

	case WM_CLOSE:
		CancelUpdate(true);
		return true;
	}

	return false;
}

static int RestartAsAdmin(LPCWSTR lpCmdLine, LPCWSTR cwd)
{
	wchar_t myPath[MAX_PATH];
	if (!GetModuleFileNameW(nullptr, myPath, _countof(myPath) - 1)) {
		return 0;
	}

	/* If the admin is a different user, add the path to the user's
	 * AppData to the command line so we can load the correct manifest. */
	wstring elevatedCmdLine(lpCmdLine);
	CoTaskMemPtr<wchar_t> pOut;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData,
					  KF_FLAG_DEFAULT, nullptr, &pOut);
	if (hr == S_OK) {
		elevatedCmdLine += L" \"--appdata=";
		elevatedCmdLine += pOut;
		elevatedCmdLine += L"\"";
	}

	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = L"runas"; /* Operation to perform */
	shExInfo.lpFile = myPath;   /* Application to start */
	shExInfo.lpParameters =
		elevatedCmdLine.c_str(); /* Additional parameters */
	shExInfo.lpDirectory = cwd;
	shExInfo.nShow = SW_NORMAL;
	shExInfo.hInstApp = 0;

	/* annoyingly the actual elevated updater will disappear behind other
	 * windows :( */
	AllowSetForegroundWindow(ASFW_ANY);

	if (ShellExecuteEx(&shExInfo)) {
		DWORD exitCode;

		WaitForSingleObject(shExInfo.hProcess, INFINITE);

		if (GetExitCodeProcess(shExInfo.hProcess, &exitCode)) {
			if (exitCode == 1) {
				return exitCode;
			}
		}
		CloseHandle(shExInfo.hProcess);
	}
	return 0;
}

static bool HasElevation()
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID sid = nullptr;
	BOOL elevated = false;
	BOOL success;

	success = AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID,
					   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0,
					   0, 0, &sid);
	if (success && sid) {
		CheckTokenMembership(nullptr, sid, &elevated);
		FreeSid(sid);
	}

	return elevated;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int)
{
	INITCOMMONCONTROLSEX icce;

	wchar_t cwd[MAX_PATH];
	GetCurrentDirectoryW(_countof(cwd) - 1, cwd);

	bool isPortable = wcsstr(lpCmdLine, L"Portable") != nullptr ||
			  wcsstr(lpCmdLine, L"--portable") != nullptr;

	if (!IsWindows10OrGreater()) {
		MessageBox(
			nullptr,
			L"OBS Studio 28 and newer no longer support Windows 7,"
			L" Windows 8, or Windows 8.1. You can disable the"
			L" following setting to opt out of future updates:"
			L" Settings → General → General → Automatically check"
			L" for updates on startup",
			L"Unsupported Operating System", MB_ICONWARNING);
		return 0;
	}

	if (!HasElevation()) {

		WinHandle hMutex = OpenMutex(
			SYNCHRONIZE, false, L"OBSUpdaterRunningAsNonAdminUser");
		if (hMutex) {
			MessageBox(
				nullptr,
				L"OBS Studio Updater must be run as an administrator.",
				L"Updater Error", MB_ICONWARNING);
			return 2;
		}

		HANDLE hLowMutex = CreateMutexW(
			nullptr, true, L"OBSUpdaterRunningAsNonAdminUser");

		/* return code 1 =  user wanted to launch OBS */
		if (RestartAsAdmin(lpCmdLine, cwd) == 1) {
			StringCbCat(cwd, sizeof(cwd), L"\\..\\..");
			GetFullPathName(cwd, _countof(obs_base_directory),
					obs_base_directory, nullptr);
			SetCurrentDirectory(obs_base_directory);

			LaunchOBS(isPortable);
		}

		if (hLowMutex) {
			ReleaseMutex(hLowMutex);
			CloseHandle(hLowMutex);
		}

		return 0;
	} else {
		StringCbCat(cwd, sizeof(cwd), L"\\..\\..");
		GetFullPathName(cwd, _countof(obs_base_directory),
				obs_base_directory, nullptr);
		SetCurrentDirectory(obs_base_directory);

		hinstMain = hInstance;

		icce.dwSize = sizeof(icce);
		icce.dwICC = ICC_PROGRESS_CLASS;

		InitCommonControlsEx(&icce);

		hwndMain = CreateDialog(hInstance,
					MAKEINTRESOURCE(IDD_UPDATEDIALOG),
					nullptr, UpdateDialogProc);
		if (!hwndMain) {
			return -1;
		}

		ShowWindow(hwndMain, SW_SHOWNORMAL);
		SetForegroundWindow(hwndMain);

		cancelRequested = CreateEvent(nullptr, true, false, nullptr);
		updateThread = CreateThread(nullptr, 0, UpdateThread, lpCmdLine,
					    0, nullptr);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (!IsDialogMessage(hwndMain, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		/* there is no non-elevated process waiting for us if UAC is
		 * disabled */
		WinHandle hMutex = OpenMutex(
			SYNCHRONIZE, false, L"OBSUpdaterRunningAsNonAdminUser");
		if (msg.wParam == 1 && !hMutex) {
			LaunchOBS(isPortable);
		}

		return (int)msg.wParam;
	}
}
