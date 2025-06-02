#include <tchar.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <Psapi.h>

HANDLE GameLoop = INVALID_HANDLE_VALUE;

struct FVector {
	float X;
	float Y;
	float Z;
};

struct FRotator {
	float Pitch;
	float Yaw;
	float Roll;
};

struct FMatrix {
	float M[4][4];
};

struct Quat {
	float X;
	float Y;
	float Z;
	float W;
};

struct FTransform {
	Quat Rotation;
	FVector Translation;
	FVector Scale3D;
};

struct D3DXMATRIX {
	float _11;
	float _12;
	float _13;
	float _14;
	float _21;
	float _22;
	float _23;
	float _24;
	float _31;
	float _32;
	float _33;
	float _34;
	float _41;
	float _42;
	float _43;
	float _44;
};


class c_driver {
private:
	HANDLE hDriver = INVALID_HANDLE_VALUE;
	struct cmd_struct {
		HANDLE ProcessId;
		PVOID VirtualAddress;
		SIZE_T Size;
	};
public:
	bool Connection(const wchar_t* LinkName) {
		if (LinkName) {
			hDriver = CreateFileW(LinkName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
			if (hDriver == INVALID_HANDLE_VALUE) {
				return false;
			} return true;
		} return false;
	}

	~c_driver() {
		if (hDriver != INVALID_HANDLE_VALUE) CloseHandle(hDriver);
	}

	inline void ReadVM(HANDLE ProcessId, PVOID address, PVOID Buffer, SIZE_T size) {
		cmd_struct cmd = {};
		cmd.ProcessId = ProcessId;
		cmd.VirtualAddress = address;
		cmd.Size = size;
		if (hDriver != INVALID_HANDLE_VALUE) {
			DWORD OutBytes = 0;
			DeviceIoControl(hDriver, 0x801, &cmd, 0x20, Buffer, size, &OutBytes, 0);
		}
	}

	inline bool WriteVM(HANDLE ProcessId, PVOID address, PVOID Buffer, SIZE_T size) {
		cmd_struct cmd = {};
		cmd.ProcessId = ProcessId;
		cmd.VirtualAddress = address;
		cmd.Size = size;
		DWORD OutBytes = 0;
		if (hDriver != INVALID_HANDLE_VALUE) {
			DeviceIoControl(hDriver, 0x802, &cmd, 0x20, Buffer, size, &OutBytes, 0);
		} if (OutBytes == size) {
			return TRUE;
		} return FALSE;
	}
};

c_driver* driver = new c_driver();

int GetAOWHANDLE() {
	int pid = 0;
	int threadCount = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnap, &pe);
	while (Process32Next(hSnap, &pe)) {
		if (_tcsicmp(pe.szExeFile, _T("aow_exe.exe")) == 0) {
			if ((int)pe.cntThreads > threadCount) {
				threadCount = pe.cntThreads;
				pid = pe.th32ProcessID;
			}
		}
	} return pid;
}

UINT64 GetAnogsHeader() {
	return 0x48e00000;
}

UINT64 GetUEHeader() {
	int pid = GetAOWHANDLE();
	UINT64 StartAddress = 0x56e30000;
	UINT64 EndAddress = 0x58e30000;
	for (UINT64 ObjectAddress = StartAddress; ObjectAddress < EndAddress; ObjectAddress += 0x1000) {
		int Data = 0;
		SIZE_T sizeout = 0;
		driver->ReadVM((HANDLE)pid, (PVOID)(ObjectAddress + 0x60), &Data, 4);
		if (Data == 0x09DF2348) {
			return ObjectAddress;
		}
	} return NULL;
}

inline BOOL ReadVM(UINT64 address, PVOID buffer, SIZE_T size) {
	if (address == NULL) {
		return FALSE;
	} driver->ReadVM(GameLoop, (PVOID)address, buffer, size);
	return TRUE;
}

inline UINT64 GetPtr(UINT64 address) {
	UINT64 RET = 0;
	if (address == NULL) {
		return 0;
	} driver->ReadVM(GameLoop, (PVOID)address, &RET, 4);
	return RET;
}

inline float GetFloat(UINT64 address) {
	float RET = 0;
	if (address == NULL) {
		return 0;
	} driver->ReadVM(GameLoop, (PVOID)address, &RET, 4);
	return RET;
}

inline UINT32 GetDWORD(UINT64 address) {
	UINT32 RET = 0;
	if (address == NULL) {
		return 0;
	} driver->ReadVM(GameLoop, (PVOID)address, &RET, 4);
	return RET;
}

inline BOOL GetBool(UINT64 address) {
	BOOL RET = 0;
	if (address == NULL) {
		return 0;
	} driver->ReadVM(GameLoop, (PVOID)address, &RET, 1);
	return RET;
}

//文本
typedef unsigned short UTF16;
typedef char UTF8;

void GetFString(UTF8* buf, UINT64 namepy) {
	UTF16 buf16[16] = {};
	driver->ReadVM(GameLoop, (PVOID)namepy, &buf16, 28);
	UTF16* pTempUTF16 = buf16;
	UTF8* pTempUTF8 = buf;
	UTF8* pUTF8End = pTempUTF8 + 32;
	while (pTempUTF16 < pTempUTF16 + 28) {
		if (*pTempUTF16 <= 0x007F && pTempUTF8 + 1 < pUTF8End) {
			*pTempUTF8++ = (UTF8)*pTempUTF16;
		}
		else if (*pTempUTF16 >= 0x0080 && *pTempUTF16 <= 0x07FF && pTempUTF8 + 2 < pUTF8End) {
			*pTempUTF8++ = (*pTempUTF16 >> 6) | 0xC0;
			*pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
		}
		else if (*pTempUTF16 >= 0x0800 && *pTempUTF16 <= 0xFFFF && pTempUTF8 + 3 < pUTF8End) {
			*pTempUTF8++ = (*pTempUTF16 >> 12) | 0xE0;
			*pTempUTF8++ = ((*pTempUTF16 >> 6) & 0x3F) | 0x80;
			*pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
		}
		else {
			break;
		} pTempUTF16++;
	}
}


