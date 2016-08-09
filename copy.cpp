#include<iostream>
#include<fstream>
#include<windows.h>
#include<string>
#include<vector>
#include<sys/stat.h>
#include <io.h>
#include<shlwapi.h>
#include <direct.h>
using namespace std;
#include <Shlobj.h>
#include <tchar.h>
#include <Commctrl.h>
#pragma comment(lib, "comctl32.lib")

#define BUFF_SIZE 65536

// wstring转string
string WstringToString(const wstring str)
{
	unsigned len = str.size() * 4;
	setlocale(LC_CTYPE, "");
	char *p = new char[len];
	wcstombs(p,str.c_str(),len);
	std::string str1(p);
	delete[] p;
	return str1;
}
//文件类型：目录或普通文件
enum type{CATALOG, COMMFILE};

string GetName(string &dir)
{
	int size = dir.size();
	int pos = size-1;
	for(int i=size-1; i>=0; --i)
	{
		if(dir[i] == '\\')
		{
			pos = i;
			break;
		}
	}
	return dir.substr(pos+1, size-pos);
}
//拷贝普通文件
void WriterData(const char *src, const char *des)
{
	if(src == NULL || des == NULL) return;
	FILE * fr= fopen(src, "rb");
	FILE * fw= fopen(des, "wb");
	if(fr == NULL || fw == NULL)
	{
		cout<<"file:"<<src<<"open error!"<<endl;
		exit(1);
	}
	char readbuffer[BUFF_SIZE]  = {'\0'};
	int len = 0;
	while((len = fread(readbuffer, sizeof(char), BUFF_SIZE, fr))>0)
	{
		fwrite(readbuffer,sizeof(char), len, fw);
	}
	fclose(fr);
	fclose(fw);
}
//拷贝目录文件
void CopyCata(string &folderPath, string &rootcata)
{
	_finddata_t FileInfo;
	string strfind = folderPath + "\\*";
	long Handle = _findfirst(strfind.c_str(), &FileInfo);

	if (Handle == -1L)
	{
		cerr << "can not match the folder path" << endl;
		exit(1);
	}
	do{
		//判断是否有子目录
		if (FileInfo.attrib & _A_SUBDIR)    
		{
			//这个语句很重要
			if( (strcmp(FileInfo.name,".") != 0 ) &&(strcmp(FileInfo.name,"..") != 0))   
			{
				string newPath = folderPath + "\\" + FileInfo.name;
				string newcata = rootcata + "\\"+ FileInfo.name;
				_mkdir(newcata.c_str());
				CopyCata(newPath, newcata);
			}
		}
		else  if(FileInfo.attrib & _A_ARCH)
		{
			string newfile = rootcata+ "\\" +  FileInfo.name;
			string newsrc = folderPath + "\\" +   FileInfo.name;
			WriterData(newsrc.c_str(), newfile.c_str());
			cout << folderPath << "\\" << FileInfo.name  << " "<<endl;
		}
	}while (_findnext(Handle, &FileInfo) == 0);

	_findclose(Handle);
}
void SendData(string &src, type flag)
{
	//获取磁盘根目录
	DWORD DSLength = GetLogicalDriveStrings(0,NULL);
	LPWSTR lpbuffer = new WCHAR[DSLength];
	DWORD length = GetLogicalDriveStrings(DSLength,  lpbuffer);
	string catalog = "";
	for(DWORD i=0;i<length; i++)
	{
		if(lpbuffer[i] == '\0')
		{
			catalog.push_back(' ');
			continue;
		}
		catalog.push_back(static_cast<char>(lpbuffer[i]));
	}
	int count = 0;

	for(DWORD i=0, j = 0;i<length/4; i++, j += 4)
	{
		UINT DType = GetDriveType(lpbuffer+i*4);
		//可移动磁盘
		if(DType == DRIVE_REMOVABLE)
		{
			count++;
			string rootcata = catalog.substr(j, 3);
			string filename = GetName(src);
			string cata = rootcata+filename;
			//拷贝普通文件
			if(flag == COMMFILE)
			{
				WriterData(src.c_str(), cata.c_str());
			}
			else if(flag == CATALOG)
			{
				_mkdir(cata.c_str());
				CopyCata(src,cata);
			}
		}
	}
	if(count == 0)
	{
		cout<<"请插入U盘！"<<endl;
	}
	delete [] lpbuffer;
	cout<<"over"<<endl;
}
void FileCopy(string &src)
{
	//判断文件是否存在
	if(_access(src.c_str(), 0) != 0)
	{
		cout<<"file no exists"<<endl;
		return;
	}
	//判断文件是目录还是普通文件
	struct _stat fileStat;
	if(_stat(src.c_str(), &fileStat) != 0)
	{
		cout<<"_stat error!"<<endl;
		return ;
	}
	if (fileStat.st_mode & _S_IFDIR)
	{
		SendData(src, CATALOG);
	}
	else if(fileStat.st_mode &S_IFREG)
	{
		SendData(src,COMMFILE);
	}
}
void OneKeyCopy()
{
	char key = 'y';
	do
	{
		LPITEMIDLIST pil = NULL;
		INITCOMMONCONTROLSEX InitCtrls = {0};
		TCHAR szBuf[4096] = {0};
		BROWSEINFO bi = {0};
		bi.hwndOwner = NULL;
		bi.iImage = 0;
		bi.lParam = NULL;
		bi.lpfn = NULL;
		bi.lpszTitle = _T("请选择文件路径");
		bi.pszDisplayName =  szBuf;
		bi.ulFlags = BIF_BROWSEINCLUDEFILES;

		InitCommonControlsEx(&InitCtrls);//在调用函数SHBrowseForFolder之前需要调用该函数初始化相关环境
		pil = SHBrowseForFolder(&bi);
		if (NULL != pil)//若函数执行成功，并且用户选择问件路径并点击确定
		{
			SHGetPathFromIDList(pil, szBuf);//获取用户选择的文件路径
			wstring srcpath = szBuf;
			FileCopy(WstringToString(srcpath));
			wprintf_s(_T("%s"), szBuf);
		}
		cout<<"Please press y or Y continue..."<<endl;
		cin>>key;
	}while(key == 'y' || key == 'Y');
}
int main()
{
	OneKeyCopy();
	return 0;
}
