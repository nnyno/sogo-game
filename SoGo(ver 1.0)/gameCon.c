#define _CRT_SECURE_NO_WARNINGS

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 45
#define HERO_MISSILE_COUNT 150
#define ENEMY_MISSILE_COUNT 100
#define BOSS_MISSILE_COUNT 30
#define ENEMY_TYPE_COUNT 4
#define ENEMY_PAT_COUNT 4
#define EARTH_LIFE 20

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <Windows.h>
#include <conio.h>
#include <fmod.h>
#include "screen.h"

// 구조체, 열거형 ////////////////////////////////////////////////////////
typedef enum _MENU {
	START, HELP, EXIT
} MENU;
typedef enum _DIRECT {
	UP, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT
} DIRECT;
typedef enum _SCRIPT {
	NONE, PLAYER_TALK, BOSS_TALK
} SCRIPT;

typedef struct _MISSILE
{
	int nLife;
	int nX, nY;

	clock_t MoveTime;
	clock_t OldMoveTime;
} MISSILE;
typedef struct _PLAYER
{
	int nLife; // (최대 20)
	int nX, nY;
	int nCannon; // 캐논 (최대 10)
	int nShield; // 베리어 (최대 10)
	int nSpeed; // 이속 조절 (최대 5)
	int nHaste; // 공속 조절 (최대 5)
	int nMissileCount; // 미사일 발사 수 (최대 3)

	clock_t HorizonMoveTime;
	clock_t HorizonOldMoveTime;
	clock_t VerticalMoveTime;
	clock_t VerticalOldMoveTime;

	clock_t FireTime;
	clock_t OldFireTime;
	clock_t FireMoveTime;

	clock_t HitTime;
	clock_t OldHitTime;
	int EnableDamage;

	clock_t SkillTime;
	clock_t OldSkillTime;
	clock_t ShieldKeepTime;
	clock_t OldShieldKeepTime;
	MISSILE CannonMissile[16];
	MISSILE Shield[7];
} PLAYER;

typedef struct _ENEMY
{
	int nLife;
	int nX, nY;
	int nType;

	clock_t AppearTime;

	clock_t MoveTime;
	clock_t OldMoveTime;

	clock_t FireTime;
	clock_t OldFireTime;
	clock_t FireMoveTime;

	int nPatType;
	int nPatIndex;
	int nPatStep;
} ENEMY;
typedef struct _ENEMY_MISSILE
{
	int nLife;
	int nX, nY;

	clock_t MoveTime;
	clock_t OldMoveTime;
} ENEMY_MISSILE;
typedef struct _ENEMY_INFO
{
	int nLife;
	int nType;

	clock_t FireTime;
	clock_t FireMoveTime;
} ENEMY_INFO;

typedef struct _BOSS_MISSILE
{
	DIRECT nDirect; // 지정된 방향으로 미사일 발사
	double nVectorX, nVectorY; // 벡터를 통해 미사일 발사
	int nType; // 미사일 모양

	int nLife;
	int nX, nY;
	double nXf, nYf; // 벡터를 통해 발사되는 미사일 전용 좌표

	clock_t MoveTime;
	clock_t OldMoveTime;
} BOSS_MISSILE;
typedef struct _BOSS
{
	int nLife;
	int nX, nY;
	int nSaveX, nSaveY;

	clock_t MoveTime;
	clock_t OldMoveTime;

	clock_t FireTime;
	clock_t OldFireTime;

	// 스킬 쿨타임
	clock_t SkillCoolTime;
	clock_t OldSkillCoolTime;

	// 포 스킬
	int EnableElectronGun;
	int WarningElectronGun;
	BOSS_MISSILE ElectronGun[50];
	clock_t WarningTime;
	clock_t OldWarningTime;
	clock_t EndElectronGunTime;
	clock_t OldEndElectronGunTime;

	// 조준탄 스킬
	int nShape;
	int EnableAimingShot;
	int SmallBossChange;
	BOSS_MISSILE AimingShot[50];
	clock_t SmallChangeTime;
	clock_t OldSmallChangeTime;
	clock_t AimingShotTime;
	clock_t OldAimingShotTime;
	clock_t EndAimingShotTime;
	clock_t OldEndAimingShotTime;

	// 유도탄 스킬
	int EnableGuidedShot;
	double nVectorMaxPower;
	BOSS_MISSILE GuidedShot[2];
	clock_t GuidedShotTime;
	clock_t OldGuidedShotTime;
	clock_t EndGuidedShotTime;
	clock_t OldEndGuidedShotTime;

	// 운석 낙하 스킬 (지구 파괴)
	int EnableMeteorFall;
	int MeteorCount;
	BOSS_MISSILE Meteor[10];
	clock_t MeteorShotTime;
	clock_t OldMeteorShotTime;

	int nPatType;
	int nPatIndex;
	int nPatStep;
} BOSS;

typedef struct _ITEM
{
	int nLife;
	int nX, nY;
	int nType;

	clock_t MoveTime;
	clock_t OldMoveTime;
} ITEM;

typedef struct _EARTH
{
	int nLife;
	int surface[37];
} EARTH;

typedef struct _PAT
{
	DIRECT nDirect;
	int nStep;
	clock_t MoveTime;
	int nDist;
} PAT;
typedef struct _PAT_INFO
{
	int nCount;
	int nX0, nY0;
	PAT* pPat;
} PAT_INFO;

typedef struct _SOUND
{
	char* Path;
	FMOD_SOUND* Sound;
	FMOD_CHANNEL* Channel;
	FMOD_MODE mode;
	int OverlapPermit;
	int ChannelIndex;
} SOUND;

// 전역 변수 ////////////////////////////////////////////////////////
// 파일 불러오기
FILE* fp;

// 사운드 변수
FMOD_SYSTEM* g_System;

SOUND menu_sound, menu_select_sound, exit_sound,
	player_shot_sound, player_cannon_sound, player_shield_sound , player_hit_sound, item_get_sound,
	boss_guideshot_sound, boss_ElectronGun_sound, boss_AimingShot_sound, boss_meteorfall_sound,
	collision_sound, earth_collision_sound, success_sound, fail_sound;
FMOD_CHANNEL* channel[32];
FMOD_BOOL IsPlaying;

// 시스템 변수
int oneRender = 1; // 랜더를 한번만 해야할 경우 사용

// 메뉴 변수
MENU selectMenu = START;

// 게임 변수
// 챕터, 스테이지, 상태, 점수, 키 제어
int chapter = 0;
int stage = 0;
int gameMenu = 1; // 게임 메뉴 상태 1, 게임 시작 0, 게임 설명 2
int gameState; // 게임 진행 중 1, 게임 오버 0, 게임 클리어 2
int gameBossScript = 0; // 보스전 시작전 대사 1, 보스전 시작 0
int score = 0;
int gameKeyControl = 1;

// 게임 기록
clock_t startTime;
clock_t endTime;
int second, minute;
char timeAttact[25] ="";
char finalScore[25] = "";

// 적기 정보
ENEMY_INFO g_EnemyInfo[ENEMY_TYPE_COUNT];
char g_EnemyType[ENEMY_TYPE_COUNT][3] = {"♨", "◆", "☎", "▣"};
clock_t g_StartTime;
int g_nEnemyIndex;
int g_nEnemyCount;
int EnemyChange = 1;
int nEnemyFileCount;

// 보스 정보
int nBossFileCount;
char g_BossMissileType[8][3] = {"↑", "↗", "→", "↘", "↓", "↙", "←", "↖"};
clock_t g_UpdateOldTime;
SCRIPT CurScript;

// 비행기
PLAYER g_Player;
ENEMY* g_Enemy;
BOSS g_Boss;

// 지구
EARTH g_Earth;

// 아이템(스킬 및 체력)
ITEM g_Item[20];
char g_ItemType[6][3] = {"♥", "◎", "★", "▲", "▲", "^"};

//  미사일 버퍼
MISSILE g_PlayerMissile[HERO_MISSILE_COUNT];
ENEMY_MISSILE g_EnemyMissile[ENEMY_MISSILE_COUNT];
BOSS_MISSILE g_BossMissile[BOSS_MISSILE_COUNT];

// 패턴 정보
PAT_INFO* g_EnemyPatInfo;
PAT_INFO* g_BossPatInfo;

// 외부 패턴 파일 이름
char* g_strFileNameEnemy[ENEMY_PAT_COUNT] = { "Pattern\\pat1.txt", "Pattern\\pat2.txt", "Pattern\\pat3.txt", "Pattern\\pat4.txt" };
char* g_strFileNameBoss[2] = { "Pattern\\boss_pat1.txt", "Pattern\\boss_pat2.txt" };

// 부가적인 함수 ////////////////////////////////////////////////////////
// 키보드 버퍼 삭제 함수
void KeyBufferClear()
{
	while (1)
	{
		if (!((GetAsyncKeyState(VK_LEFT) & 0x8001) || (GetAsyncKeyState(VK_LEFT) & 0x8000)))
			
		if (!((GetAsyncKeyState(VK_RIGHT) & 0x8001) || (GetAsyncKeyState(VK_RIGHT) & 0x8000)))
			
		if (!((GetAsyncKeyState(VK_UP) & 0x8001) || (GetAsyncKeyState(VK_UP) & 0x8000)))
			
		if (!((GetAsyncKeyState(VK_DOWN) & 0x8001) || (GetAsyncKeyState(VK_DOWN) & 0x8000)))
			
		if (!((GetAsyncKeyState(VK_SPACE) & 0x8001) || (GetAsyncKeyState(VK_SPACE) & 0x8000)))

		if (!((GetAsyncKeyState('z') & 0x8001) || (GetAsyncKeyState('z') & 0x8000)))

		if (!((GetAsyncKeyState('Z') & 0x8001) || (GetAsyncKeyState('Z') & 0x8000)))

		if (!((GetAsyncKeyState('x') & 0x8001) || (GetAsyncKeyState('x') & 0x8000)))

		if (!((GetAsyncKeyState('X') & 0x8001) || (GetAsyncKeyState('X') & 0x8000)))
			break;  
	}
}
// 충돌 유무 함수
int Collision(int nX1, int nY1, int nX2, int nY2, int nX3, int nY3, int nX4, int nY4)
{
	if (nX1 < nX4 && nX2 > nX3 && nY1 <= nY4 && nY2 >= nY3)
		return 1;
	else
		return 0;
}
// 피타고라스 정리
double Pythagoras(int nX1, int nY1, int nX2, int nY2)
{
	return sqrt(pow((double)abs(nX1 - nX2), 2.0) + pow((double)abs(nY1 - nY2), 2.0));
}

// 소리 초기화 함수 ////////////////////////////////////////////////////////
int isSoundPlay(SOUND* sound)
{
	if ((*sound).ChannelIndex == -1)
	{
		return 0;
	}
	else
	{
		FMOD_Channel_IsPlaying((*sound).Channel, &IsPlaying);
		if (IsPlaying == 1)
		{
			IsPlaying = 0;
			return 1;
		}
		else
		{
			(*sound).ChannelIndex = -1;
			return 0;
		}
	}
}
void SoundInit()
{
	// 초기화
	FMOD_System_Create(&g_System);
	FMOD_System_Init(g_System, 32, FMOD_INIT_NORMAL, NULL);

	// 소리 객체 생성
	menu_sound.Path				= "Sound\\menu.wav";				menu_sound.ChannelIndex = -1;				menu_sound.OverlapPermit = 0;				menu_sound.mode = FMOD_LOOP_NORMAL;
	menu_select_sound.Path		= "Sound\\menu_select.wav";			menu_select_sound.ChannelIndex = -1;		menu_select_sound.OverlapPermit = 1;		menu_select_sound.mode = FMOD_DEFAULT;
	player_shot_sound.Path		= "Sound\\player_shot.wav";			player_shot_sound.ChannelIndex = -1;		player_shot_sound.OverlapPermit = 1;		player_shot_sound.mode = FMOD_DEFAULT;
	player_cannon_sound.Path	= "Sound\\player_cannon.wav";		player_cannon_sound.ChannelIndex = -1;		player_cannon_sound.OverlapPermit = 1;		player_cannon_sound.mode = FMOD_DEFAULT;
	player_shield_sound.Path	= "Sound\\player_shield.wav";		player_shield_sound.ChannelIndex = -1;		player_shield_sound.OverlapPermit = 1;		player_shield_sound.mode = FMOD_DEFAULT;
	player_hit_sound.Path		= "Sound\\player_hit.wav";			player_hit_sound.ChannelIndex = -1;			player_hit_sound.OverlapPermit = 1;			player_hit_sound.mode = FMOD_DEFAULT;
	item_get_sound.Path			= "Sound\\item_get.wav";			item_get_sound.ChannelIndex = -1;			item_get_sound.OverlapPermit = 0;			item_get_sound.mode = FMOD_DEFAULT;

	boss_guideshot_sound.Path	= "Sound\\boss_guideshot.wav";		boss_guideshot_sound.ChannelIndex = -1;		boss_guideshot_sound.OverlapPermit = 1;		boss_guideshot_sound.mode = FMOD_DEFAULT;
	boss_ElectronGun_sound.Path = "Sound\\boss_ElectronGun.wav";	boss_ElectronGun_sound.ChannelIndex = -1;	boss_ElectronGun_sound.OverlapPermit = 1;	boss_ElectronGun_sound.mode = FMOD_DEFAULT;
	boss_AimingShot_sound.Path	= "Sound\\boss_AimingShot.wav";		boss_AimingShot_sound.ChannelIndex = -1;	boss_AimingShot_sound.OverlapPermit = 1;	boss_AimingShot_sound.mode = FMOD_DEFAULT;
	boss_meteorfall_sound.Path	= "Sound\\boss_meteorfall.wav";		boss_meteorfall_sound.ChannelIndex = -1;	boss_meteorfall_sound.OverlapPermit = 1;	boss_meteorfall_sound.mode = FMOD_DEFAULT;

	collision_sound.Path		= "Sound\\collision.wav";			collision_sound.ChannelIndex = -1;			collision_sound.OverlapPermit = 1;			collision_sound.mode = FMOD_DEFAULT;
	earth_collision_sound.Path	= "Sound\\earth_collision.wav";		earth_collision_sound.ChannelIndex = -1;	earth_collision_sound.OverlapPermit = 1;	earth_collision_sound.mode = FMOD_DEFAULT;

	success_sound.Path			= "Sound\\success.wav";				success_sound.ChannelIndex = -1;			success_sound.OverlapPermit = 0;			success_sound.mode = FMOD_LOOP_NORMAL;
	fail_sound.Path				= "Sound\\fail.wav";				fail_sound.ChannelIndex = -1;				fail_sound.OverlapPermit = 0;				menu_sound.mode = FMOD_LOOP_NORMAL;

	exit_sound.Path				= "Sound\\end.wav";					exit_sound.ChannelIndex = -1;				exit_sound.OverlapPermit = 0;				menu_sound.mode = FMOD_DEFAULT;

}
void SoundPlay(SOUND* sound, float volume)
{
	if ((*sound).ChannelIndex != -1)
	{
		if (isSoundPlay(sound) == 0)
		{
			FMOD_Sound_Release((*sound).Sound);
			(*sound).ChannelIndex = -1;
		}
	}

	if ((*sound).ChannelIndex == -1)
	{
		FMOD_System_CreateSound(g_System, (*sound).Path, (*sound).mode, 0, &((*sound).Sound));
		FMOD_System_PlaySound(g_System, FMOD_CHANNEL_FREE, (*sound).Sound, 0, &((*sound).Channel));
		FMOD_Channel_SetVolume((*sound).Channel, volume);
		(*sound).ChannelIndex = 1;
	}
	else if ((*sound).OverlapPermit)
	{
		FMOD_System_PlaySound(g_System, FMOD_CHANNEL_FREE, (*sound).Sound, 0, &((*sound).Channel));
		FMOD_Channel_SetVolume((*sound).Channel, volume);
	}
}
void SoundStop(SOUND* sound)
{
	FMOD_Channel_Stop(channel[(*sound).ChannelIndex]);
	FMOD_Sound_Release((*sound).Sound);
	(*sound).ChannelIndex = -1;
}
void AllSoundStop()
{
	for (int i = 0; i < 5; i++)
	{
		FMOD_Channel_Stop(channel[i]);
		channel[i] = NULL;
	}
}
void SoundRelease()
{
	FMOD_System_Close(g_System);
	FMOD_System_Release(g_System);
}

