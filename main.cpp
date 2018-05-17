#include <Windows.h>
#include <stdio.h>
#include <direct.h>
#include <ImageHlp.h>

#define TYPE_DIRECTORY 0
#define TYPE_FILE 1

typedef struct {
	int nFile;
	DWORD size;
} FOLDERINFO;

typedef struct  {
	int type;
	DWORD size;
	char FileName[256];
	int nFolder; // 0 = root folder
	FOLDERINFO Folder;
} PAKFILE;


void uncompress(char*);
void xpakuncompress(char*);
void multiuncompress(char*);
void compress(char*);
FOLDERINFO GetFiles(char*, int = 0);
int FilesCount(char*, int*);

int gc = 0;

PAKFILE *File = NULL;

void main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("\nUnkown Argument\n\nDecompress: dorpack -d myfile.ps\n");
		printf("Compress  : dorpack -c myfolder\\\n");
		return;
	}

	if (strcmp(argv[1], "-d") == 0) uncompress(argv[2]);
	else if (strcmp(argv[1], "-c") == 0) compress(argv[2]);

	else {
		printf("\nUnkown Argument\n\nDecompress: dorpack -d myfile.ps\n");
		printf("Compress  : dorpack -c myfolder\\\n");
	}
}

void uncompress(char *src)
{
	HANDLE hFile = CreateFileA(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Couldn't Open File.\n");
		return;
	}

	DWORD temp;
	int FolderEnd = 0;
	int FileSize = GetFileSize(hFile, NULL);
	char folder[256], folder2[256], sign[11];

	ZeroMemory(sign, 11);
	ReadFile(hFile, sign, 10, &temp, NULL);

	CloseHandle(hFile);

	if (strcmp(sign, "XPAKFILE_1") == 0) {
		xpakuncompress(src);
	}
	else if (strcmp(sign, "MULTI_1") == 0) {
		multiuncompress(src);
	}
	else {
		printf("Unknown File.\n");
	}
}

void xpakuncompress(char *src)
{
	HANDLE hFile = CreateFileA(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Couldn't Open File.\n");
		return;
	}

	DWORD temp;
	int FolderEnd = 0;
	int FileSize = GetFileSize(hFile, NULL);
	char folder[256], folder2[256], sign[11];

	ZeroMemory(sign, 11);
	ReadFile(hFile, sign, 10, &temp, NULL);

	SetFilePointer(hFile, 0x0123, NULL, FILE_BEGIN);
	ReadFile(hFile, folder2, 256, &temp, NULL); // フォルダ名

	strcpy(folder, folder2);
	strcat(folder, "\\");
	strcpy(folder2, folder);

	// 出力用フォルダ作成
	MakeSureDirectoryPathExists(folder);

	int sum = 0;
	sum = SetFilePointer(hFile, 0x045b, NULL, FILE_BEGIN);
	while (sum < FileSize) {
		char FileName[257];
		int foldertype = TYPE_DIRECTORY;
		ReadFile(hFile, &foldertype, 4, &temp, NULL); // 0x0001ならファイル、// 0x0000ならフォルダ
		if (foldertype == TYPE_DIRECTORY) {
			char FolderName[256];
			ReadFile(hFile, FolderName, 256, &temp, NULL);
			sprintf(folder2, "%s%s\\", folder, FolderName);
			
			MakeSureDirectoryPathExists(folder2);
			SetFilePointer(hFile, 12, NULL, FILE_CURRENT);
			ReadFile(hFile, &FolderEnd, 4, &temp, NULL);
			SetFilePointer(hFile, 552, NULL, FILE_CURRENT);
			ReadFile(hFile, &foldertype, 4, &temp, NULL); // 0x0001ならファイル、// 0x0000ならフォルダ
		}
		//FileName
		ReadFile(hFile, FileName, 256, &temp, NULL);
		FileName[256] = '\0';

		// 12byte Skip
		sum = SetFilePointer(hFile, 12, NULL, FILE_CURRENT);

		DWORD FileEndPoint = 0;
		ReadFile(hFile, &FileEndPoint, 4, &temp, NULL);
		sum += 4;
		// ファイルの終わりの位置が0ならファイルの終わりはファイルサイズ
		if (FileEndPoint == 0) {
			FileEndPoint = GetFileSize(hFile, NULL);
			if (FolderEnd != 0) {
				FileEndPoint = FolderEnd;
				FolderEnd = 0;
			}
		}

		char OutFile[512];
		sprintf(OutFile, "%s%s", folder2, FileName);
		HANDLE hOutFile = CreateFileA(OutFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOutFile == INVALID_HANDLE_VALUE) {
			printf("Skip:%s\n", OutFile);
			continue;
		}
		printf("Decompressing:%s\n", OutFile);
		while (SetFilePointer(hFile, 0, NULL, FILE_CURRENT) < FileEndPoint) {
			int ReadBytes = ((FileEndPoint - SetFilePointer(hFile, 0, NULL, FILE_CURRENT) >= 16384) ? 16384 : (FileEndPoint - SetFilePointer(hFile, 0, NULL, FILE_CURRENT)));
			byte data[16384];
			DWORD temp2;
			ReadFile(hFile, data, ReadBytes, &temp, NULL);
			WriteFile(hOutFile, data, ReadBytes, &temp2, NULL);
		}
		sum = FileEndPoint;
		CloseHandle(hOutFile);

	}
	CloseHandle(hFile);
}

