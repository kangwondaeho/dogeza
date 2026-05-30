# 고멘나사이! 도게자 슬라이딩

> 원버튼 타이밍 액션 게임  
> 상대에게 닿기 직전, 가장 가까운 거리에서 도게자 슬라이딩을 멈추는 것이 목표입니다.

---

## 1. 게임 소개

**고멘나사이! 도게자 슬라이딩**은 플레이어가 리시버 NPC를 향해 달려가다가, 적절한 순간에 도게자 자세로 미끄러지는 타이밍 게임입니다.

플레이어는 리시버 앞에서 최대한 가깝게 멈춰야 합니다. 너무 멀리 멈추면 낮은 판정을 받고, 리시버에게 닿거나 지나치면 `FAIL`이 됩니다.

---

## 2. 실행 방법

1. Visual Studio 2022에서 `DogezaSliding.sln` 파일을 엽니다.
2. 빌드 구성을 `Debug x64` 또는 `Release x64`로 설정합니다.
3. 빌드 후 실행합니다.

> 프로젝트는 DirectX 11 기반으로 작성되었습니다.

---

## 3. 조작 방법

| 키 | 기능 |
|---|---|
| `Space` | 시작 / 달리기 시작 / 도게자 슬라이딩 |
| `R` | 현재 라운드 재시작 |
| `ESC` | 게임 종료 |

---

## 4. 게임 진행 방식

게임은 다음 상태 흐름으로 진행됩니다.

```text
TITLE → READY → APPROACH → PRONE_SLIDE → JUDGE → RESULT
```

### TITLE

타이틀 화면입니다. `Space`를 누르면 게임이 시작됩니다.

### READY

새 라운드가 준비된 상태입니다. 이때 맵과 리시버 위치가 랜덤으로 결정됩니다.

### APPROACH

플레이어가 리시버를 향해 달려갑니다. 원하는 타이밍에 `Space`를 누르면 도게자 슬라이딩을 시작합니다.

### PRONE_SLIDE

플레이어가 도게자 자세로 미끄러집니다. 맵마다 관성 시간이 다르기 때문에 같은 타이밍에 눌러도 멈추는 위치가 달라집니다.

### JUDGE / RESULT

플레이어와 리시버 사이의 거리를 계산하여 판정을 표시합니다.

---

## 5. 맵 규칙

게임에는 3개의 맵이 랜덤으로 등장합니다.

| 맵 | 이름 | 관성 시간 | 특징 |
|---|---|---:|---|
| MAP A | Immediate Stop | 0.0초 | 도게자 입력 즉시 정지 |
| MAP B | 1.5 sec Inertia | 1.5초 | 입력 후 조금 더 미끄러짐 |
| MAP C | 3.0 sec Inertia | 3.0초 | 입력 후 가장 길게 미끄러짐 |

맵마다 배경 이미지도 다르게 표시됩니다.

---

## 6. 판정 규칙

판정은 리시버와 플레이어 사이의 거리로 결정됩니다.

```text
거리 = receiverZ - playerZ
```

| 거리 조건 | 판정 | 점수 |
|---|---|---:|
| `0 < 거리 <= 25` | PERFECT | +1000 |
| `25 < 거리 <= 60` | GREAT | +700 |
| `60 < 거리 <= 120` | GOOD | +300 |
| `거리 <= 0` | FAIL | 현재 점수 초기화 |
| `거리 > 120` | FAIL | 현재 점수 초기화 |

핵심은 **리시버를 지나치지 않으면서 최대한 가까이 멈추는 것**입니다.

---

## 7. 점수 규칙

점수는 현재 점수와 최고 점수로 나뉩니다.

- `SCORE`: 현재 점수
- `BEST`: 최고 점수
- `LAST`: 직전 라운드에서 획득한 점수

성공 판정이 나오면 현재 점수에 점수가 누적됩니다.

```text
PERFECT → +1000
GREAT   → +700
GOOD    → +300
```

`FAIL`이 나오면 현재 점수는 `0`으로 초기화됩니다. 단, 최고 점수 `BEST`는 유지됩니다.

---

## 8. 주요 설계 구조

이 프로젝트는 강의에서 제공된 `GameLoop - GameObject - Component` 구조를 유지합니다.

```text
GameLoop
 └─ GameObject
     └─ Component
```

각 기능은 독립적인 컴포넌트로 나누어 구현했습니다.

| 컴포넌트 | 역할 |
|---|---|
| `MapRandomizerComponent` | 맵 랜덤 선택 및 관성 시간 제공 |
| `DepthTargetComponent` | 리시버의 목표 위치 관리 |
| `ProneSlideComponent` | 플레이어 달리기 / 도게자 / 관성 이동 처리 |
| `GameStateMachineComponent` | 게임 상태 전환 관리 |
| `JudgeComponent` | 거리 기반 판정 계산 |
| `ScoreComponent` | 현재 점수와 최고 점수 관리 |
| `FollowCameraComponent` | 플레이어 위치 기반 카메라 계산 |
| `BackgroundZoomComponent` | 맵별 배경 이미지와 배경 줌 처리 |
| `PlayerVisualComponent` | 플레이어 스프라이트 위치 및 자세 표시 |
| `ReceiverVisualComponent` | 리시버 위치 표시 |
| `BitmapFontHudComponent` | HUD 텍스트 표시 |
| `ResultImageOverlayComponent` | 판정 결과 이미지 표시 |

---

## 9. 프로젝트 파일 구성

| 파일 / 폴더 | 설명 |
|---|---|
| `main.cpp` | 게임 오브젝트와 컴포넌트를 생성하고 연결하는 진입점 |
| `DogezaTypes.hpp` | 게임 상태, 판정, 맵 규칙 자료형 |
| `DogezaComponents.hpp` | 게임 규칙 및 화면 표시 컴포넌트 |
| `Texture.hpp` | PNG 텍스처 로딩 |
| `Material.hpp` | 색상 / 텍스처 머티리얼 |
| `MeshRenderer.hpp` | 메시 렌더링 컴포넌트 |
| `assets/` | 배경, 캐릭터, 결과 이미지, 타이틀 이미지 |
| `effect.hlsl` | 기본 색상 렌더링 셰이더 |
| `texture.hlsl` | 텍스처 렌더링 셰이더 |

---

## 10. 설계 의도

`main.cpp`에 모든 게임 규칙을 직접 작성하지 않고, 역할별 컴포넌트로 분리했습니다.

- 맵 규칙은 `MapRandomizerComponent`
- 이동은 `ProneSlideComponent`
- 판정은 `JudgeComponent`
- 점수는 `ScoreComponent`
- 상태 전환은 `GameStateMachineComponent`
- 화면 표시는 Visual / HUD 컴포넌트

이렇게 나눈 이유는 특정 기능을 수정할 때 다른 코드에 주는 영향을 줄이기 위해서입니다.

예를 들어 점수 정책을 바꾸고 싶으면 `ScoreComponent`를 수정하면 되고, 판정 기준을 바꾸고 싶으면 `JudgeComponent`를 수정하면 됩니다.

---

## 11. 게임 목표 요약

```text
1. Space로 달리기 시작
2. 리시버에게 가까워질 때 Space로 도게자 슬라이딩
3. 리시버 앞에서 최대한 가깝게 멈추기
4. PERFECT / GREAT / GOOD 판정으로 점수 획득
5. FAIL을 피하면서 BEST 점수 갱신
```