// Script 함수 ////////////////////////////////////////////////////////
void PlayerScript(int xPlayer, int yPlayer)
{
	ScreenPrint(xPlayer, yPlayer, "┏━┓ ━━━ ┏━┓");
	ScreenPrint(xPlayer, yPlayer + 1, "┗━┛ ┃ ┃ ┗━┛");
	ScreenPrint(xPlayer + 2, yPlayer + 2, "┃  ▽ ┃");
	ScreenPrint(xPlayer + 3, yPlayer + 3, "━━━━━");
}
void BossScript(int xBoss, int yBoss)
{
	ScreenPrint(xBoss + 1, yBoss, "/| ━  // ━  |＼");
	ScreenPrint(xBoss, yBoss + 1, "//q ●    ● p＼");
	ScreenPrint(xBoss + 2, yBoss + 2, "(┗━┳━━━┳━┛ )");
	ScreenPrint(xBoss + 5, yBoss + 3, "┃ ㅣ┃");
	ScreenPrint(xBoss + 5, yBoss + 4, "┗━━━┛");
}
void BossDieScript(int xBoss, int yBoss)
{
	ScreenPrint(xBoss + 1, yBoss, "/| ￣ // ￣ |＼");
	ScreenPrint(xBoss, yBoss + 1, "//q  X    X  p＼");
	ScreenPrint(xBoss + 2, yBoss + 2, "(┗━┳━━━┳━┛ )");
	ScreenPrint(xBoss + 5, yBoss + 3, "┃ ㅣ┃");
	ScreenPrint(xBoss + 5, yBoss + 4, "┗━━━┛");
}
void Script(int x, int y, char *name, char* script)
{
	int i = 0;
	char script2[300];
	char* ptr;

	strcpy(script2, script);

	ScreenPrint(x, y, name);
	ScreenPrint(x+6, y, ":");

	ptr = strtok(script2, "\n");

	while (ptr != NULL)
	{
		ScreenPrint(x + 8, y + (i*2), ptr);          
		ptr = strtok(NULL, "\n");
		i++;
	}
	
}

void MainScript(SCRIPT value)
{
	switch (chapter)
	{
	case 1:
		switch (value) 
		{
		case PLAYER_TALK:
			PlayerScript(15, 23);
			BossScript(50, 13);
			Script(15, 28, "주인공", "너네들은 누구냐!!");
			break;
		case BOSS_TALK:
			PlayerScript(15, 13);
			BossScript(50, 23);
			Script(15, 28, "악당", "우리는 지구를 침략하러 왔다!!");
			break;
		}
		break;
	case 2:
		switch (value)
		{
		case PLAYER_TALK:
			PlayerScript(15, 23);
			BossScript(50, 13);
			Script(15, 28, "주인공", "어째서 이런 일을 벌이는거야?");
			break;
		case BOSS_TALK:
			PlayerScript(15, 13);
			BossScript(50, 23);
			Script(15, 28, "악당", "이렇게 넓은 우주에 우리뿐이라면\n엄청난 공간 낭비가 아닐까?");
			break;
		}
		break;
	case 3:
		switch (value)
		{
		case PLAYER_TALK:
			PlayerScript(15, 23);
			BossScript(50, 13);
			Script(15, 28, "주인공", "우린 답을 찾을 거야, 늘 그랬듯이");
			break;
		case BOSS_TALK:
			PlayerScript(15, 13);
			BossScript(50, 23);
			Script(15, 28, "악당", "하하하 누가보면 벌써 다 끝난 줄 알겠어?");
			break;
		}
		break;
	case 4:
		switch (value)
		{
		case PLAYER_TALK:
			PlayerScript(15, 23);
			BossScript(50, 13);
			Script(15, 28, "주인공", "어벤져스 도와줘요..!!");
			break;
		case BOSS_TALK:
			PlayerScript(15, 13);
			BossScript(50, 23);
			Script(15, 28, "악당", "영화에 나오는 애들을 왜 찾는거야? 하하하!");
			break;
		}
		break;
	case 5:
		switch (value)
		{
		case PLAYER_TALK:
			PlayerScript(15, 23);
			Script(15, 28, "주인공", "후...해치웠나?");
			break;
		case BOSS_TALK:
			PlayerScript(15, 13);
			BossScript(50, 23);
			Script(15, 28, "악당", "하하 내 부하들이 힘을 충분히 빼주었구나\n마지막 공격이다!!");
			break;
		}
		break;
	}
}

// Init : 게임 변수 초기화 함수 ////////////////////////////////////////////////////////
void PlayerInit()
{
	// 플레이어/ 플레이어 스킬 설정값 초기화
	g_Player.nLife = 5;
	g_Player.nX = 40;
	g_Player.nY = 40;
	g_Player.nCannon = 2;
	g_Player.nShield = 2;
	g_Player.nSpeed = 0;
	g_Player.nHaste = 0;
	g_Player.nMissileCount = 1;
	g_Player.EnableDamage = 1;

	// 이동 컨트롤 초기화
	g_Player.HorizonMoveTime = 50;
	g_Player.HorizonOldMoveTime = clock();
	g_Player.VerticalMoveTime = 50;
	g_Player.VerticalOldMoveTime = clock();

	// 미사일, 스킬 컨트롤 초기화
	g_Player.FireTime = 100;
	g_Player.OldFireTime = clock();
	g_Player.FireMoveTime = 50;
	g_Player.SkillTime = 3000;
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		g_PlayerMissile[i].nLife = 0;
	}
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		g_Player.CannonMissile[i].nLife = 0;
	}

	// 실드 컨트롤 초기화
	g_Player.ShieldKeepTime = 3000;

	// 피격 컨트롤 초기화
	g_Player.HitTime = 2000;
}
void EnemyInit()
{
	// 적기,적기 미사일 컨트롤 초기화
	for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
	{
		g_EnemyInfo[i].nLife = 0;
		g_EnemyInfo[i].nType = i;
		g_EnemyInfo[i].FireTime = 2800;
		g_EnemyInfo[i].FireMoveTime = 50;
	}
	for (int i = 0; i < ENEMY_MISSILE_COUNT; i++)
	{
		g_EnemyMissile[i].nLife = 0;
	}

	// 적기 패턴 컨트롤 초기화
	nEnemyFileCount = sizeof(g_strFileNameEnemy) / sizeof(char*);
	g_EnemyPatInfo = (PAT_INFO*)malloc(sizeof(PAT_INFO) * nEnemyFileCount);
	for (int i = 0; i < nEnemyFileCount; i++)
	{
		fp = fopen(g_strFileNameEnemy[i], "r");

		fscanf(fp, "%d\n", &g_EnemyPatInfo[i].nCount);
		g_EnemyPatInfo[i].pPat = (PAT*)malloc(sizeof(PAT) * g_EnemyPatInfo[i].nCount);

		for (int j = 0; j < g_EnemyPatInfo[i].nCount; j++)
		{
			fscanf(fp, "%d %d %d %d\n", &g_EnemyPatInfo[i].pPat[j].nDirect, &g_EnemyPatInfo[i].pPat[j].nStep, &g_EnemyPatInfo[i].pPat[j].MoveTime, &g_EnemyPatInfo[i].pPat[j].nDist);
		}
		fclose(fp);
	}
}
void BossInit()
{
	// 보스 대사 컨트롤
	CurScript = NONE;
	g_UpdateOldTime = clock();

	// 보스 변수값 초기화
	g_Boss.nLife = 0;

	// 보스 스킬 초기화
	g_Boss.SkillCoolTime = 3000;
	g_Boss.WarningTime = 3000;
	g_Boss.EndElectronGunTime = 5000;
	g_Boss.SmallChangeTime = 900;
	g_Boss.AimingShotTime = 190;
	g_Boss.EndAimingShotTime = 10000;
	g_Boss.nVectorMaxPower = 1.7;
	g_Boss.GuidedShotTime = 2000;
	g_Boss.EndGuidedShotTime = 10000;
	g_Boss.MeteorShotTime = 1000;

	// 보스,보스 미사일 컨트롤 초기화
	for (int i = 0; i < BOSS_MISSILE_COUNT; i++)
	{
		g_BossMissile[i].nLife = 0;
	}
	for (int i = 0; i < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); i++)
	{
		g_Boss.ElectronGun[i].nLife = 0;
	}
	for (int i = 0; i < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); i++)
	{
		g_Boss.AimingShot[i].nLife = 0;
	}
	for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
	{
		g_Boss.GuidedShot[i].nLife = 0;
	}
	for (int i = 0; i < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); i++)
	{
		g_Boss.Meteor[i].nLife = 0;
	}

	// 보스 패턴 컨트롤 초기화
	nBossFileCount = sizeof(g_strFileNameBoss) / sizeof(char*);
	g_BossPatInfo = (PAT_INFO*)malloc(sizeof(PAT_INFO) * nBossFileCount);
	for (int i = 0; i < nBossFileCount; i++)
	{
		fp = fopen(g_strFileNameBoss[i], "r");

		fscanf(fp, "%d\n", &g_BossPatInfo[i].nCount);
		fscanf(fp, "%d %d\n", &g_BossPatInfo[i].nX0, &g_BossPatInfo[i].nY0);
		g_BossPatInfo[i].pPat = (PAT*)malloc(sizeof(PAT) * g_BossPatInfo[i].nCount);

		for (int j = 0; j < g_BossPatInfo[i].nCount; j++)
		{
			fscanf(fp, "%d %d %d %d\n", &g_BossPatInfo[i].pPat[j].nDirect, &g_BossPatInfo[i].pPat[j].nStep, &g_BossPatInfo[i].pPat[j].MoveTime, &g_BossPatInfo[i].pPat[j].nDist);
		}
		fclose(fp);
	}
}
void EarthInit()
{
	// 지구 컨트롤 초기화
	g_Earth.nLife = EARTH_LIFE;
	for (int i = 0; i < sizeof(g_Earth.surface) / sizeof(int); i++)
	{
		if ((i >= 3 && i <= 10) || (i >= 20 && i <= 30))
			g_Earth.surface[i] = 1;
		else
			g_Earth.surface[i] = 0;
	}
}
void ItemInit()
{
	for (int i = 0; i < sizeof(g_Item) / sizeof(ITEM); i++)
	{
		g_Item[i].nLife = 0;
		g_Item[i].MoveTime = 1000; // 아이템 떨어지는 속도
	}
}

void Init()
{
	// 게임 설정값 초기화
	chapter = 1;
	stage = 0;
	score = 0;
	EnemyChange = 1;

	srand((unsigned int)time(NULL)); // 랜덤 설정 초기화

	PlayerInit();
	EnemyInit();
	BossInit();
	EarthInit();
	ItemInit();
}