void multiuncompress(char *src)
{
	HANDLE hFile = CreateFileA(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Couldn't Open File.\n");
		return;
	}

	DWORD temp;
	int FolderEnd = 0;
	int FileSize = GetFileSize(hFile, NULL);
	char folder[256], folder2[256], sign[11];

	ZeroMemory(sign, 11);
	ReadFile(hFile, sign, 10, &temp, NULL);

	SetFilePointer(hFile, 0x0120, NULL, FILE_BEGIN);
	ReadFile(hFile, folder2, 256, &temp, NULL);
	strcpy(folder, folder2);
	strcat(folder, "\\");
	strcpy(folder2, folder);

	MakeSureDirectoryPathExists(folder);

	int sum = 0;
	int offset = 0;
	DWORD temp3 = 0;
	bool flag2 = false, brk = false;
	int subfolderpos = 0;
	sum = SetFilePointer(hFile, 0x0458, NULL, FILE_BEGIN);

	while (sum < FileSize) {
		char FileName[256];
		int foldertype = TYPE_DIRECTORY;

		ReadFile(hFile, &foldertype, 4, &temp, NULL); // 0x0001ならファイル、// 0x0000ならフォルダ
		bool flag = false;
		while (foldertype == TYPE_DIRECTORY) {
			char FolderName[256];
			ReadFile(hFile, FolderName, 256, &temp, NULL);

			strcat(folder2, FolderName);
			strcat(folder2, "\\");

			MakeSureDirectoryPathExists(folder2);
			SetFilePointer(hFile, 12, NULL, FILE_CURRENT);

			if (!flag && !flag2) ReadFile(hFile, &FolderEnd, 4, &temp, NULL);
			else ReadFile(hFile, &subfolderpos, 4, &temp, NULL);

			if (!flag2) {
				if (FolderEnd != 0) {
					temp3 = FolderEnd;
					flag2 = true;
				}
			}

			SetFilePointer(hFile, 552, NULL, FILE_CURRENT);
			ReadFile(hFile, &foldertype, 4, &temp, NULL); // 0x0001ならファイル、// 0x0000ならフォルダ
			flag = true;
		}
		//FileName
		ReadFile(hFile, FileName, 256, &temp, NULL);

		// 12byte Skip
		int sizes;
		ReadFile(hFile, &sizes, 4, &temp, NULL);
		ReadFile(hFile, &sizes, 4, &temp, NULL);
		sum = SetFilePointer(hFile, 4, NULL, FILE_CURRENT);

		DWORD FileEndPoint = 0;
		ReadFile(hFile, &FileEndPoint, 4, &temp, NULL);
		sum += 4;

		if (sizes + SetFilePointer(hFile, 0, NULL, FILE_CURRENT) >= FileSize) {
			FileEndPoint = GetFileSize(hFile, NULL);
			brk = true;
		}

		char OutFile[256] = ""; 
		sprintf(OutFile, "%s%s", folder2, FileName);
		HANDLE hOutFile = CreateFileA(OutFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOutFile == INVALID_HANDLE_VALUE) {
			printf("Skip:%s\n", OutFile);
			continue;
		}

		printf("Decompressing:%s\n", OutFile);

		int FileSizes = sizes;
		byte *buf = new byte[FileSizes];

		ReadFile(hFile, buf, FileSizes, NULL, NULL);
		WriteFile(hOutFile, buf, FileSizes, NULL, NULL);

		delete[] buf;

		sum = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

		if (subfolderpos == sum) {
			char *p = folder2;
			p += strlen(folder2) - 2;
			while (*p != '\\') p--; p++;
			*p = '\0';
		}

		else if (temp3 == sum && flag2) {

			sprintf(folder2, "%s", folder);
			flag2 = false;
		}

		CloseHandle(hOutFile);
		if (brk) break;

	}
	CloseHandle(hFile);
}