//UE4 Math funtions
static float px = 0, py = 0;

inline ImVec2 WorldToScreen(FVector obj, float matrix[16], float ViewW) {
	float x = px + (matrix[0] * obj.X + matrix[4] * obj.Y + matrix[8] * obj.Z + matrix[12]) / ViewW * px;
	float y = py - (matrix[1] * obj.X + matrix[5] * obj.Y + matrix[9] * obj.Z + matrix[13]) / ViewW * py;
	return ImVec2{ x, y };
}

//Bone
inline D3DXMATRIX ToMatrixWithScale(Quat Rotation, FVector Translation, FVector Scale3D) {
	D3DXMATRIX M;
	float X2, Y2, Z2, xX2, Yy2, Zz2, Zy2, Wx2, Xy2, Wz2, Zx2, Wy2;
	M._41 = Translation.X;
	M._42 = Translation.Y;
	M._43 = Translation.Z;
	X2 = Rotation.X + Rotation.X;
	Y2 = Rotation.Y + Rotation.Y;
	Z2 = Rotation.Z + Rotation.Z;
	xX2 = Rotation.X * X2;
	Yy2 = Rotation.Y * Y2;
	Zz2 = Rotation.Z * Z2;
	M._11 = (1 - (Yy2 + Zz2)) * Scale3D.X;
	M._22 = (1 - (xX2 + Zz2)) * Scale3D.Y;
	M._33 = (1 - (xX2 + Yy2)) * Scale3D.Z;
	Zy2 = Rotation.Y * Z2;
	Wx2 = Rotation.W * X2;
	M._32 = (Zy2 - Wx2) * Scale3D.Z;
	M._23 = (Zy2 + Wx2) * Scale3D.Y;
	Xy2 = Rotation.X * Y2;
	Wz2 = Rotation.W * Z2;
	M._21 = (Xy2 - Wz2) * Scale3D.Y;
	M._12 = (Xy2 + Wz2) * Scale3D.X;
	Zx2 = Rotation.X * Z2;
	Wy2 = Rotation.W * Y2;
	M._31 = (Zx2 + Wy2) * Scale3D.Z;
	M._13 = (Zx2 - Wy2) * Scale3D.X;
	M._14 = 0;
	M._24 = 0;
	M._34 = 0;
	M._44 = 1;
	return M;
}

inline FTransform ReadFTransform(uintptr_t address) {
	FTransform Result;
	Result.Rotation.X = GetFloat(address);  // Rotation_X 
	Result.Rotation.Y = GetFloat(address + 4);  // Rotation_y
	Result.Rotation.Z = GetFloat(address + 8);  // Rotation_z
	Result.Rotation.W = GetFloat(address + 12); // Rotation_w
	Result.Translation.X = GetFloat(address + 16);  // /Translation_X
	Result.Translation.Y = GetFloat(address + 20);  // Translation_y
	Result.Translation.Z = GetFloat(address + 24);  // Translation_z
	Result.Scale3D.X = GetFloat(address + 32);; // Scale_X
	Result.Scale3D.Y = GetFloat(address + 36);; // Scale_y
	Result.Scale3D.Z = GetFloat(address + 40);; // Scale_z
	return Result;
}

inline FVector D3dMatrixMultiply(D3DXMATRIX bonematrix, D3DXMATRIX actormatrix) {
	FVector result;
	result.X =
		bonematrix._41 * actormatrix._11 + bonematrix._42 * actormatrix._21 +
		bonematrix._43 * actormatrix._31 + bonematrix._44 * actormatrix._41;
	result.Y =
		bonematrix._41 * actormatrix._12 + bonematrix._42 * actormatrix._22 +
		bonematrix._43 * actormatrix._32 + bonematrix._44 * actormatrix._42;
	result.Z =
		bonematrix._41 * actormatrix._13 + bonematrix._42 * actormatrix._23 +
		bonematrix._43 * actormatrix._33 + bonematrix._44 * actormatrix._43;
	return result;
}

inline FVector getBoneXYZ(uintptr_t humanAddr, uintptr_t boneAddr, int Part) {
	FTransform Bone = ReadFTransform(boneAddr + Part * 48);
	FTransform Actor = ReadFTransform(humanAddr);
	D3DXMATRIX Bone_Matrix = ToMatrixWithScale(Bone.Rotation, Bone.Translation, Bone.Scale3D);
	D3DXMATRIX Component_ToWorld_Matrix = ToMatrixWithScale(Actor.Rotation, Actor.Translation, Actor.Scale3D);
	FVector result = D3dMatrixMultiply(Bone_Matrix, Component_ToWorld_Matrix);
	return result;
}

FRotator ToRotator(FVector local, FVector target) {
	FVector rotation;
	rotation.X = local.X - target.X;
	rotation.Y = local.Y - target.Y;
	rotation.Z = local.Z - target.Z;
	float hyp = sqrt(rotation.X * rotation.X + rotation.Y * rotation.Y);
	FRotator newViewAngle = { 0 };
	newViewAngle.Pitch = -atan(rotation.Z / hyp) * (180.f / (float)3.14159265358979323846);
	newViewAngle.Yaw = atan(rotation.Y / rotation.X) * (180.f / (float)3.14159265358979323846);
	newViewAngle.Roll = (float)0.0f;
	if (rotation.X >= 0.f) {
		newViewAngle.Yaw += 180.0f;
	} return newViewAngle;
}