// PlayerKey : 키보드 함수 ///////////////////////////////////////////
void MenuKey()
{
	int nKey;

	if (GetAsyncKeyState(VK_SPACE) & 0x8000) nKey = 32;
	else if (GetAsyncKeyState(VK_UP) & 0x8000) nKey = 72;
	else if (GetAsyncKeyState(VK_DOWN) & 0x8000) nKey = 80;
	else nKey = -1;

	if (nKey >= 0)
	{
		switch (nKey)
		{
		case 72:
			SoundPlay(&menu_select_sound, 1.0f);
			if (selectMenu == EXIT)
				selectMenu = HELP;
			else if (selectMenu == HELP)
				selectMenu = START;
			KeyBufferClear();
			break;
		case 80:
			SoundPlay(&menu_select_sound, 1.0f);
			if (selectMenu == START)
				selectMenu = HELP;
			else if (selectMenu == HELP)
				selectMenu = EXIT;
			KeyBufferClear();
			break;
		case 32:
			SoundPlay(&menu_select_sound, 1.0f);
			if (selectMenu == START)
			{
				gameMenu = 0;
				gameState = 1;
				KeyBufferClear();
			}
			else if (selectMenu == HELP)
			{
				gameMenu = 2;
				KeyBufferClear();
			}
			else if (selectMenu == EXIT)
				gameMenu = -1;
			break;
		}
	}
}
void HelpKey()
{
	int nKey;

	if (GetAsyncKeyState(VK_SPACE) & 0x8000) nKey = 32;
	else nKey = -1;

	if (nKey >= 0)
	{
		switch (nKey)
		{
		case 32:
			SoundPlay(&menu_select_sound, 1.0f);
			selectMenu = START;
			gameMenu = 1;

			break;
		}
		KeyBufferClear();
	}
}
void GameKey()
{
	int nKey;
	int nCount; // 미사일 조절
	clock_t CurTime = clock();

	if (!gameBossScript) //  보스 스크립트 진행 유무
	{
		// 좌우
		if (GetAsyncKeyState(VK_LEFT) & 0x8001) nKey = 75;
		else if (GetAsyncKeyState(VK_RIGHT) & 0x8001) nKey = 77;
		else nKey = -1;

		if (nKey >= 0)
		{
			switch (nKey)
			{
			case 75: // 왼쪽
				if (CurTime - g_Player.HorizonOldMoveTime > g_Player.HorizonMoveTime - g_Player.nSpeed * 5) // MoveTime 마다 이동 가능
				{
					g_Player.HorizonOldMoveTime = CurTime; // 다음 이동 제어 하기 위해
					if (g_Player.nX - 1 > 1) // 콘솔 스크린 범위을 넘어가지 않기 위해
					{
						g_Player.nX--;

						if (g_Player.Shield[0].nLife)
						{
							for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
							{
								g_Player.Shield[i].nX--;
							}
						}
					}
				}
				break;
			case 77: // 오른쪽
				if (CurTime - g_Player.HorizonOldMoveTime > g_Player.HorizonMoveTime - g_Player.nSpeed * 5) // MoveTime 마다 이동 가능
				{
					g_Player.HorizonOldMoveTime = CurTime; // 다음 이동 제어 하기 위해
					if (g_Player.nX + 1 < SCREEN_WIDTH - 5) // 콘솔 스크린 범위을 넘어가지 않기 위해
					{
						g_Player.nX++;

						if (g_Player.Shield[0].nLife)
						{
							for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
							{
								g_Player.Shield[i].nX++;
							}
						}
					}
				}
				break;
			}
		}
		
		// 상하
		if (GetAsyncKeyState(VK_UP) & 0x8001) nKey = 72;
		else if (GetAsyncKeyState(VK_DOWN) & 0x8001) nKey = 80;
		else nKey = -1;

		if (nKey >= 0)
		{
			switch (nKey)
			{
			case 72: // 위쪽
				if (CurTime - g_Player.VerticalOldMoveTime > g_Player.VerticalMoveTime - g_Player.nSpeed * 5) // MoveTime 마다 이동 가능
				{
					g_Player.VerticalOldMoveTime = CurTime; // 다음 이동 제어 하기 위해
					if (g_Player.nY - 1 > 3) // 콘솔 스크린 범위을 넘어가지 않기 위해
					{
						g_Player.nY--;

						if (g_Player.Shield[0].nLife)
						{
							for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
							{
								g_Player.Shield[i].nY--;
							}
						}
					}
				}
				break;
			case 80: // 아래쪽
				if (CurTime - g_Player.VerticalOldMoveTime > g_Player.VerticalMoveTime - g_Player.nSpeed * 5) // MoveTime 마다 이동 가능
				{
					g_Player.VerticalOldMoveTime = CurTime; // 다음 이동 제어 하기 위해
					if (g_Player.nY + 1 < SCREEN_HEIGHT - 2) // 콘솔 스크린 범위을 넘어가지 않기 위해
					{
						g_Player.nY++;

						if (g_Player.Shield[0].nLife)
						{
							for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
							{
								g_Player.Shield[i].nY++;
							}
						}
					}
				}
				break;
			}
		}

		// 미사일
		if (GetAsyncKeyState(VK_SPACE) & 0x8001) nKey = 32;
		else nKey = -1;

		if (nKey >= 0)
		{
			switch (nKey)
			{
			case 32:

				nCount = g_Player.nMissileCount;
				if (CurTime - g_Player.OldFireTime > g_Player.FireTime - g_Player.nHaste * 10)
				{
					SoundPlay(&player_shot_sound, 0.3f);

					g_Player.OldFireTime = CurTime;
					for (int i = 0; i < HERO_MISSILE_COUNT; i++)
					{
						if (g_PlayerMissile[i].nLife == 0)
						{
							g_PlayerMissile[i].nLife = 1;
							g_PlayerMissile[i].nY = g_Player.nY - 1;
							g_PlayerMissile[i].MoveTime = g_Player.FireMoveTime;
							g_PlayerMissile[i].OldMoveTime = clock();

							if (g_Player.nMissileCount == 3)
							{
								g_PlayerMissile[i].nX = g_Player.nX + nCount;
								nCount--;
							}
							else if (g_Player.nMissileCount == 2)
							{
								if(nCount == 2)
									g_PlayerMissile[i].nX = g_Player.nX + (nCount + 1);
								else if(nCount == 1)
									g_PlayerMissile[i].nX = g_Player.nX + nCount;
								nCount--;
							}
							else if (g_Player.nMissileCount == 1)
							{
								g_PlayerMissile[i].nX = g_Player.nX + (nCount + 1);
								nCount--;
							}

							if (nCount == 0)
							{
								break;
							}
						}
					}
				}
				break;
			}
		}

		// 스킬
		if (GetAsyncKeyState('z') & 0x8001 || GetAsyncKeyState('Z') & 0x8001) nKey = 122;
		else if (GetAsyncKeyState('x') & 0x8001 || GetAsyncKeyState('X') & 0x8001) nKey = 120;
		else nKey = -1;

		if (nKey >= 0)
		{
			switch (nKey)
			{
			case 122:
				if (g_Player.nCannon > 0) 
				{
					if (CurTime - g_Player.OldSkillTime > g_Player.SkillTime)
					{
						SoundPlay(&player_cannon_sound, 0.3f);

						g_Player.OldSkillTime = CurTime;
						for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
						{
							g_Player.CannonMissile[i].nLife = 1;
							g_Player.CannonMissile[i].MoveTime = g_Player.FireMoveTime;
							g_Player.CannonMissile[i].OldMoveTime = clock();
						}

						{
							g_Player.CannonMissile[0].nX = g_Player.nX - 1;
							g_Player.CannonMissile[0].nY = g_Player.nY - 1;
							g_Player.CannonMissile[1].nX = g_Player.nX;
							g_Player.CannonMissile[1].nY = g_Player.nY - 1;
							g_Player.CannonMissile[2].nX = g_Player.nX + 1;
							g_Player.CannonMissile[2].nY = g_Player.nY - 1;
							g_Player.CannonMissile[3].nX = g_Player.nX + 2;
							g_Player.CannonMissile[3].nY = g_Player.nY - 1;
							g_Player.CannonMissile[4].nX = g_Player.nX + 3;
							g_Player.CannonMissile[4].nY = g_Player.nY - 1;
							g_Player.CannonMissile[5].nX = g_Player.nX + 4;
							g_Player.CannonMissile[5].nY = g_Player.nY - 1;
							g_Player.CannonMissile[6].nX = g_Player.nX + 5;
							g_Player.CannonMissile[6].nY = g_Player.nY - 1;

							g_Player.CannonMissile[7].nX = g_Player.nX;
							g_Player.CannonMissile[7].nY = g_Player.nY - 2;
							g_Player.CannonMissile[8].nX = g_Player.nX + 1;
							g_Player.CannonMissile[8].nY = g_Player.nY - 2;
							g_Player.CannonMissile[9].nX = g_Player.nX + 2;
							g_Player.CannonMissile[9].nY = g_Player.nY - 2;
							g_Player.CannonMissile[10].nX = g_Player.nX + 3;
							g_Player.CannonMissile[10].nY = g_Player.nY - 2;
							g_Player.CannonMissile[11].nX = g_Player.nX + 4;
							g_Player.CannonMissile[11].nY = g_Player.nY - 2;

							g_Player.CannonMissile[12].nX = g_Player.nX + 1;
							g_Player.CannonMissile[12].nY = g_Player.nY - 3;
							g_Player.CannonMissile[13].nX = g_Player.nX + 2;
							g_Player.CannonMissile[13].nY = g_Player.nY - 3;
							g_Player.CannonMissile[14].nX = g_Player.nX + 3;
							g_Player.CannonMissile[14].nY = g_Player.nY - 3;

							g_Player.CannonMissile[15].nX = g_Player.nX + 2;
							g_Player.CannonMissile[15].nY = g_Player.nY - 4;
						}
						g_Player.nCannon--;
					}
				}
				break;
			case 120:
				if (g_Player.nShield > 0) 
				{
					if (CurTime - g_Player.OldSkillTime > g_Player.SkillTime)
					{
						SoundPlay(&player_shield_sound, 0.3f);

						g_Player.OldSkillTime = CurTime;
						for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
						{
							g_Player.Shield[i].nLife = 1;
							g_Player.Shield[i].nY = g_Player.nY - 2;
						}

						{
							g_Player.Shield[0].nX = g_Player.nX - 1;
							g_Player.Shield[1].nX = g_Player.nX;
							g_Player.Shield[2].nX = g_Player.nX + 1;
							g_Player.Shield[3].nX = g_Player.nX + 2;
							g_Player.Shield[4].nX = g_Player.nX + 3;
							g_Player.Shield[5].nX = g_Player.nX + 4;
							g_Player.Shield[6].nX = g_Player.nX + 5;
						}
						g_Player.nShield--;
						g_Player.OldShieldKeepTime = clock();
					}
				}
				break;
			}
		}
	}
	else
	{
		KeyBufferClear();
	}
}
void GameEndingKey()
{
	int nKey;

	if (gameKeyControl)
	{
		gameKeyControl = 0;
		KeyBufferClear();
	}

	if (GetAsyncKeyState(VK_SPACE) & 0x8000) nKey = 32;
	else nKey = -1;

	if (nKey >= 0)
	{
		switch (nKey)
		{
		case 32:
			SoundPlay(&menu_select_sound, 1.0f);

			selectMenu = START;
			gameMenu = 1;
			gameKeyControl = 1;
			break;
		}
	}
	KeyBufferClear();
}

void PlayerKey()
{
	clock_t CurTime = clock();

	if (gameMenu == 1) // 메인 화면
	{
		MenuKey();
	}
	else if (gameMenu == 2) // 게임 설명
	{
		HelpKey();
	}
	else if (gameMenu == 0) // 게임 시작
	{
		if (gameState == 1) // 게임 진행
		{
			GameKey();
		}
		else // 게임 클리어,오버
		{
			GameEndingKey();
		}
	}
}

// Stage : 각 스테이지마다 해당하는 변수 초기화 함수 ///////////////////////////////////////////
void EnemyStage()
{
	int random = 0;
	int EnemyTypeIndex = -1;

	g_nEnemyCount = chapter + 4;
	g_Enemy = (ENEMY*)malloc(sizeof(ENEMY) * g_nEnemyCount);
	for (int i = 0; i < g_nEnemyCount; i++) {
		random = (rand() % 100) + 1;

		// 적 타입 랜덤 생성 인덱스
		if (random >= 30) EnemyTypeIndex = 0;
		else if (random >= 10) EnemyTypeIndex = 1;
		else if (random >= 5) EnemyTypeIndex = 2;
		else EnemyTypeIndex = 3;

		g_Enemy[i].nLife = g_EnemyInfo[EnemyTypeIndex].nLife;
		g_Enemy[i].nType = g_EnemyInfo[EnemyTypeIndex].nType;
		g_Enemy[i].AppearTime = 1000 * (i + 1);
		g_Enemy[i].FireTime = g_EnemyInfo[EnemyTypeIndex].FireTime;
		g_Enemy[i].OldFireTime = clock();
		g_Enemy[i].FireMoveTime = g_EnemyInfo[EnemyTypeIndex].FireMoveTime;
		g_Enemy[i].nPatType = rand() % ENEMY_PAT_COUNT;
		g_Enemy[i].nPatIndex = 0;
		g_Enemy[i].nPatStep = -1;
		g_Enemy[i].nX = rand() % 75 + 4;
		g_Enemy[i].nY = 4;
		g_Enemy[i].MoveTime = g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[0].MoveTime / (g_Enemy[i].nType + 1);
		g_Enemy[i].OldMoveTime = clock();
	}
}
void BossStage()
{
	gameBossScript = 1; // 처음 보스 스테이지 들어올 때는 스크립트 먼저 실행되어야 하기 때문

	g_Boss.nLife = chapter * 100;
	g_Boss.nShape = 1;
	g_Boss.nPatType = 0;
	g_Boss.nPatIndex = 0;
	g_Boss.nPatStep = -1;
	g_Boss.nX = g_BossPatInfo[g_Boss.nPatType].nX0;
	g_Boss.nY = g_BossPatInfo[g_Boss.nPatType].nY0;
	g_Boss.MoveTime = g_BossPatInfo[g_Boss.nPatType].pPat[0].MoveTime;
	g_Boss.OldMoveTime = clock();
	g_Boss.FireTime = 1000;
	g_Boss.OldFireTime = clock();
	g_Boss.OldSkillCoolTime = clock();

	// 처음에는 보스 스킬 안씀
	g_Boss.EnableElectronGun = 0;
	g_Boss.EnableAimingShot = 0;
	g_Boss.EnableGuidedShot = 0;
	g_Boss.EnableMeteorFall = 0;

	// 플레이어 위치 초기화
	g_Player.nX = 40;
	g_Player.nY = 40;
}

void Stage()
{
	// 각 스테이지 마다 세팅 변경
	if (!EnemyChange)
	{
		EnemyChange = 1;
		if (g_Boss.nLife > 0) // 보스전 중
		{
			EnemyChange = 0; // 아직 스테이지 변경 안됨
		}
		else // 보스전이 아니거나 보스전이 끝난 경우
		{
			if (stage == 5) // 보스전 끝남
			{
				stage = 0;
				chapter++;
			}

			// 스테이지에 적기가 아직도 존재 하는지
			for (int i = 0; i < g_nEnemyCount; i++)
			{
				if (g_Enemy[i].nLife != -1)
				{
					EnemyChange = 0; // 아직 스테이지 변경 안됨
					break;
				}
			}
		}
	}

	if (EnemyChange) // EnemyChange : 변경 해야 하는지
	{
		stage++;

		if (stage != 5) // 5일때 보스전
		{
			EnemyStage(); // 적기 스테이지 초기화
		}
		else
		{
			BossStage(); // 보스 스테이지 초기화
		}

		// 다음 stage 함수 실행시 초기에 값들이 적용 되어 있던 값
		g_nEnemyIndex = 0;
		g_StartTime = clock();
		EnemyChange = 0;
	}
}

// Update : 모든 변수 변화 함수 ///////////////////////////////////////////
void PlayerMissileUpdate(clock_t CurTime)
{
	// 일반 미사일 이동
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife)
		{
			if (CurTime - g_PlayerMissile[i].OldMoveTime > g_PlayerMissile[i].MoveTime)
			{
				g_PlayerMissile[i].OldMoveTime = CurTime;
				if (g_PlayerMissile[i].nY - 1 > 3)
				{
					g_PlayerMissile[i].nY--;
				}
				else
					g_PlayerMissile[i].nLife = 0;
			}
		}
	}
	// 캐논 미사일 이동
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife)
		{
			if (CurTime - g_Player.CannonMissile[i].OldMoveTime > g_Player.CannonMissile[i].MoveTime)
			{
				g_Player.CannonMissile[i].OldMoveTime = CurTime;

				if (g_Player.CannonMissile[i].nY - 1 > 3)
				{
					g_Player.CannonMissile[i].nY--;
				}
				else
					g_Player.CannonMissile[i].nLife = 0;
			}
		}
	}
	// 실드 유무
	if (g_Player.Shield[0].nLife)
	{
		if (CurTime - g_Player.OldShieldKeepTime > g_Player.ShieldKeepTime)
		{
			for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
			{
				g_Player.Shield[i].nLife = 0;
			}
		}
	}
}