void compress(char *src)
{
	char output[256];
	char folder[256];

	strcpy(folder, src);
	if (folder[strlen(src) - 1] == '\\') {
		folder[strlen(src) - 1] = '\0';
	}
	sprintf(output, "%s.ps", folder);
	

	HANDLE hFile = CreateFileA(output, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	char *sign = "XPAKFILE_1";
	int temp = 0;

	int n = 0;
	FilesCount(src, &n);
	
	File = new PAKFILE[n];

	GetFiles(src);

	WriteFile(hFile, sign, strlen(sign), NULL, NULL);

	SetFilePointer(hFile, 0xc, NULL, FILE_BEGIN);
	temp = 0x2e000000;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x117, NULL, FILE_BEGIN);
	temp = 0x0b;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	temp = 0x011f;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x0123, NULL, FILE_BEGIN); // first folder name;
	WriteFile(hFile, folder, strlen(folder), NULL, NULL);

	SetFilePointer(hFile, 0x22b, NULL, FILE_BEGIN);
	temp = 0x0233;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x237, NULL, FILE_BEGIN);
	temp = 0x2e;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x33c, NULL, FILE_BEGIN);
	temp = 0x33000000;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x340, NULL, FILE_BEGIN);
	temp = 0x47000002;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	temp = 0x03;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	temp = 0x2e000000;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	temp = 0x2e;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	SetFilePointer(hFile, 0x453, NULL, FILE_BEGIN);
	temp = 0x0b;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	temp = 0x045b;
	WriteFile(hFile, &temp, 4, NULL, NULL);

	int offset = 0x45b;
	SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	for (int i = 0; i < gc; i++) {
		if (File[i].type == TYPE_FILE) {
			HANDLE hOut = CreateFileA(File[i].FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hOut == INVALID_HANDLE_VALUE) {
				printf("Skip.\n");
				CloseHandle(hOut);
				continue;
			}

			int size = GetFileSize(hOut, NULL);

			//int sum = offset + 4 + 256 + 12 + 4;

			WriteFile(hFile, &File[i].type, 4, NULL, NULL);

			char *p = File[i].FileName;
			p += strlen(File[i].FileName);
			while (*p != '\\') p--; p++;

			int namelen = strlen(p);
			WriteFile(hFile, p, namelen, NULL, NULL);
			SetFilePointer(hFile, 256 - namelen, NULL, FILE_CURRENT);

			WriteFile(hFile, &size, 4, NULL, NULL);
			WriteFile(hFile, &size, 4, NULL, NULL);
			temp = 0;
			WriteFile(hFile, &temp, 4, NULL, NULL);

			int sum = SetFilePointer(hFile, 0, NULL, FILE_CURRENT) + size + 4;
			if (i + 1 == gc) sum = 0;
			WriteFile(hFile, &sum, 4, NULL, NULL);

			printf("Compressing:%s\n", p);

			byte *buf = new byte[size];

			ReadFile(hOut, buf, size, NULL, NULL);
			WriteFile(hFile, buf, size, NULL, NULL);

			CloseHandle(hOut);
			delete[] buf;

			offset += size;
		}
		// type is TYPE_DIRECTORY
		else {
			WriteFile(hFile, File[i].FileName, 256, NULL, NULL);

			int zero = 0, unknown = 0x2e;
			WriteFile(hFile, &zero, 4, NULL, NULL);
			WriteFile(hFile, &zero, 4, NULL, NULL);

			WriteFile(hFile, &zero, 4, NULL, NULL); // unknown data
			WriteFile(hFile, &zero, 4, NULL, NULL); // unknown data

			WriteFile(hFile, &zero, 4, NULL, NULL);
			WriteFile(hFile, &unknown, 4, NULL, NULL);
		}
	}

	delete[] File;
	CloseHandle(hFile);
}



