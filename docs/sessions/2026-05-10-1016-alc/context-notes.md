# 1016 ALC 컨텍스트 노트

## 확인한 사실
- `ModelY_CH.dbc.txt`의 `BO_ 1016 UI_driverAssistControl`가 이번 작업 기준이다.
- `UI_ulcStalkConfirm : 1|1@1+`는 고속도로 자동 차선 변경 컨펌 관련 bit로 해석한다.
- `UI_alcOffHighwayEnable : 56|1@1+`는 시내 ALC enable bit로 해석한다.
- 현재 `HW3Handler`는 1021만 필터링하므로 1016 수신을 필터에 추가해야 한다.

## 구현 판단
- 1016 프레임에서 `UI_ulcStalkConfirm` 토글이 ON이면 bit1을 0으로 유지한다.
- 1016 프레임에서 `UI_alcOffHighwayEnable` 토글이 ON이면 bit56을 1로 유지한다.
- 각 신호가 이미 필요한 bit 값이면 신호별 스킵 카운터만 증가시키고 해당 신호는 수정하지 않는다.
- 한 프레임에서 두 신호 중 하나라도 변경이 필요할 때만 기존 A채널 `shouldSkipATx()` guard를 거쳐 `sendCheck()`한다.
- 1016 DBC 블록에는 checksum/counter 신호가 없어 checksum 갱신은 하지 않는다.

## 검증 메모
- 기존 native 전체 테스트는 이전 세션에서 실패 이력이 있으므로, 우선 변경과 직접 관련된 HW3 테스트와 빌드를 확인한다.
- `pio test -e native -f test_native_hw3`에서 신규 1016 신호별 토글 케이스 5개와 follow-distance 케이스는 통과했다.
- 같은 HW3 테스트 내 기존 1021 mux0/FSD, 787 track-mode 기대값 5개는 현재 핸들러 동작과 맞지 않아 실패했다. 이번 1016 변경으로 새로 생긴 실패는 아니다.
- `pio run -e lilygo_t2can`는 성공했다. RAM 22.7%, Flash 46.7%.
- mock 서버 재시작 후 Web UI 제어 탭에서 `UI_ulcStalkConfirm`, `UI_alcOffHighwayEnable` 스위치가 각각 표시됨을 확인했다.
- mock API에서 `UI_ulcStalkConfirm` OFF, `UI_alcOffHighwayEnable` ON 조합과 반대 조합이 독립적으로 반영됨을 확인했다.
- mock Web UI 진단 탭에서 신호별 주입/스킵 카운터 표시를 확인했다.

## 추가 판단
- 사용자가 bit1과 bit56을 따로 ON/OFF 하라고 정정했으므로 단일 `ALC 1016` 토글은 쓰지 않는다.
- `UI_ulcStalkConfirm` 토글은 bit1을 0으로 유지한다. UI 설명은 고속도로 NoA시 자동차선 변경 허용(스톡 컨펌완화)로 둔다.
- `UI_alcOffHighwayEnable` 토글은 bit56을 1로 유지한다. UI 설명은 비고속도로 자동차선변경 허용으로 둔다.
- 각 토글을 ON으로 켤 때는 기존 EAP/TSLLC와 같이 A채널 TX 마스터가 OFF이면 자동으로 켠다.
- 각 신호가 이미 필요한 bit 값이면 신호별 스킵 카운터만 증가시키고 해당 신호는 수정하지 않는다.
- 첨부 로그 `canmod_20260510_071256.txt`의 10분 시계열 기준 AP state 3 구간은 69샘플이고, 주입이 있었던 5초 샘플은 56개다.
- 같은 로그에서 첫 echo delay는 유효 표본 32개 기준 median 700ms, p75 700ms, max 710ms다. 현재 기본 700ms가 실제 동작 경계에 정확히 걸려 있다.
- 주입 토크 절대값은 주입 샘플 기준 median 0.94Nm, p75 1.18Nm, max 2.10Nm다.
- state2 mild 토크 절대값은 n=18, median 0.93Nm, p75 1.06Nm, max 1.50Nm다.
- strong 토크 절대값은 n=6, median 2.10Nm, p75 2.10Nm, max 2.10Nm다.
- 딜레이와 토크를 함께 조정한다면 state2 delay는 700ms에서 600ms 전후로 낮추고, state2 mild 상한은 1.5Nm에서 1.7Nm 전후로 조금 올리는 쪽이 로그와 가장 잘 맞는다.
- strong은 이미 2.10Nm까지 쓰고 표본이 적으므로 우선 400ms/2.10Nm를 유지한다.