void ItemDropUpdate(int nX, int nY)
{
	int random = rand() % 100;

	if (random >= 85)
	{
		random = rand() % 100;
		for (int i = 0; i < sizeof(g_Item) / sizeof(ITEM); i++)
		{
			if (g_Item[i].nLife == 0)
			{
				g_Item[i].nLife = 1;
				g_Item[i].nX = nX;
				g_Item[i].nY = nY + 1;

				if (random >= 80)
				{
					g_Item[i].nType = 0;
				}
				else if (random >= 52)
				{
					g_Item[i].nType = 1;
				}
				else if (random >= 23)
				{
					g_Item[i].nType = 2;
				}
				else if (random >= 13)
				{
					g_Item[i].nType = 3;
				}
				else if (random >= 3)
				{
					g_Item[i].nType = 4;
				}
				else
				{
					g_Item[i].nType = 5;
				}
				g_Item[i].OldMoveTime = clock();
				break;
			}
		}
	}
}
void ItemMoveUpdate(clock_t CurTime)
{
	for (int i = 0; i < sizeof(g_Item) / sizeof(ITEM); i++)
	{
		if (g_Item[i].nLife)
		{
			if (CurTime - g_Item[i].OldMoveTime >= g_Item[i].MoveTime)
			{
				g_Item[i].OldMoveTime = CurTime;
				g_Item[i].nY++;

				if (g_Item[i].nY > SCREEN_HEIGHT - 3)
				{
					g_Item[i].nLife = 0;
				}
			}
		}
	}
}
void ItemAndPlayerCollisionUpdate()
{
	int CollisionExis;

	for (int i = 0; i < sizeof(g_Item) / sizeof(ITEM); i++)
	{
		if (g_Item[i].nLife)
		{
			CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
				g_Item[i].nX, g_Item[i].nY, g_Item[i].nX + 2, g_Item[i].nY + 1);
			if (CollisionExis)
			{
				g_Item[i].nLife = 0;
				switch (g_Item[i].nType)
				{
				case 0:
					if(g_Player.nLife < 20)
						g_Player.nLife++;
					break;
				case 1:
					if (g_Player.nCannon < 10)
						g_Player.nCannon++;
					break;
				case 2:
					if (g_Player.nShield < 10)
						g_Player.nShield++;
					break;
				case 3:
					if(g_Player.nSpeed < 5)
						g_Player.nSpeed++;
					break;
				case 4:
					if (g_Player.nHaste < 5)
						g_Player.nHaste++;
					break;
				case 5:
					if (g_Player.nMissileCount < 3)
						g_Player.nMissileCount++;
					break;
				}
				SoundPlay(&item_get_sound, 0.1f);
				break;
			}
		}
	}
}

void EnemyCreateUpdate(clock_t CurTime)
{
	for (int i = g_nEnemyIndex; i < g_nEnemyCount; i++)
	{
		if (g_Enemy[i].nLife == 0)
		{
			if (CurTime - g_StartTime >= g_Enemy[i].AppearTime)
			{
				g_Enemy[i].nLife = 1;
				g_nEnemyIndex++;
			}
			else
				break;
		}
	}
}
void EnemyMoveAndMissileUpdate(clock_t CurTime)
{
	int nSignX, nSignY;

	for (int i = 0; i < g_nEnemyIndex; i++)
	{
		if (g_Enemy[i].nLife == 1)
		{
			if (CurTime - g_Enemy[i].OldMoveTime > g_Enemy[i].MoveTime)
			{
				g_Enemy[i].OldMoveTime = CurTime;
				g_Enemy[i].nPatStep++;

				if (g_Enemy[i].nPatStep == g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[g_Enemy[i].nPatIndex].nStep)
				{
					g_Enemy[i].nPatIndex++;

					if (g_Enemy[i].nPatIndex == g_EnemyPatInfo[g_Enemy[i].nPatType].nCount)
					{
						g_Enemy[i].nLife = -1;
						g_Earth.nLife -= 1;
						for (int j = 0; j < (int)((sizeof(g_Earth.surface) / sizeof(int)) * (EARTH_LIFE - g_Earth.nLife) / 20); j++)
						{
							g_Earth.surface[j] = -1;
						}
						SoundPlay(&earth_collision_sound, 0.3f);
						continue;
					}
					else
					{
						g_Enemy[i].MoveTime =
							g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[g_Enemy[i].nPatIndex].MoveTime / (g_Enemy[i].nType + 1);
						g_Enemy[i].nPatStep = 0;
					}
				}

				switch (g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[g_Enemy[i].nPatIndex].nDirect)
				{
				case UP: 		nSignX = 0;	nSignY = -1; break;
				case UP_RIGHT: 	nSignX = 1;	nSignY = -1; break;
				case RIGHT:		nSignX = 1;	nSignY = 0;	break;
				case DOWN_RIGHT:nSignX = 1;	nSignY = 1;	break;
				case DOWN:		nSignX = 0;	nSignY = 1;	break;
				case DOWN_LEFT:	nSignX = -1; nSignY = 1;	break;
				case LEFT:		nSignX = -1; nSignY = 0;	break;
				case UP_LEFT:	nSignX = -1; nSignY = -1; break;
				}
				g_Enemy[i].nX += nSignX * g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[g_Enemy[i].nPatIndex].nDist;
				g_Enemy[i].nY += nSignY * g_EnemyPatInfo[g_Enemy[i].nPatType].pPat[g_Enemy[i].nPatIndex].nDist;

				if (g_Enemy[i].nX < 4) 			g_Enemy[i].nX = 5;
				if (g_Enemy[i].nX > SCREEN_WIDTH - 4) 	g_Enemy[i].nX = SCREEN_WIDTH - 5;
				if (g_Enemy[i].nY < 1)			g_Enemy[i].nY = 2;
				if (g_Enemy[i].nY > SCREEN_HEIGHT - 1) 	g_Enemy[i].nY = SCREEN_HEIGHT - 2;
			}

			if (CurTime - g_Enemy[i].OldFireTime > g_Enemy[i].FireTime)
			{
				g_Enemy[i].OldFireTime = CurTime;

				for (int j = 0; j < ENEMY_MISSILE_COUNT; j++)
				{
					if (g_EnemyMissile[j].nLife == 0)
					{
						g_EnemyMissile[j].nX = g_Enemy[i].nX;
						g_EnemyMissile[j].nY = g_Enemy[i].nY + 1;
						g_EnemyMissile[j].nLife = 1;
						g_EnemyMissile[j].MoveTime = g_Enemy[i].FireMoveTime;
						g_EnemyMissile[j].OldMoveTime = CurTime;
						break;
					}
				}
			}
		}
	}
}
void EnemyMissileMoveUpdate(clock_t CurTime)
{
	for (int i = 0; i < ENEMY_MISSILE_COUNT; i++)
	{
		if (g_EnemyMissile[i].nLife == 1)
		{
			if (CurTime - g_EnemyMissile[i].OldMoveTime > g_EnemyMissile[i].MoveTime)
			{
				g_EnemyMissile[i].OldMoveTime = CurTime;
				if (g_EnemyMissile[i].nY + 1 > SCREEN_HEIGHT - 3)
				{
					g_EnemyMissile[i].nLife = 0;
				}
				else
					g_EnemyMissile[i].nY++;
			}
		}
	}
}
void EnemyAndPlayerCollisionUpdate()
{
	int CollisionExis = 0;

	// 플레이어 미사일 <=> 적기 미사일
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife == 1)
		{
			for (int j = 0; j < ENEMY_MISSILE_COUNT; j++)
			{
				if (g_EnemyMissile[j].nLife == 1)
				{
					CollisionExis = Collision(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, g_PlayerMissile[i].nX + 1, g_PlayerMissile[i].nY + 1,
						g_EnemyMissile[j].nX, g_EnemyMissile[j].nY, g_EnemyMissile[j].nX + 2, g_EnemyMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_PlayerMissile[i].nLife = 0;
						g_EnemyMissile[j].nLife = 0;
						break;
					}
				}
			}
		}
	}
	// 플레이어 캐논 미사일 <=> 적기 미사일
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife == 1)
		{
			for (int j = 0; j < ENEMY_MISSILE_COUNT; j++)
			{
				if (g_EnemyMissile[j].nLife == 1)
				{
					CollisionExis = Collision(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, g_Player.CannonMissile[i].nX + 1, g_Player.CannonMissile[i].nY + 1,
						g_EnemyMissile[j].nX, g_EnemyMissile[j].nY, g_EnemyMissile[j].nX + 2, g_EnemyMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_EnemyMissile[j].nLife = 0;
						break;
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 적기 미사일
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < ENEMY_MISSILE_COUNT; j++)
			{
				if (g_EnemyMissile[j].nLife == 1)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						g_EnemyMissile[j].nX, g_EnemyMissile[j].nY, g_EnemyMissile[j].nX + 2, g_EnemyMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_EnemyMissile[j].nLife = 0;
					}
				}
			}
		}
	}

	// 플레이어 미사일 <=> 적기
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife == 1)
		{
			for (int j = 0; j < g_nEnemyIndex; j++)
			{
				if (g_Enemy[j].nLife == 1)
				{
					CollisionExis = Collision(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, g_PlayerMissile[i].nX + 1, g_PlayerMissile[i].nY + 1,
						g_Enemy[j].nX, g_Enemy[j].nY, g_Enemy[j].nX + 2, g_Enemy[j].nY + 1);
					if (CollisionExis)
					{
						g_Enemy[j].nLife = -1;
						g_PlayerMissile[i].nLife = 0;
						score += 500;

						ItemDropUpdate(g_Enemy[j].nX, g_Enemy[j].nY);

						SoundPlay(&collision_sound, 0.3f);
					}
				}
			}
		}
	}
	// 플레이어 캐논 미사일 <=> 적기
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife == 1)
		{
			for (int j = 0; j < g_nEnemyIndex; j++)
			{
				if (g_Enemy[j].nLife == 1)
				{
					CollisionExis = Collision(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, g_Player.CannonMissile[i].nX + 1, g_Player.CannonMissile[i].nY + 1,
						g_Enemy[j].nX, g_Enemy[j].nY, g_Enemy[j].nX + 2, g_Enemy[j].nY + 1);
					if (CollisionExis)
					{
						g_Enemy[j].nLife = -1;
						score += 500;

						ItemDropUpdate(g_Enemy[j].nX, g_Enemy[j].nY);

						SoundPlay(&collision_sound, 0.3f);
					}
				}
			}
		}
	}

	// 플레이어 <=> 적기 미사일
	if (g_Player.EnableDamage)
		for (int i = 0; i < ENEMY_MISSILE_COUNT; i++)
		{
			if (g_EnemyMissile[i].nLife == 1)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					g_EnemyMissile[i].nX, g_EnemyMissile[i].nY, g_EnemyMissile[i].nX + 2, g_EnemyMissile[i].nY + 1);
				if (CollisionExis)
				{
					g_Player.nLife--;
					g_EnemyMissile[i].nLife = 0;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
				}
			}
		}

	// 플레이어 <-> 적기
	if (g_Player.EnableDamage)
		for (int i = 0; i < g_nEnemyIndex; i++)
		{
			if (g_Enemy[i].nLife == 1)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					g_Enemy[i].nX, g_Enemy[i].nY, g_Enemy[i].nX + 2, g_Enemy[i].nY + 1);
				if (CollisionExis)
				{
					g_Enemy[i].nLife = -1;
					g_Player.nLife--;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&collision_sound, 0.3f);
					SoundPlay(&player_hit_sound, 0.5f);
				}
			}
		}

}