FOLDERINFO GetFiles(char* input, int nFolder)
{
	HANDLE hFind;
	WIN32_FIND_DATAA wd;
	int size = 0;
	FOLDERINFO folder = {0, 0};
	char temp[256], search[256];
	strcpy(temp, input);

	if (temp[strlen(temp) - 1] != '\\') {
		strcat(temp, "\\");
	}

	strcpy(search, temp);
	strcat(search, "*.*");

	hFind = FindFirstFileA(search, &wd);
	if (hFind == INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		return FOLDERINFO{ 0, 0 };
	}

	do {
		// フォルダ
		if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(wd.cFileName, "..") != 0 && strcmp(wd.cFileName, ".") != 0) {
			File[gc].type = TYPE_DIRECTORY;
			File[gc].nFolder = nFolder;

			sprintf(File[gc].FileName, "%s", wd.cFileName);

			char sub[256];
			sprintf(sub, "%s%s\\", temp, wd.cFileName);

			FOLDERINFO folder2 = GetFiles(sub, nFolder + 1);
			File[gc].size = folder2.size;
			File[gc].Folder.nFile = folder2.nFile;

			gc++;
		}
		else { // ファイル
			if (strcmp(wd.cFileName, "..") != 0 && strcmp(wd.cFileName, ".") != 0) {

				File[gc].type =TYPE_FILE;
				File[gc].size = wd.nFileSizeLow;
				File[gc].nFolder = nFolder;
				File[gc].Folder.nFile = 0;
				sprintf(File[gc].FileName, "%s%s", temp, wd.cFileName);
				folder.nFile++;
				folder.size += wd.nFileSizeHigh * MAXDWORD + wd.nFileSizeLow;

				gc++;
			}
		}
	} while (FindNextFileA(hFind, &wd));

	FindClose(hFind);
	return folder;
}

int FilesCount(char* input, int *n)
{
	HANDLE hFind;
	WIN32_FIND_DATAA wd;
	char temp[256], search[256];
	strcpy(temp, input);

	if (temp[strlen(temp) - 1] != '\\') {
		strcat(temp, "\\");
	}

	strcpy(search, temp);
	strcat(search, "*.*");

	hFind = FindFirstFileA(search, &wd);
	if (hFind == INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		return -1;
	}

	do {
		if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(wd.cFileName, "..") != 0 && strcmp(wd.cFileName, ".") != 0) {
			*n += 1;

			char sub[256];
			sprintf(sub, "%s%s\\", temp, wd.cFileName);
			FilesCount(sub, n);
		}

		else {
			if (strcmp(wd.cFileName, "..") != 0 && strcmp(wd.cFileName, ".") != 0) {
				*n += 1;
			}
		}
	} while (FindNextFileA(hFind, &wd));

	FindClose(hFind);
	return 0;
}