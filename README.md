# sogo-game
C언어 기반 콘솔 화면을 이용한 소고게임(개작)
## 개발 환경
- 언어: C언어
- 개발 Tool: Visual Studio
## 팀원
- 4인 개발

이름|엄민호|백진우|안성민|장현수|
---|---|---|---|---|
역할|버그 확인 및 수정, 벨런스 조정, 디자인, 화면구현|밸런스를 조정, 사운드 수집, 게임 내 대사 작성|버그 확인 및 수정, 플레이어 스킬 구상|코딩 총괄, 보스 스킬을 구상|
## 기간
- 2020.06 ~ 2020.12 (약 7개)
## 게임 디자인 및 개작
![image](https://github.com/nnyno/sogo-game/assets/104333303/ea2e1e0d-60fb-40e4-bf29-74e84f8c3241)
![image](https://github.com/nnyno/sogo-game/assets/104333303/b75a53bc-1ad7-44c6-aeb4-db96ff1b3a07)
![image](https://github.com/nnyno/sogo-game/assets/104333303/02b1a413-a80b-4f25-8035-683453ea70fd)
![image](https://github.com/nnyno/sogo-game/assets/104333303/6e028568-a790-453c-af95-4d9de9a0e41a)
### 개작 전 후
![image](https://github.com/nnyno/sogo-game/assets/104333303/f9bc3de6-76c9-4f4b-a3f4-34c9be529e6d)

4개의 보스 공격 패턴 중 하나로 2챕터 이후부터 나오도록 설계하였고 경고창이 뜬 후 앞으로 쏘는 방식
보스 미사일 구조체에서 가로로 노란줄이 하나씩 뜨면서 아래로 쏘는 방식

![image](https://github.com/nnyno/sogo-game/assets/104333303/c1483f09-c52b-47e9-809f-681fa726caa2)

유도탄으로 보스 머리에서 쏘아지며 플레이어가 어디에 있든 따라가서 맞추는 공격
유도탄->플레이어 방향으로 벡터를 구한 후 현재 유도탄이 움직이는 벡터와 더한 후 일정 스칼라만큼 크기를 조절해 연산한 벡터를 가지고 유도탄이 움직이게 설계

![image](https://github.com/nnyno/sogo-game/assets/104333303/41000df8-bb93-43a6-a67f-9e644a9c08be)

중앙에 나와서 미사일을 쏘기전 플레이어가 있는 위치에 방향을 잡고 일직선으로 쏘는 스킬

![image](https://github.com/nnyno/sogo-game/assets/104333303/3947714d-0909-4dbd-b66e-1adf2ee1b3d7)

보스가 운석을 위로 쏘아 올려서 떨어트리는 방식으로 떨어지면서 속도가 증가하는 방식으로 설계
보스 머리에서 위로 쏘아올라가고 일정 높이까지 올라가면 x축에 랜덤으로 떨어지게 설계
떨어질 때 속도를 등속으로 높아지기 위해 시간을 분수 함수 특징을 이용해 연산 제

![image](https://github.com/nnyno/sogo-game/assets/104333303/ff8ba3c5-a66d-4143-8eb6-1a132c7f7f0a)
입력 키를 따로 따로 연산하기 때문에 복수의 키를 누르면 동시에 반응하게 되어 사선 이동이 가능
## 함수 호출 계층도
![image](https://github.com/nnyno/sogo-game/assets/104333303/3c184713-1772-454e-894e-27f61dd15b1a)

![image](https://github.com/nnyno/sogo-game/assets/104333303/a705eadd-1b1f-4251-8f17-3a8e47a12741)![image](https://github.com/nnyno/sogo-game/assets/104333303/0758fdbc-0b69-46cc-bdfd-569d8a15f32f)

![image](https://github.com/nnyno/sogo-game/assets/104333303/b1768c1e-c33b-4a3b-b55f-688971a1d72c)![image](https://github.com/nnyno/sogo-game/assets/104333303/8f615e16-10dd-4716-aa10-3e0463f9e2ae)

![image](https://github.com/nnyno/sogo-game/assets/104333303/80cca4ea-fc3a-489a-8088-95265a6e9217)

![image](https://github.com/nnyno/sogo-game/assets/104333303/0fdc7106-8736-48f1-882c-be26b84504d4)

![image](https://github.com/nnyno/sogo-game/assets/104333303/c08fca87-141d-4e4c-910e-43bc43935711)