void BossScriptUpdate(clock_t CurTime)
{
	if (CurTime - g_UpdateOldTime >= 4000)
	{
		if (CurScript == NONE)
		{
			CurScript = PLAYER_TALK;
			g_UpdateOldTime = CurTime;
		}
		else if (CurScript == PLAYER_TALK)
		{
			CurScript = BOSS_TALK;
			g_UpdateOldTime = CurTime;
		}
		else if (CurScript == BOSS_TALK)
		{
			CurScript = NONE;
			g_UpdateOldTime = CurTime;
			gameBossScript = 0;
		}
	}
	g_Player.nX = 40;
	g_Player.nY = 40;
}
void BossMoveUpdate(clock_t CurTime)
{
	int nSignX, nSignY;

	if (CurTime - g_Boss.OldMoveTime > g_Boss.MoveTime)
	{
		g_Boss.OldMoveTime = CurTime;
		g_Boss.nPatStep++;

		if (g_Boss.nPatStep == g_BossPatInfo[g_Boss.nPatType].pPat[g_Boss.nPatIndex].nStep)
		{
			g_Boss.nPatIndex++;

			if (g_Boss.nPatIndex == g_BossPatInfo[g_Boss.nPatType].nCount)
			{
				g_Boss.nPatType++;
				g_Boss.nPatType %= nBossFileCount;
				g_Boss.nPatIndex = 0;
				g_Boss.nPatStep = -1;

				g_Boss.nX = g_BossPatInfo[g_Boss.nPatType].nX0;
				g_Boss.nY = g_BossPatInfo[g_Boss.nPatType].nY0;
				g_Boss.MoveTime = g_BossPatInfo[g_Boss.nPatType].pPat[0].MoveTime;
				g_Boss.OldMoveTime = clock();
			}
			else
			{
				g_Boss.MoveTime = g_BossPatInfo[g_Boss.nPatType].pPat[g_Boss.nPatIndex].MoveTime;
				g_Boss.nPatStep = 0;
			}
		}

		switch (g_BossPatInfo[g_Boss.nPatType].pPat[g_Boss.nPatIndex].nDirect)
		{
		case UP:		nSignX = 0;	nSignY = -1; break;
		case UP_RIGHT:	nSignX = 1;	nSignY = -1; break;
		case RIGHT:		nSignX = 1;	nSignY = 0;	break;
		case DOWN_RIGHT:nSignX = 1;	nSignY = 1;	break;
		case DOWN:		nSignX = 0;	nSignY = 1;	break;
		case DOWN_LEFT:	nSignX = -1; nSignY = 1;	break;
		case LEFT:		nSignX = -1; nSignY = 0;	break;
		case UP_LEFT:	nSignX = -1; nSignY = -1; break;
		}

		g_Boss.nX += nSignX * g_BossPatInfo[g_Boss.nPatType].pPat[g_Boss.nPatIndex].nDist;
		g_Boss.nY += nSignY * g_BossPatInfo[g_Boss.nPatType].pPat[g_Boss.nPatIndex].nDist;
	}
}
void BossMissileUpdate(clock_t CurTime)
{
	int nFireMissileCount; // 보스 동시에 쏘는 미사일 수 제어

	// 보스 미사일 생성
	if (CurTime - g_Boss.OldFireTime > g_Boss.FireTime)
	{
		g_Boss.OldFireTime = CurTime;
		nFireMissileCount = 0;

		for (int i = 0; i < BOSS_MISSILE_COUNT; i++)
		{
			if (g_BossMissile[i].nLife == 0)
			{
				g_BossMissile[i].nX = g_Boss.nX + 6;
				g_BossMissile[i].nY = g_Boss.nY + 2;
				g_BossMissile[i].nLife = 1;
				g_BossMissile[i].nDirect = DOWN_LEFT - nFireMissileCount;
				g_BossMissile[i].nType = DOWN_LEFT - nFireMissileCount;
				g_BossMissile[i].MoveTime = 50;
				g_BossMissile[i].OldMoveTime = CurTime;
				nFireMissileCount++;

				if (nFireMissileCount > 2) break;
			}
		}
	}
}
void BossSkillCoolTimeUpdate(clock_t CurTime)
{
	float random;

	if (CurTime - g_Boss.OldSkillCoolTime >= g_Boss.SkillCoolTime)
	{
		g_Boss.OldSkillCoolTime = CurTime;
		random = (rand() % 1000) / 10.0f;

		if (random <= 2.0f && chapter >= 5)
		{
			g_Boss.EnableMeteorFall = 1;
			g_Boss.MeteorCount = 0;
			g_Boss.OldMeteorShotTime = clock();
		}
		else if (random <= 5.0f && chapter >= 4)
		{
			g_Boss.EnableAimingShot = 1; // 조준탄 스킬 사용 유무
			g_Boss.SmallBossChange = 1; // 보스 소형화 유무
			g_Boss.OldSmallChangeTime = CurTime;
		}
		else if (random <= 15.0f && chapter >= 3)
		{
			for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
			{
				if (g_Boss.GuidedShot[i].nLife == 1)
					break;

				if (i == sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE) - 1)
				{
					g_Boss.EnableGuidedShot = 1;
					g_Boss.OldEndGuidedShotTime = CurTime;
					g_Boss.OldGuidedShotTime = CurTime;
				}
			}
		}
		else if (random <= 30.0f && chapter >= 2)
		{
			g_Boss.EnableElectronGun = 1; // 포 스킬 사용 유무
			g_Boss.WarningElectronGun = 1; // 포 사정권 경고 이미지
			g_Boss.OldWarningTime = CurTime;
		}

	}
}
void BossElectronGunUpdate(clock_t CurTime)
{
	if (g_Boss.WarningElectronGun) // 포 경고 이미지
	{
		if (CurTime - g_Boss.OldWarningTime >= g_Boss.WarningTime)
		{
			g_Boss.WarningElectronGun = 0; // WarningTime 만큼 경고가 뜸
			g_Boss.OldEndElectronGunTime = CurTime; // 포 발사 기준 시간 저장
			SoundPlay(&boss_ElectronGun_sound, 0.2f);
		}
	}
	else
	{
		// 포 생성
		for (int i = 0; i < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.ElectronGun[i].nLife == 0)
			{
				g_Boss.ElectronGun[i].nLife = 1;
				g_Boss.ElectronGun[i].nX = g_Boss.nX;
				g_Boss.ElectronGun[i].nY = g_Boss.nY + 3;
				break;
			}
		}

		if (CurTime - g_Boss.OldEndElectronGunTime >= g_Boss.EndElectronGunTime)
		{
			g_Boss.EnableElectronGun = 0;
		}
	}
}
void BossAimingShotUpdate(clock_t CurTime)
{
	double radian;
	double angle;

	if (g_Boss.SmallBossChange) // 보스 스몰화
	{
		if (CurTime - g_Boss.OldSmallChangeTime >= g_Boss.SmallChangeTime)
		{
			if (g_Boss.nShape == 1)
			{
				g_Boss.nShape = 2;
				g_Boss.OldSmallChangeTime = CurTime;
			}
			else if (g_Boss.nShape == 2)
			{
				g_Boss.nShape = 3;
				g_Boss.OldSmallChangeTime = CurTime;
			}
			else if (g_Boss.nShape == 3)
			{
				g_Boss.SmallBossChange = 0;
				g_Boss.OldSmallChangeTime = CurTime;
				g_Boss.OldEndAimingShotTime = CurTime;

				g_Boss.nSaveX = g_Boss.nX;
				g_Boss.nSaveY = g_Boss.nY;

				g_Boss.nX = SCREEN_WIDTH / 2;
				g_Boss.nY = SCREEN_HEIGHT / 2;
			}
		}
	}
	else
	{
		if (CurTime - g_Boss.OldAimingShotTime >= g_Boss.AimingShotTime)
		{
			g_Boss.OldAimingShotTime = CurTime;
			for (int i = 0; i < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); i++)
			{
				if (g_Boss.AimingShot[i].nLife == 0)
				{
					radian = asin((((double)g_Player.nX + 2.0) - (double)(g_Boss.nX)) / Pythagoras(g_Player.nX + 2, g_Player.nY, g_Boss.nX, g_Boss.nY));
					angle = radian * 57.296;

					if (g_Player.nY < g_Boss.nY)
					{
						if (g_Player.nX >= g_Boss.nX)
						{
							angle = 90.0 - angle;
							angle += 90.0;
						}
						else
						{
							angle = - 90.0 - angle;
							angle -= 90.0;
						}
					}

					g_Boss.AimingShot[i].nLife = 1;
					if (angle >= 160.0 || angle < -160.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY - 1.0;
						g_Boss.AimingShot[i].nType = 0;
					}
					else if (angle >= 110.0 && angle < 160.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX + 1.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY - 1.0;
						g_Boss.AimingShot[i].nType = 1;
					}
					else if (angle >= 70.0 && angle < 110.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX + 1.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY;
						g_Boss.AimingShot[i].nType = 2;
					}
					else if (angle >= 20.0 && angle < 70.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX + 1.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY + 1.0;
						g_Boss.AimingShot[i].nType = 3;
					}
					else if (angle >= -20.0 && angle < 20.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY + 1.0;
						g_Boss.AimingShot[i].nType = 4;
					}
					else if (angle >= -70.0 && angle < -20.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX - 2.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY + 1.0;
						g_Boss.AimingShot[i].nType = 5;
					}
					else if (angle >= -110.0 && angle < -70.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX - 2.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY;
						g_Boss.AimingShot[i].nType = 6;
					}
					else if (angle >= -160.0 && angle < -110.0)
					{
						g_Boss.AimingShot[i].nXf = (double)g_Boss.nX - 2.0;
						g_Boss.AimingShot[i].nYf = (double)g_Boss.nY - 1.0;
						g_Boss.AimingShot[i].nType = 7;
					}
					g_Boss.AimingShot[i].nVectorX = ((double)g_Player.nX + 2.0 - (double)g_Boss.nX) / Pythagoras(g_Player.nX + 2, g_Player.nY, g_Boss.nX, g_Boss.nY);
					g_Boss.AimingShot[i].nVectorY = ((double)g_Player.nY - (double)g_Boss.nY) / Pythagoras(g_Player.nX + 2, g_Player.nY, g_Boss.nX, g_Boss.nY);

					g_Boss.AimingShot[i].MoveTime = g_Boss.AimingShotTime;
					g_Boss.AimingShot[i].OldMoveTime = clock();

					SoundPlay(&boss_AimingShot_sound, 0.1f);
					break;
				}
			}
		}

		if (CurTime - g_Boss.OldEndAimingShotTime >= g_Boss.EndAimingShotTime)
		{
			g_Boss.EnableAimingShot = 0;
			g_Boss.SmallBossChange = 0;
			g_Boss.nShape = 1;
			g_Boss.nX = g_Boss.nSaveX;
			g_Boss.nY = g_Boss.nSaveY;
		}
	}
}
void BossGuidedShotUpdate(clock_t CurTime)
{
	if (CurTime - g_Boss.OldGuidedShotTime >= g_Boss.GuidedShotTime)
	{
		g_Boss.OldGuidedShotTime = CurTime;
		for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.GuidedShot[i].nLife == 0)
			{
				g_Boss.GuidedShot[i].nLife = 1;
				g_Boss.GuidedShot[i].nVectorX = 0;
				g_Boss.GuidedShot[i].nVectorY = -1.0;
				g_Boss.GuidedShot[i].nXf = (double)g_Boss.nX + 7.0;
				g_Boss.GuidedShot[i].nYf = (double)g_Boss.nY - 1.0;
				g_Boss.GuidedShot[i].MoveTime = 120;
				g_Boss.GuidedShot[i].OldMoveTime = clock();

				SoundPlay(&boss_guideshot_sound, 0.2f);
				break;
			}
		}
	}

	if (CurTime - g_Boss.OldEndGuidedShotTime >= g_Boss.EndGuidedShotTime)
	{
		for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
		{
			g_Boss.GuidedShot[i].nLife = 0;
		}
		g_Boss.EnableGuidedShot = 0;
	}
}
void BossMeteorFallUpdate(clock_t CurTime)
{
	if (CurTime - g_Boss.OldMeteorShotTime >= g_Boss.MeteorShotTime)
	{
		g_Boss.OldMeteorShotTime = CurTime;

		if (g_Boss.MeteorCount == sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE))
		{
			g_Boss.EnableMeteorFall = 0;
			for (int i = 0; i < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); i++)
			{
				if (g_Boss.Meteor[i].nLife)
				{
					g_Boss.EnableMeteorFall = 1;
					break;
				}
			}
		}
		else
		{
			g_Boss.Meteor[g_Boss.MeteorCount].nLife = 1;
			g_Boss.Meteor[g_Boss.MeteorCount].nX = g_Boss.nX + 7;
			g_Boss.Meteor[g_Boss.MeteorCount].nY = g_Boss.nY - 1;
			g_Boss.Meteor[g_Boss.MeteorCount].nDirect = UP;
			g_Boss.Meteor[g_Boss.MeteorCount].MoveTime = 60;
			g_Boss.Meteor[g_Boss.MeteorCount].OldMoveTime = clock();

			SoundPlay(&boss_meteorfall_sound, 0.2f);
			g_Boss.MeteorCount++;
		}
	}
}
void BossMissileMoveUpdate(clock_t CurTime)
{
	int nSignX, nSignY;

	// 미사일 업데이트
	for (int i = 0; i < BOSS_MISSILE_COUNT; i++)
	{
		if (g_BossMissile[i].nLife)
		{
			if (CurTime - g_BossMissile[i].OldMoveTime > g_BossMissile[i].MoveTime)
			{
				g_BossMissile[i].OldMoveTime = CurTime;

				switch (g_BossMissile[i].nDirect)
				{
				case UP:		nSignX = 0;	nSignY = -1; break;
				case UP_RIGHT:	nSignX = 1;	nSignY = -1; break;
				case RIGHT:		nSignX = 1;	nSignY = 0;	break;
				case DOWN_RIGHT:nSignX = 1;	nSignY = 1;	break;
				case DOWN:		nSignX = 0;	nSignY = 1;	break;
				case DOWN_LEFT:	nSignX = -1; nSignY = 1;break;
				case LEFT:		nSignX = -1; nSignY = 0;break;
				case UP_LEFT:	nSignX = -1; nSignY = -1; break;
				}
				g_BossMissile[i].nX += nSignX;
				g_BossMissile[i].nY += nSignY;
			}

			if (g_BossMissile[i].nX < 2 || g_BossMissile[i].nX > SCREEN_WIDTH - 3 ||
				g_BossMissile[i].nY < 3 || g_BossMissile[i].nY > SCREEN_HEIGHT - 3)
			{
				g_BossMissile[i].nLife = 0;
			}
		}
	}

	// 포 업데이트
	for (int i = 0; i < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.ElectronGun[i].nLife)
		{
			g_Boss.ElectronGun[i].nX = g_Boss.nX;
			if (g_Boss.ElectronGun[i].nY + 1 > SCREEN_HEIGHT - 3)
			{
				g_Boss.ElectronGun[i].nLife = 0;
			}
			else
			{
				g_Boss.ElectronGun[i].nY++;
			}
		}
	}

	// 조준탄 업데이트
	for (int i = 0; i < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.AimingShot[i].nLife)
		{
			if (CurTime - g_Boss.AimingShot[i].OldMoveTime >= g_Boss.AimingShot[i].MoveTime)
			{
				g_Boss.AimingShot[i].nXf += g_Boss.AimingShot[i].nVectorX * 2.5;
				g_Boss.AimingShot[i].nYf += g_Boss.AimingShot[i].nVectorY * 2.5;
				if (g_Boss.AimingShot[i].nXf < 2.0 || g_Boss.AimingShot[i].nXf >(double)(SCREEN_WIDTH - 3) ||
					g_Boss.AimingShot[i].nYf < 3.0 || g_Boss.AimingShot[i].nYf >(double)(SCREEN_HEIGHT - 3))
				{
					g_Boss.AimingShot[i].nLife = 0;
				}
			}
		}
	}
	
	// 유도탄 업데이트
	for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
	{
		double nVectorX, nVectorY;
		double nSlope;
		if (g_Boss.GuidedShot[i].nLife)
		{
			if (CurTime - g_Boss.GuidedShot[i].OldMoveTime >= g_Boss.GuidedShot[i].MoveTime)
			{
				g_Boss.GuidedShot[i].OldMoveTime = CurTime;
				nVectorX = (((double)g_Player.nX - g_Boss.GuidedShot[i].nXf) * g_Boss.nVectorMaxPower) / (3 * Pythagoras(g_Player.nX, g_Player.nY, (int)g_Boss.GuidedShot[i].nXf, (int)g_Boss.GuidedShot[i].nYf));
				nVectorY = (((double)g_Player.nY - g_Boss.GuidedShot[i].nYf) * g_Boss.nVectorMaxPower) / (3 * Pythagoras(g_Player.nX, g_Player.nY, (int)g_Boss.GuidedShot[i].nXf, (int)g_Boss.GuidedShot[i].nYf));

				g_Boss.GuidedShot[i].nVectorX += nVectorX;
				g_Boss.GuidedShot[i].nVectorY += nVectorY;
				nSlope = Pythagoras(0, 0, (int)g_Boss.GuidedShot[i].nVectorX, (int)g_Boss.GuidedShot[i].nVectorY);

				if (nSlope > g_Boss.nVectorMaxPower)
				{
					g_Boss.GuidedShot[i].nVectorX = (g_Boss.GuidedShot[i].nVectorX * g_Boss.nVectorMaxPower) / nSlope;
					g_Boss.GuidedShot[i].nVectorY = (g_Boss.GuidedShot[i].nVectorY * g_Boss.nVectorMaxPower) / nSlope;
				}

				g_Boss.GuidedShot[i].nXf += g_Boss.GuidedShot[i].nVectorX;
				g_Boss.GuidedShot[i].nYf += g_Boss.GuidedShot[i].nVectorY;
			}

			if (CurTime - g_Boss.OldEndGuidedShotTime >= g_Boss.EndGuidedShotTime)
			{
				g_Boss.GuidedShot[i].nLife = 0;
			}
		}
	}

	// 운석 낙하 업데이트
	for (int i = 0; i < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.Meteor[i].nLife)
		{
			if (CurTime - g_Boss.Meteor[i].OldMoveTime > g_Boss.Meteor[i].MoveTime)
			{
				g_Boss.Meteor[i].OldMoveTime = CurTime;

				switch (g_Boss.Meteor[i].nDirect)
				{
				case UP:		nSignX = 0;	nSignY = -1; break;
				case UP_RIGHT:	nSignX = 1;	nSignY = -1; break;
				case RIGHT:		nSignX = 1;	nSignY = 0;	break;
				case DOWN_RIGHT:nSignX = 1;	nSignY = 1;	break;
				case DOWN:		nSignX = 0;	nSignY = 1;	break;
				case DOWN_LEFT:	nSignX = -1; nSignY = 1;break;
				case LEFT:		nSignX = -1; nSignY = 0;break;
				case UP_LEFT:	nSignX = -1; nSignY = -1; break;
				}
				g_Boss.Meteor[i].nX += nSignX;
				g_Boss.Meteor[i].nY += nSignY;

				if (g_Boss.Meteor[i].nY == -3)
				{
					g_Boss.Meteor[i].nX = rand() % 75 + 4;
					g_Boss.Meteor[i].nY = 3;
					g_Boss.Meteor[i].nDirect = DOWN;
				}
				else if (g_Boss.Meteor[i].nY > SCREEN_HEIGHT - 3)
				{
					g_Boss.Meteor[i].nLife = 0;
					g_Earth.nLife -= 1;
					for (int j = 0; j < (int)((sizeof(g_Earth.surface) / sizeof(int)) * (EARTH_LIFE - g_Earth.nLife) / 20); j++)
					{
						g_Earth.surface[j] = -1;
					}

					SoundPlay(&earth_collision_sound, 0.3f);
				}

				if (g_Boss.Meteor[i].nDirect == DOWN)
				{
					g_Boss.Meteor[i].MoveTime = 500/(int)(2 + 0.5*(g_Boss.Meteor[i].nY - 3));
				}
			}
		}
	}
}
void BossAndPlayerCollisionUpdate()
{
	int CollisionExis = 0;

	// 플레이어 <=> 보스 공격
	if (g_Player.EnableDamage)
	{
		// 플레이어 <=> 보스 미사일
		for (int i = 0; i < BOSS_MISSILE_COUNT; i++)
		{
			if (g_Player.nLife > 0 && g_BossMissile[i].nLife)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					g_BossMissile[i].nX, g_BossMissile[i].nY, g_BossMissile[i].nX + 2, g_BossMissile[i].nY + 1);
				if (CollisionExis)
				{
					g_Player.nLife--;
					g_BossMissile[i].nLife = 0;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
					break;
				}
			}
		}

		// 플레이어 <=> 보스 포
		for (int i = 0; i < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.ElectronGun[i].nLife)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					g_Boss.ElectronGun[i].nX, g_Boss.ElectronGun[i].nY, g_Boss.ElectronGun[i].nX + 16, g_Boss.ElectronGun[i].nY + 1);
				if (CollisionExis)
				{
					g_Player.nLife--;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
					break;
				}
			}
		}

		// 플레이어 <=> 보스 조준탄
		for (int i = 0; i < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.AimingShot[i].nLife)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					(int)g_Boss.AimingShot[i].nXf, (int)g_Boss.AimingShot[i].nYf, (int)g_Boss.AimingShot[i].nXf + 2, (int)g_Boss.AimingShot[i].nYf + 1);
				if (CollisionExis && !g_Player.Shield[0].nLife)
				{
					g_Boss.AimingShot[i].nLife = 0;
					g_Player.nLife--;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
					break;
				}
			}
		}

		// 플레이어 <=> 보스 유도탄
		for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.GuidedShot[i].nLife)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					(int)g_Boss.GuidedShot[i].nXf, (int)g_Boss.GuidedShot[i].nYf, (int)g_Boss.GuidedShot[i].nXf + 1, (int)g_Boss.GuidedShot[i].nYf + 1);
				if (CollisionExis)
				{
					g_Boss.GuidedShot[i].nLife = 0;
					g_Player.nLife--;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
					break;
				}
			}
		}

		// 플레이어 <=> 운석
		for (int i = 0; i < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); i++)
		{
			if (g_Boss.Meteor[i].nLife)
			{
				CollisionExis = Collision(g_Player.nX, g_Player.nY, g_Player.nX + 5, g_Player.nY + 1,
					g_Boss.Meteor[i].nX, g_Boss.Meteor[i].nY, g_Boss.Meteor[i].nX + 2, g_Boss.Meteor[i].nY + 1);
				if (CollisionExis)
				{
					g_Boss.Meteor[i].nLife = 0;
					g_Player.nLife--;
					g_Player.EnableDamage = 0;
					g_Player.OldHitTime = clock();

					SoundPlay(&player_hit_sound, 0.5f);
					break;
				}
			}
		}
	}

	if (!g_Boss.EnableAimingShot)
	{
		// 플레이어 미사일 <=> 보스
		for (int i = 0; i < HERO_MISSILE_COUNT; i++)
		{
			if (g_PlayerMissile[i].nLife && g_Boss.nLife > 0)
			{
				CollisionExis = Collision(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, g_PlayerMissile[i].nX + 1, g_PlayerMissile[i].nY + 1,
					g_Boss.nX, g_Boss.nY, g_Boss.nX + 16, g_Boss.nY + 3);

				if (CollisionExis)
				{
					g_Boss.nLife--;
					g_PlayerMissile[i].nLife = 0;

					if (g_Boss.nLife <= 0)
					{
						score += 5000;

						SoundPlay(&collision_sound, 0.3f);
					}
				}
			}
		}

		// 플레이어 캐논 미사일 <=> 보스
		for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
		{
			if (g_Player.CannonMissile[i].nLife && g_Boss.nLife > 0)
			{
				CollisionExis = Collision(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, g_Player.CannonMissile[i].nX + 1, g_Player.CannonMissile[i].nY + 1,
					g_Boss.nX, g_Boss.nY, g_Boss.nX + 16, g_Boss.nY + 3);
				if (CollisionExis)
				{
					g_Boss.nLife--;
					g_Player.CannonMissile[i].nLife = 0;

					if (g_Boss.nLife <= 0)
					{
						score += 5000;

						SoundPlay(&collision_sound, 0.3f);
					}
				}
			}
		}
	}

	// 플레이어 미사일 <=> 보스 미사일
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife)
		{
			for (int j = 0; j < BOSS_MISSILE_COUNT; j++)
			{
				if (g_BossMissile[j].nLife)
				{
					CollisionExis = Collision(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, g_PlayerMissile[i].nX + 1, g_PlayerMissile[i].nY + 1,
						g_BossMissile[j].nX, g_BossMissile[j].nY, g_BossMissile[j].nX + 2, g_BossMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_PlayerMissile[i].nLife = 0;
						g_BossMissile[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 미사일 <=> 보스 운석
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife)
		{
			for (int j = 0; j < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.Meteor[j].nLife)
				{
					CollisionExis = Collision(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, g_PlayerMissile[i].nX + 1, g_PlayerMissile[i].nY + 1,
						g_Boss.Meteor[j].nX, g_Boss.Meteor[j].nY, g_Boss.Meteor[j].nX + 2, g_Boss.Meteor[j].nY + 1);
					if (CollisionExis)
					{
						g_PlayerMissile[i].nLife = 0;
						g_Boss.Meteor[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 캐논 미사일 <=> 보스 미사일 (보스 미사일만 사라짐)
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife)
		{
			for (int j = 0; j < BOSS_MISSILE_COUNT; j++)
			{
				if (g_BossMissile[j].nLife)
				{
					CollisionExis = Collision(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, g_Player.CannonMissile[i].nX + 1, g_Player.CannonMissile[i].nY + 1,
						g_BossMissile[j].nX, g_BossMissile[j].nY, g_BossMissile[j].nX + 2, g_BossMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_BossMissile[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 캐논 미사일 <=> 보스 운석
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife)
		{
			for (int j = 0; j < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.Meteor[j].nLife)
				{
					CollisionExis = Collision(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, g_Player.CannonMissile[i].nX + 1, g_Player.CannonMissile[i].nY + 1,
						g_Boss.Meteor[j].nX, g_Boss.Meteor[j].nY, g_Boss.Meteor[j].nX + 2, g_Boss.Meteor[j].nY + 1);
					if (CollisionExis)
					{
						g_Boss.Meteor[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 보스 미사일
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < BOSS_MISSILE_COUNT; j++)
			{
				if (g_BossMissile[j].nLife)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						g_BossMissile[j].nX, g_BossMissile[j].nY, g_BossMissile[j].nX + 2, g_BossMissile[j].nY + 1);
					if (CollisionExis)
					{
						g_BossMissile[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 보스 포
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.ElectronGun[j].nLife)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						g_Boss.ElectronGun[j].nX, g_Boss.ElectronGun[j].nY, g_Boss.ElectronGun[j].nX + 16, g_Boss.ElectronGun[j].nY + 1);
					if (CollisionExis)
					{
						g_Boss.ElectronGun[j].nLife = 0;
						break; // 포 한줄이 하나이기 때문
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 보스 조준탄
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.AimingShot[j].nLife)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						(int)g_Boss.AimingShot[j].nXf, (int)g_Boss.AimingShot[j].nYf, (int)g_Boss.AimingShot[j].nXf + 2, (int)g_Boss.AimingShot[j].nYf + 1);
					if (CollisionExis)
					{
						g_Boss.AimingShot[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 보스 유도탄
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.GuidedShot[j].nLife)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						(int)g_Boss.GuidedShot[j].nXf, (int)g_Boss.GuidedShot[j].nYf, (int)g_Boss.GuidedShot[j].nXf + 1, (int)g_Boss.GuidedShot[j].nYf + 1);
					if (CollisionExis)
					{
						g_Boss.GuidedShot[j].nLife = 0;
					}
				}
			}
		}
	}
	// 플레이어 실드 <=> 보스 운석
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			for (int j = 0; j < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); j++)
			{
				if (g_Boss.Meteor[j].nLife)
				{
					CollisionExis = Collision(g_Player.Shield[i].nX, g_Player.Shield[i].nY, g_Player.Shield[i].nX + 1, g_Player.Shield[i].nY + 1,
						g_Boss.Meteor[j].nX, g_Boss.Meteor[j].nY, g_Boss.Meteor[j].nX + 2, g_Boss.Meteor[j].nY + 1);
					if (CollisionExis)
					{
						g_Boss.Meteor[j].nLife = 0;
					}
				}
			}
		}
	}

}

void Update()
{
	clock_t CurTime = clock();

	PlayerMissileUpdate(CurTime); // 플레이어 모든 미사일 이동 제어

	// 데미지를 받으면 활성화 되며 HitTime이 경과될 때까지 데미지를 받지 않는다.
	if (!g_Player.EnableDamage)
	{
		if (CurTime - g_Player.OldHitTime >= g_Player.HitTime)
		{
			g_Player.EnableDamage = 1;
		}
	}

	if (stage != 5) // 보스전인지 확인 ( 5가 아니면 일반 스테이지 )
	{
		EnemyCreateUpdate(CurTime); // 적 생성 ( 각 스테이지마다 해당 )

		EnemyAndPlayerCollisionUpdate(); // 1차 적기 충돌 유무
		
		EnemyMoveAndMissileUpdate(CurTime); // 적 이동과 적 초기 미사일 발사
		
		EnemyMissileMoveUpdate(CurTime); // 적 미사일 이동

		EnemyAndPlayerCollisionUpdate(); // 2차 적기 충돌 유무

		ItemMoveUpdate(CurTime); // 아이템 떨어졌으면 이동

		ItemAndPlayerCollisionUpdate(); // 아이템 충돌 유무

		BossAndPlayerCollisionUpdate(); // 1차 보스 충돌 유무

		BossMissileMoveUpdate(CurTime); // 보스 일반적인 미사일, 포 미사일 이동

		BossAndPlayerCollisionUpdate(); // 2차 보스 충돌 유무
	}
	else // 보스전
	{
		if (gameBossScript) // 보스전 때 스크립트 유무
		{
			BossScriptUpdate(CurTime); // 스크립트 변화
		}
		else
		{
			// 보스전 시작 때는 일반 적기 미사일 모두 삭제
			for (int i = 0; i < ENEMY_MISSILE_COUNT; i++)
			{
				g_EnemyMissile[i].nLife = 0;
			}

			if (g_Boss.nLife) // 보스가 살아 있으면 보스 이동 및 여러 미사일 생성
			{
				// 스킬 및 일반적인 포
				if (g_Boss.EnableElectronGun) // 포 스킬이 유무
				{
					BossMoveUpdate(CurTime); // 보스 이동
					BossElectronGunUpdate(CurTime); // 보스 포 미사일 생성
				}
				else if (g_Boss.EnableAimingShot)
				{
					BossAimingShotUpdate(CurTime); // 보스 조준탄 미사일 생성
				}
				else if (g_Boss.EnableGuidedShot)
				{
					BossMoveUpdate(CurTime); // 보스 이동
					BossGuidedShotUpdate(CurTime); // 유도탄 발사
					BossMissileUpdate(CurTime); // 보스 일반적인 미사일 생성
				}
				else if (g_Boss.EnableMeteorFall)
				{
					BossMoveUpdate(CurTime); // 보스 이동
					BossMeteorFallUpdate(CurTime); // 운석 낙하
					BossMissileUpdate(CurTime); // 보스 일반적인 미사일 생성
				}
				else
				{
					BossMoveUpdate(CurTime); // 보스 이동
					BossMissileUpdate(CurTime); // 보스 일반적인 미사일 생성
					BossSkillCoolTimeUpdate(CurTime); // 스킬 쿨
				}
			}
			
			ItemMoveUpdate(CurTime); // 아이템 떨어졌으면 이동

			ItemAndPlayerCollisionUpdate(); // 아이템 충돌 유무

			BossAndPlayerCollisionUpdate(); // 1차 충돌 유무

			BossMissileMoveUpdate(CurTime); // 보스 일반적인 미사일, 포 미사일 이동

			BossAndPlayerCollisionUpdate(); // 2차 충돌 유무

			if (g_Boss.nLife <= 0)
			{
				g_Boss.EnableElectronGun = 0;
				g_Boss.EnableAimingShot = 0;
				g_Boss.EnableGuidedShot = 0;
				g_Boss.EnableMeteorFall = 0;

				g_Boss.WarningElectronGun = 0;
				g_Boss.SmallBossChange = 0;

			}
		}
	}

	// 플레이어가 죽거나 지구가 죽으면 게임 오버
	if (g_Player.nLife <= 0)
	{
		gameState = 0;
	}
	else if (g_Earth.nLife <= 0)
	{
		gameState = 0;
	}
	else if (chapter == 5 && stage == 5 && g_Boss.nLife <= 0) // 게임 클리어
	{
		char sSecond[3];
		char sMinute[4];
		char sScore[10];

		gameState = 2;
		endTime = clock();
		
		second = (endTime - startTime) / 1000;
		minute = second / 60;
		second %= 60;

		strcpy(timeAttact, "TIME : ");
		strcpy(finalScore, "SCORE : ");

		_itoa(second, sSecond, 10);
		_itoa(minute, sMinute, 10);

		strcat(timeAttact, sMinute);
		strcat(timeAttact, "m ");
		strcat(timeAttact, sSecond);
		strcat(timeAttact, "s");

		_itoa(score, sScore, 10);
		strcat(finalScore, sScore);
	}
}

// Render : 스크린 랜더 함수 ///////////////////////////////////////////
void ScreenBorderRender()
{
	ScreenPrint(0, 0, "┌");
	ScreenPrint(1, 0, "─────────────────────────────────────────────────────────────────────────────");
	ScreenPrint(SCREEN_WIDTH - 1, 0, "┐");
	for (int i = 0; i < 43; i++)
		ScreenPrint(0, 1 + i, "│");
	ScreenPrint(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, "┘");
	ScreenPrint(1, SCREEN_HEIGHT - 1, "─────────────────────────────────────────────────────────────────────────────");
	ScreenPrint(0, SCREEN_HEIGHT - 1, "└");
	for (int i = 0; i < 43; i++)
		ScreenPrint(SCREEN_WIDTH - 1, 1 + i, "│");
}

void StartRender() {

	int xStart = 4;
	int yStart = 2;

	ScreenPrint(xStart, yStart, "위   : ↑");
	ScreenPrint(xStart, yStart + 1, "아래 : ↓");
	ScreenPrint(xStart, yStart + 2, "선택 : Space bar");
	SetColor(10);
	ScreenPrint(xStart + 23, yStart + 10, "###     #    #       #  #####");
	ScreenPrint(xStart + 22, yStart + 11, "#       # #    #     #   #");
	ScreenPrint(xStart + 22, yStart + 12, "####   #####    #   #    ####");
	ScreenPrint(xStart + 26, yStart + 13, "#  #   #     # #     #");
	ScreenPrint(xStart + 22, yStart + 14, "####   #   #      #      #####");
	SetColor(9);
	ScreenPrint(xStart + 21, yStart + 17, "#####    #    ####   #####  #   #");
	ScreenPrint(xStart + 21, yStart + 18, "#       # #   #   #    #    #   #");
	ScreenPrint(xStart + 21, yStart + 19, "####   #####  ####     #    #####");
	ScreenPrint(xStart + 21, yStart + 20, "#      #   #  #  #     #    #   #");
	ScreenPrint(xStart + 21, yStart + 21, "#####  #   #  #   #    #    #   #");
	SetColor(15);
}
void MenuRender(MENU select) {
	int xMenu = 38;
	int yMenu = 25;

	switch (select) {
	case START:
		SetColor(6);
		ScreenPrint(xMenu, yMenu, "> START");
		SetColor(15);
		ScreenPrint(xMenu + 2, yMenu + 1, "HELP");
		ScreenPrint(xMenu + 2, yMenu + 2, "EXIT");
		break;
	case HELP:
		ScreenPrint(xMenu + 2, yMenu, "START");
		SetColor(6);
		ScreenPrint(xMenu, yMenu + 1, "> HELP");
		SetColor(15);
		ScreenPrint(xMenu + 2, yMenu + 2, "EXIT");
		break;
	case EXIT:
		ScreenPrint(xMenu + 2, yMenu, "START");
		ScreenPrint(xMenu + 2, yMenu + 1, "HELP");
		SetColor(6);
		ScreenPrint(xMenu, yMenu + 2, "> EXIT");
		SetColor(15);
		break;
	}
}
void HelpRender()
{
	int xHelp = 11;
	int yHelp = 8;

	SetColor(11);
	ScreenPrint(xHelp, yHelp, "                    - 게 임 스 토 리 -                     ");
	ScreenPrint(xHelp, yHelp + 1, "          지구가 외계인의 침공으로 위기에 빠졌다.          ");
	ScreenPrint(xHelp, yHelp + 2, " 여러 가지 방법으로 침공해오는 외계인을 막고 지구를 구하자.");
	SetColor(15);
	ScreenPrint(xHelp, yHelp + 6, "                        [ 조 작 법 ]                        ");
	ScreenPrint(xHelp, yHelp + 7, "                  이 동 : ← , ↑ , → , ↓                 ");
	ScreenPrint(xHelp, yHelp + 8, "               미 사 일 : 스 페 이 스 바                 ");
	ScreenPrint(xHelp, yHelp + 9, "                  케 논 : Z                 ");
	ScreenPrint(xHelp, yHelp + 10, "               베 리 어 : X                 ");
	ScreenPrint(xHelp, yHelp + 11, "                  선 택 : 스 페 이 스 바                   ");

	ScreenPrint(xHelp, yHelp + 13, "                        [ 아 이 템 ]                        ");
	SetColor(12);
	ScreenPrint(xHelp + 10, yHelp + 14, "♥");
	SetColor(8);
	ScreenPrint(xHelp + 10, yHelp + 16, "◎");
	SetColor(10);
	ScreenPrint(xHelp + 10, yHelp + 18, "★");
	SetColor(11);
	ScreenPrint(xHelp + 35, yHelp + 14, "▲");
	SetColor(13);
	ScreenPrint(xHelp + 35, yHelp + 16, "▲");
	SetColor(14);
	ScreenPrint(xHelp + 35, yHelp + 18, " ^");
	SetColor(15);
	ScreenPrint(xHelp + 14, yHelp + 14, ": 체력 1 회복");
	ScreenPrint(xHelp + 14, yHelp + 16, ": 캐논 스킬 1회 획득");
	ScreenPrint(xHelp + 14, yHelp + 18, ": 실드 스킬 1회 획득");
	ScreenPrint(xHelp + 39, yHelp + 14, ": 이속 증가");
	ScreenPrint(xHelp + 39, yHelp + 16, ": 공속 증가");
	ScreenPrint(xHelp + 39, yHelp + 18, ": 미사일 추가");

	SetColor(12);
	ScreenPrint(xHelp, yHelp + 21, " 개 발 자 : ");
	SetColor(14);
	ScreenPrint(xHelp + 12, yHelp + 21, "6 조");
	SetColor(2);
	ScreenPrint(xHelp + 17, yHelp + 21, "( 엄 민 호, ");
	SetColor(11);
	ScreenPrint(xHelp + 29, yHelp + 21, "백 진 우, ");
	SetColor(9);
	ScreenPrint(xHelp + 39, yHelp + 21, "안 성 민, ");
	SetColor(5);
	ScreenPrint(xHelp + 49, yHelp + 21, "장 현 수 ) ");
	SetColor(15);
	SetColor(12);
	ScreenPrint(xHelp + 21, yHelp + 23, "팀");
	SetColor(14);
	ScreenPrint(xHelp + 24, yHelp + 23, "명 :");
	SetColor(2);
	ScreenPrint(xHelp + 29, yHelp + 23, "미");
	SetColor(11);
	ScreenPrint(xHelp + 32, yHelp + 23, "소");
	SetColor(9);
	ScreenPrint(xHelp + 35, yHelp + 23, "G");
	SetColor(5);
	ScreenPrint(xHelp + 38, yHelp + 23, "O");
	SetColor(15);
	SetColor(1);
	ScreenPrint(xHelp, yHelp + 27, "스");
	SetColor(2);
	ScreenPrint(xHelp + 3, yHelp + 27, "페");
	SetColor(3);
	ScreenPrint(xHelp + 6, yHelp + 27, "이");
	SetColor(4);
	ScreenPrint(xHelp + 9, yHelp + 27, "스");
	SetColor(5);
	ScreenPrint(xHelp + 12, yHelp + 27, "키");
	SetColor(6);
	ScreenPrint(xHelp + 15, yHelp + 27, "를");
	SetColor(7);
	ScreenPrint(xHelp + 18, yHelp + 27, "누");
	SetColor(8);
	ScreenPrint(xHelp + 21, yHelp + 27, "르");
	SetColor(9);
	ScreenPrint(xHelp + 24, yHelp + 27, "면");
	SetColor(10);
	ScreenPrint(xHelp + 27, yHelp + 27, "메");
	SetColor(11);
	ScreenPrint(xHelp + 30, yHelp + 27, "인");
	SetColor(12);
	ScreenPrint(xHelp + 33, yHelp + 27, "화");
	SetColor(13);
	ScreenPrint(xHelp + 36, yHelp + 27, "면");
	SetColor(14);
	ScreenPrint(xHelp + 39, yHelp + 27, "으");
	SetColor(15);
	ScreenPrint(xHelp + 42, yHelp + 27, "로");
	SetColor(1);
	ScreenPrint(xHelp + 45, yHelp + 27, "이");
	SetColor(2);
	ScreenPrint(xHelp + 48, yHelp + 27, "동");
	SetColor(3);
	ScreenPrint(xHelp + 51, yHelp + 27, "합");
	SetColor(4);
	ScreenPrint(xHelp + 54, yHelp + 27, "니");
	SetColor(5);
	ScreenPrint(xHelp + 57, yHelp + 27, "다.");
	SetColor(15);
}
void GameOverRender() {
	int xGameOver = 4;
	int yGameOver = 12;

	SetColor(12);
	ScreenPrint(xGameOver, yGameOver + 0, "                        ####      #     # #   #####");
	ScreenPrint(xGameOver, yGameOver + 1, "                       #         # #   # # #  #    ");
	ScreenPrint(xGameOver, yGameOver + 2, "                       # #####  #####  # # #  #### ");
	ScreenPrint(xGameOver, yGameOver + 3, "                       #    #   #   #  # # #  #    ");
	ScreenPrint(xGameOver, yGameOver + 4, "                       #### #   #   #  # # #  #####");

	ScreenPrint(xGameOver, yGameOver + 8, "                        ####   #    #  #####  #####");
	ScreenPrint(xGameOver, yGameOver + 9, "                       #    #  #    #  #      #   #");
	ScreenPrint(xGameOver, yGameOver + 10, "                       #    #  #    #  ####   #### ");
	ScreenPrint(xGameOver, yGameOver + 11, "                       #    #   #  #   #      #  # ");
	ScreenPrint(xGameOver, yGameOver + 12, "                        ####     #     #####  #   #                        ");
	SetColor(15);
	ScreenPrint(xGameOver, yGameOver + 16, "               space bar를 누르시면 메인 메뉴로 돌아갑니다.                ");
}
void GameClearRender() {

	int xClear = 4;
	int yClear = 2;

	PlayerScript(18, 18);
	BossDieScript(50, 8);

	ScreenPrint(xClear + 3, yClear + 8, "악당들과 맞서 싸워 힘겹게 승리를");
	ScreenPrint(xClear + 3, yClear + 9, "거둔 주인공은 평화를 되찾았다");
	ScreenPrint(xClear + 3, yClear + 10, "오늘도 지구를 지킨 주인공은");
	ScreenPrint(xClear + 3, yClear + 11, "영원히 평생 지구인들에게 기억될 것이다.");
	ScreenPrint(xClear + 3, yClear + 12, "-save earth ending-");

	ScreenPrint(xClear + 40, yClear + 16, "- 최종 기록 -");
	ScreenPrint(xClear + 41, yClear + 17, timeAttact);
	ScreenPrint(xClear + 40, yClear + 18, finalScore);

	SetColor(10);
	ScreenPrint(xClear + 23, yClear + 24, "###     #    #       #  #####");
	ScreenPrint(xClear + 22, yClear + 25, "#       # #    #     #   #");
	ScreenPrint(xClear + 22, yClear + 26, "####   #####    #   #    ####");
	ScreenPrint(xClear + 26, yClear + 27, "#  #   #     # #     #");
	ScreenPrint(xClear + 22, yClear + 28, "####   #   #      #      #####");
	SetColor(15);
	SetColor(9);
	ScreenPrint(xClear + 21, yClear + 31, "#####    #    ####   #####  #   #");
	ScreenPrint(xClear + 21, yClear + 32, "#       # #   #   #    #    #   #");
	ScreenPrint(xClear + 21, yClear + 33, "####   #####  ####     #    #####");
	ScreenPrint(xClear + 21, yClear + 34, "#      #   #  #  #     #    #   #");
	ScreenPrint(xClear + 21, yClear + 35, "#####  #   #  #   #    #    #   #");
	SetColor(15);
}

void EarthRender()
{
	for (int i = 0; i < sizeof(g_Earth.surface) / sizeof(int); i++)
	{
		if (g_Earth.surface[i] == -1)
			SetColor(12);
		else if (g_Earth.surface[i] == 0)
			SetColor(9);
		else if (g_Earth.surface[i] == 1)
			SetColor(10);
		ScreenPrint(3 + i * 2, SCREEN_HEIGHT - 2, "■"); // 37개
	}
	SetColor(7);
}
void UiRender()
{
	char str[10];

	SetColor(15);
	ScreenPrint(2, 1, "HP : ");
	SetColor(12);
	for (int i = 0; i < g_Player.nLife; i++)
		ScreenPrint(7 + i * 2, 1, "♥");
	SetColor(15);

	ScreenPrint(2, 2, "CANNON : ");
	SetColor(8);
	for (int i = 0; i < g_Player.nCannon; i++)
		ScreenPrint(11 + i * 2, 2, "◎");
	SetColor(15);

	ScreenPrint(40, 2, "SHIELD : ");
	SetColor(10);
	for (int i = 0; i < g_Player.nShield; i++)
		ScreenPrint(49 + i * 2, 2, "★");
	SetColor(15);

	ScreenPrint(45, 1, "CHAPTER : ");
	_itoa(chapter, str, 10);
	ScreenPrint(55, 1, str);
	ScreenPrint(57, 1, "-");
	_itoa(stage, str, 10);
	ScreenPrint(59, 1, str);


	_itoa(score, str, 10);
	ScreenPrint(64, 1, "SCORE : ");
	ScreenPrint(72, 1, str);
}
void PlayerRender()
{
	if (g_Player.EnableDamage) // 데미지 받으면
	{
		SetColor(11);
	}
	else
	{
		SetColor(4);
	}
	ScreenPrint(g_Player.nX, g_Player.nY, "#-@-#");
	SetColor(15);
}
void PlayerMissileRender()
{
	// 플레이어 일반적인 미사일
	for (int i = 0; i < HERO_MISSILE_COUNT; i++)
	{
		if (g_PlayerMissile[i].nLife == 1)
		{
			SetColor(14);
			ScreenPrint(g_PlayerMissile[i].nX, g_PlayerMissile[i].nY, "^");
			SetColor(15);
		}
	}
	// 플레이어 캐논 미사일
	for (int i = 0; i < sizeof(g_Player.CannonMissile) / sizeof(MISSILE); i++)
	{
		if (g_Player.CannonMissile[i].nLife == 1)
		{
			SetColor(10);
			ScreenPrint(g_Player.CannonMissile[i].nX, g_Player.CannonMissile[i].nY, "^");
			SetColor(15);
		}
	}
	// 플레이어 실드
	for (int i = 0; i < sizeof(g_Player.Shield) / sizeof(MISSILE); i++)
	{
		if (g_Player.Shield[i].nLife == 1)
		{
			if (!(g_Player.Shield[i].nX < 2 || g_Player.Shield[i].nX > (SCREEN_WIDTH - 2) ||
				g_Player.Shield[i].nY < 3 || g_Player.Shield[i].nY > (SCREEN_HEIGHT - 3)))
			{
				SetColor(10);
				ScreenPrint(g_Player.Shield[i].nX, g_Player.Shield[i].nY, "-");
				SetColor(15);
			}
		}
	}

}
void ItemRender()
{
	for (int i = 0; i < sizeof(g_Item) / sizeof(ITEM); i++)
	{
		if (g_Item[i].nLife)
		{
			switch (g_Item[i].nType)
			{
			case 0:
				SetColor(12);
				break;
			case 1:
				SetColor(8);
				break;
			case 2:
				SetColor(10);
				break;
			case 3:
				SetColor(11);
				break;
			case 4:
				SetColor(13);
				break;
			case 5:
				SetColor(14);
				break;
			}
			ScreenPrint(g_Item[i].nX, g_Item[i].nY, g_ItemType[g_Item[i].nType]);
			SetColor(15);
		}
	}
}
void EnemyRender()
{
	for (int i = 0; i < g_nEnemyIndex; i++)
	{
		if (g_Enemy[i].nLife == 1)
		{
			if (g_EnemyType[g_Enemy[i].nType] == g_EnemyType[0])
			{
				SetColor(2);
			}
			else if (g_EnemyType[g_Enemy[i].nType] == g_EnemyType[1])
			{
				SetColor(8);
			}
			else if (g_EnemyType[g_Enemy[i].nType] == g_EnemyType[2])
			{
				SetColor(6);
			}
			else
			{
				SetColor(1);
			}
			ScreenPrint(g_Enemy[i].nX, g_Enemy[i].nY, g_EnemyType[g_Enemy[i].nType]);
			SetColor(15);
		}
	}
}
void EnemyMissileRender()
{
	// 적기 미사일
	for (int i = 0; i < ENEMY_MISSILE_COUNT; i++)
	{
		if (g_EnemyMissile[i].nLife == 1)
		{
			SetColor(12);
			ScreenPrint(g_EnemyMissile[i].nX, g_EnemyMissile[i].nY, "↓");
			SetColor(15);
		}
	}
}
void BossRender()
{
	if (g_Boss.nLife > 0)
	{
		if (g_Boss.SmallBossChange)
		{
			if (g_Boss.nShape == 1)
			{
				SetColor(6);
				ScreenPrint(g_Boss.nX, g_Boss.nY, " /|");
				SetColor(8);
				ScreenPrint(g_Boss.nX + 4, g_Boss.nY, "━ // ━ ");
				SetColor(6);
				ScreenPrint(g_Boss.nX + 12, g_Boss.nY, "|＼");
				SetColor(6);
				ScreenPrint(g_Boss.nX, g_Boss.nY + 1, "//q");
				SetColor(15);
				ScreenPrint(g_Boss.nX + 4, g_Boss.nY + 1, "●    ●");
				SetColor(6);
				ScreenPrint(g_Boss.nX + 13, g_Boss.nY + 1, "p＼");
				SetColor(12);
				ScreenPrint(g_Boss.nX, g_Boss.nY + 2, "  (┗━┻━━━┻━┛ )");
				SetColor(15);
			}
			else if (g_Boss.nShape == 2)
			{
				SetColor(6);
				ScreenPrint(g_Boss.nX + 4, g_Boss.nY + 1, "/");
				SetColor(15);
				ScreenPrint(g_Boss.nX + 6, g_Boss.nY + 1, "*");
				SetColor(12);
				ScreenPrint(g_Boss.nX + 8, g_Boss.nY + 1, "_");
				SetColor(15);
				ScreenPrint(g_Boss.nX + 10, g_Boss.nY + 1, "*");
				SetColor(6);
				ScreenPrint(g_Boss.nX + 12, g_Boss.nY + 1, "＼");
				SetColor(15);
			}
			else if (g_Boss.nShape == 3)
			{
				SetColor(15);
				ScreenPrint(g_Boss.nX + 8, g_Boss.nY + 1, "*");
				SetColor(15);
			}
		}
		else
		{
			if (g_Boss.nShape == 1)
			{
				SetColor(6);
				ScreenPrint(g_Boss.nX, g_Boss.nY, " /|");
				SetColor(8);
				ScreenPrint(g_Boss.nX + 4, g_Boss.nY, "━ // ━ ");
				SetColor(6);
				ScreenPrint(g_Boss.nX + 12, g_Boss.nY, "|＼");
				SetColor(6);
				ScreenPrint(g_Boss.nX, g_Boss.nY + 1, "//q");
				SetColor(15);
				ScreenPrint(g_Boss.nX + 4, g_Boss.nY + 1, "●    ●");
				SetColor(6);
				ScreenPrint(g_Boss.nX + 13, g_Boss.nY + 1, "p＼");
				SetColor(12);
				ScreenPrint(g_Boss.nX, g_Boss.nY + 2, "  (┗━┻━━━┻━┛ )");
				SetColor(15);
			}
			else if (g_Boss.nShape == 3)
			{
				SetColor(15);
				ScreenPrint(g_Boss.nX, g_Boss.nY, "*");
				SetColor(15);
			}
		}


	}
}
void BossMissileRender()
{
	// 보스 일반적인 미사일
	for (int i = 0; i < BOSS_MISSILE_COUNT; i++)
	{
		if (g_BossMissile[i].nLife)
		{
			SetColor(12);
			ScreenPrint(g_BossMissile[i].nX, g_BossMissile[i].nY, g_BossMissileType[g_BossMissile[i].nType]);
			SetColor(15);
		}
	}

	// 보스 스킬1 : 포
	if (g_Boss.EnableElectronGun)
	{
		if (g_Boss.WarningElectronGun) // 포 사용 전 경고
		{
			SetColor(12);
			for (int i = 0; i < SCREEN_HEIGHT - 3 - (g_Boss.nY + 2); i++)
			{
				ScreenPrint(g_Boss.nX, g_Boss.nY + (3 + i), "|");
				ScreenPrint(g_Boss.nX + 15, g_Boss.nY + (3 + i), "|");
			}
			ScreenPrint(g_Boss.nX + 7, g_Boss.nY + 6, "#");
			ScreenPrint(g_Boss.nX + 6, g_Boss.nY + 7, "# #");
			ScreenPrint(g_Boss.nX + 5, g_Boss.nY + 8, "# ! #");
			ScreenPrint(g_Boss.nX + 4, g_Boss.nY + 9, "#######");
			SetColor(15);
		}
	}
	for (int i = 0; i < sizeof(g_Boss.ElectronGun) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.ElectronGun[i].nLife)
		{
			SetColor(14);
			ScreenPrint(g_Boss.ElectronGun[i].nX, g_Boss.ElectronGun[i].nY, "■■■■■■■■");
			SetColor(15);
		}
	}

	// 보스 스킬2 : 조준탄
	for (int i = 0; i < sizeof(g_Boss.AimingShot) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.AimingShot[i].nLife)
		{
			SetColor(14);
			ScreenPrint((int)g_Boss.AimingShot[i].nXf, (int)g_Boss.AimingShot[i].nYf, g_BossMissileType[g_Boss.AimingShot[i].nType]);
			SetColor(15);
		}
	}

	// 보스 스킬3 : 유도탄
	for (int i = 0; i < sizeof(g_Boss.GuidedShot) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.GuidedShot[i].nLife)
		{
			if (!(g_Boss.GuidedShot[i].nXf < 2.0 || g_Boss.GuidedShot[i].nXf > (double)(SCREEN_WIDTH - 3) ||
				g_Boss.GuidedShot[i].nYf < 3.0 || g_Boss.GuidedShot[i].nYf > (double)(SCREEN_HEIGHT - 3)))
			{
				SetColor(14);
				ScreenPrint((int)g_Boss.GuidedShot[i].nXf, (int)g_Boss.GuidedShot[i].nYf,"*");
				SetColor(15);
			}
		}
	}

	// 보스 스킬4 : 운석 낙하
	for (int i = 0; i < sizeof(g_Boss.Meteor) / sizeof(BOSS_MISSILE); i++)
	{
		if (g_Boss.Meteor[i].nLife)
		{
			if (!(g_Boss.Meteor[i].nX < 2 || g_Boss.Meteor[i].nX > SCREEN_WIDTH - 3 ||
				g_Boss.Meteor[i].nY < 3 || g_Boss.Meteor[i].nY > SCREEN_HEIGHT - 3))
			{
				SetColor(12);
				if(g_Boss.Meteor[i].nDirect == UP)
					ScreenPrint(g_Boss.Meteor[i].nX, g_Boss.Meteor[i].nY, "▲");
				else if(g_Boss.Meteor[i].nDirect == DOWN)
					ScreenPrint(g_Boss.Meteor[i].nX, g_Boss.Meteor[i].nY, "▼");
				SetColor(15);
			}
		}
	}
}

void Render()
{
	ScreenClear();

	SetColor(15);
	if (gameMenu == 1) // 메인 메뉴 화면
	{
		StartRender();

		MenuRender(selectMenu); // 메뉴 선택 화면
	}
	else if (gameMenu == 2) // 도움말 화면
	{
		HelpRender();
	}
	else if(gameMenu == 0) // 게임 화면
	{
		EarthRender(); // 지구

		UiRender(); // 플레이어 인터페이스

		if (stage != 5) // 일반 스테이지
		{
			PlayerRender(); // 플레이어
			PlayerMissileRender(); // 플레이어 모든 미사일

			EnemyRender(); // 적기
			EnemyMissileRender(); // 적기 모든 미사일

			ItemRender();

			BossMissileRender(); // 보스 모든 미사일
		}
		else // 보스 스테이지
		{
			if (gameBossScript) // 보스 대화 유무
			{
				MainScript(CurScript); // 보스 대화 화면
			}
			else
			{
				PlayerRender(); // 플레이어
				PlayerMissileRender(); // 플레이어 모든 미사일

				BossRender(); // 보스
				BossMissileRender(); // 보스 모든 미사일

				ItemRender();
			}
		}

		// game over / game clear 화면
		if (gameState == 0) {
			GameOverRender();
		}
		else if (gameState == 2)
		{
			GameClearRender();
		}
	}

	ScreenFlipping();
}

// 메모리 반환 함수 ///////////////////////////////////////////
void Release()
{
	int nEnemyFileCount, nBossFileCount;
	nEnemyFileCount = sizeof(g_strFileNameEnemy) / sizeof(char*);
	nBossFileCount = sizeof(g_strFileNameBoss) / sizeof(char*);

	for (int i = 0; i < nEnemyFileCount; i++)
	{
		free(g_EnemyPatInfo[i].pPat);
	}
	free(g_EnemyPatInfo);

	for (int i = 0; i < nBossFileCount; i++)
	{
		free(g_BossPatInfo[i].pPat);
	}
	free(g_BossPatInfo);

	free(g_Enemy);
}

// 메인 함수 ///////////////////////////////////////////
int main()
{
	system("mode con cols=81 lines=45");
	SoundInit();
	ScreenInit();
	ScreenBorderRender();

	while (gameMenu != -1) { // 게임 종료는 -1
		SoundPlay(&menu_sound, 0.3f);
		if (gameMenu == 1) // 메인 메뉴
		{
			Render();
			PlayerKey();
		}
		else if (gameMenu == 2) // 도움말
		{
			PlayerKey();
			Render();
		}
		else if(gameMenu == 0) // 게임 시작
		{
			Init();

			SoundStop(&menu_sound);

			startTime = clock();
			while (gameMenu != 1) // 게임 메뉴로 돌아갈려면 1
			{
				if (gameState == 1) // 게임 오버 유무
				{
					PlayerKey();
					Stage();
					Update();
					Render();
				}
				else if (gameState == 2)
				{
					SoundPlay(&success_sound, 0.3f);

					if (oneRender)
					{
						Render();
						oneRender = 0;
					}
					PlayerKey();
				}
				else
				{
					SoundPlay(&fail_sound, 0.3f);

					if (oneRender)
					{
						Render();
						oneRender = 0;
					}
					PlayerKey();
				}
				FMOD_System_Update(g_System);
			}
			oneRender = 1;

			if (gameState == 2)
				SoundStop(&success_sound);
			else if(gameState == 0)
				SoundStop(&fail_sound);

			Release();
		}
		FMOD_System_Update(g_System);
	}

	AllSoundStop();
	SoundPlay(&exit_sound, 0.5f);

	while (isSoundPlay(&exit_sound) == 1){}
	SoundStop(&exit_sound);

	ScreenRelease();
	SoundRelease();
	return 0;
}