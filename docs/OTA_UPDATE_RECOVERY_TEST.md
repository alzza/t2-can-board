# OTA 업데이트 및 복구모드 실차 검증 절차

이 문서는 차량 안에서 노트북으로 TeslaCAN AP에 접속한 뒤 OTA 업로드, 새 펌웨어 확인, 자동 롤백, 복구모드 진입, 복구 펌웨어 업로드를 확인하는 절차다.

## 전제

- 차량은 안전하게 정차한 상태에서 진행한다.
- 노트북 배터리와 차량 전원이 충분한 상태에서 진행한다.
- OTA 중에는 보드 전원, USB-C 전원, 차량 CAN 배선을 건드리지 않는다.
- 현재 보드 AP 주소는 `http://192.168.4.1/` 이다.
- OTA 파일은 PlatformIO 빌드 산출물 `.pio/build/lilygo_t2can/firmware.bin` 을 사용한다.
- `ota_pending` 상태 의미는 아래와 같다.

| 값 | 의미 |
|---:|---|
| 0 | 정상 상태 |
| 1 | 새 펌웨어 기록 완료, 다음 부팅에서 확인 창 진입 예정 |
| 2 | 새 펌웨어 확인 대기 중 |
| 3 | 이전 파티션 롤백 예정 |
| 4 | 이전 펌웨어 복구 확인 대기 중 |
| 5 | OTA 전용 복구모드 |

## 1. 노트북 준비

1. 로컬에서 펌웨어를 빌드한다.

```bash
pio run -e lilygo_t2can
```

2. 산출물 크기와 첫 바이트를 확인한다.

```bash
ls -l .pio/build/lilygo_t2can/firmware.bin
xxd -l 16 .pio/build/lilygo_t2can/firmware.bin
```

성공 기준:
- 파일이 존재한다.
- 첫 바이트가 `e9` 로 시작한다.
- 크기가 `min_spiffs.csv`의 OTA app slot 크기 `0x1E0000` 보다 작다.

3. 차량에서 노트북 Wi-Fi를 `TeslaCAN` AP에 연결한다.

4. 상태 API가 응답하는지 확인한다.

```bash
curl -s http://192.168.4.1/api/status | head
```

`jq`가 있으면 아래 명령이 더 보기 쉽다.

```bash
curl -s http://192.168.4.1/api/status | jq '{fw:.firmware_build_id,state:.ota_pending_state,current:.ota_current_label,fallback:.ota_fallback_label,recovery:.ota_recovery_mode}'
```

성공 기준:
- HTTP 응답이 온다.
- `ota_pending_state` 가 보인다.

## 2. 최초 1회 업데이트 우회 절차

현재 차량에 올라간 구 펌웨어 UI가 `FormData` 방식이면 웹 버튼 업로드가 `업로드 중...` 에서 멈출 수 있다. 이 경우 최초 1회는 브라우저 버튼 대신 raw `.bin` 업로드 명령으로 진행한다.

```bash
curl -v \
  --connect-timeout 10 \
  --max-time 300 \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @.pio/build/lilygo_t2can/firmware.bin \
  http://192.168.4.1/api/ota
```

기대 결과:
- 정상 완료 시 `{"ok":true,"restarting":true}` 응답이 온다.
- 응답 직후 보드가 재부팅한다.
- 재부팅 중 Wi-Fi가 잠시 끊긴다.

주의:
- `curl`이 마지막에 연결 종료 또는 timeout처럼 보이더라도, 보드가 재부팅 중이면 정상일 수 있다.
- 업로드가 100% 전송되기 전에 끊기면 실패다. 다시 Wi-Fi 연결 후 재시도한다.

## 3. 새 펌웨어 부팅 확인

1. 재부팅 후 노트북을 다시 `TeslaCAN` AP에 연결한다.

2. 브라우저에서 `http://192.168.4.1/` 을 연다.

3. 페이지 최상단에 `펌웨어 업데이트 확인` 배너가 표시되는지 확인한다.

4. 상태 API를 확인한다.

```bash
curl -s http://192.168.4.1/api/status | jq '{fw:.firmware_build_id,state:.ota_pending_state,current:.ota_current_label,fallback:.ota_fallback_label,remaining:.ota_confirm_remaining_ms}'
```

성공 기준:
- `ota_pending_state` 가 `2` 다.
- `ota_confirm_remaining_ms` 가 0보다 크다.
- `ota_current_label` 과 `ota_fallback_label` 이 표시된다.

5. 정상 사용을 확정하려면 60초 안에 웹 배너의 `이 펌웨어 사용` 버튼을 누른다.

6. 다시 상태 API를 확인한다.

```bash
curl -s http://192.168.4.1/api/status | jq '{state:.ota_pending_state,fallback:.ota_fallback_label}'
```

성공 기준:
- `ota_pending_state` 가 `0` 이다.
- `ota_fallback_label` 이 비어 있거나 더 이상 롤백 대기 상태가 아니다.

## 4. 웹 버튼 OTA 재검증

이 절차는 이번 수정이 올라간 뒤 웹 UI 버튼이 raw `.bin`으로 업로드하는지 확인하는 테스트다.

1. OTA 탭을 연다.

2. `.pio/build/lilygo_t2can/firmware.bin` 을 선택한다.

3. `펌웨어 업로드` 버튼을 누른다.

4. 진행률이 올라가는지 본다.

성공 기준:
- 상태 문구가 `업로드 중...` 에서 계속 고정되지 않는다.
- 성공 시 `업로드 완료. 재부팅 중...` 이 표시된다.
- 재부팅 후 `ota_pending_state=2` 확인 배너가 나온다.

실패 기준:
- 5분 후 `업로드 실패: 시간 초과` 가 표시된다.
- `Upload raw firmware .bin, not multipart/form-data` 가 나오면 브라우저나 캐시가 구 UI를 쓰는 것이다. 새로고침 후 다시 시도하거나 2번의 `curl --data-binary` 절차를 사용한다.

## 5. 자동 롤백 검증

이 테스트는 새 펌웨어 확인을 일부러 누르지 않고 이전 파티션으로 돌아가는지 확인한다.

1. OTA 업로드를 완료해서 새 펌웨어로 재부팅한다.

2. `펌웨어 업데이트 확인` 배너가 보여도 `이 펌웨어 사용`을 누르지 않는다.

3. 60초 이상 기다린다.

4. 보드가 자동 재부팅하는지 확인한다.

5. 다시 `TeslaCAN` AP에 연결한다.

6. 상태 API를 확인한다.

```bash
curl -s http://192.168.4.1/api/status | jq '{fw:.firmware_build_id,state:.ota_pending_state,current:.ota_current_label,fallback:.ota_fallback_label,rollbackRemaining:.ota_rollback_remaining_ms}'
```

성공 기준:
- 이전 파티션으로 돌아온다.
- `ota_pending_state` 가 `4` 다.
- 웹 최상단에 `펌웨어 복구 완료` 배너가 보인다.

복구 완료를 확정하려면 60초 안에 `복구 완료 확인` 버튼을 누른다.

확정 후 성공 기준:
- `ota_pending_state` 가 `0` 이다.
- 다음 재부팅 후 복구 배너가 다시 뜨지 않는다.

## 6. OTA 전용 복구모드 진입 검증

이 테스트는 이전 펌웨어 복구 후에도 확인을 누르지 않았을 때 CAN 기능 없는 OTA 복구모드로 들어가는지 확인한다.

중요:
- 이 단계는 현재 파티션과 fallback 파티션 양쪽에 복구모드 로직이 들어간 펌웨어가 올라간 뒤에 정확히 검증된다.
- 한쪽 슬롯이 오래된 펌웨어면 `pending=4 → pending=5` 전이가 실행되지 않을 수 있다.

절차:
1. 5번 자동 롤백 검증에서 `펌웨어 복구 완료` 배너가 뜬 상태를 만든다.

2. `복구 완료 확인`을 누르지 않는다.

3. 60초 이상 기다린다.

4. 페이지를 새로고침하거나 보드 AP에 다시 연결한다.

5. `OTA 복구모드` 화면이 뜨는지 확인한다.

6. 상태 API를 확인한다.

```bash
curl -s http://192.168.4.1/api/status | jq '{state:.ota_pending_state,recovery:.ota_recovery_mode,current:.ota_current_label}'
```

성공 기준:
- `ota_pending_state` 가 `5` 다.
- `ota_recovery_mode` 가 `true` 다.
- 웹 화면이 일반 대시보드가 아니라 `OTA 복구모드` 화면이다.
- CAN 관련 기능이 비활성 상태로 웹 서버와 OTA만 살아 있다.

## 7. 복구모드에서 펌웨어 재업로드

1. 복구모드 화면에서 `.pio/build/lilygo_t2can/firmware.bin` 을 선택한다.

2. `펌웨어 업로드` 버튼을 누른다.

3. 진행률과 상태 문구를 확인한다.

성공 기준:
- `업로드 중: N%` 가 진행된다.
- 완료 시 `업로드 완료 — 재부팅 중...` 이 표시된다.
- 재부팅 후 일반 대시보드로 돌아온다.
- 새 펌웨어 확인 배너가 표시되고 `ota_pending_state=2` 가 된다.

4. 정상 동작 확인 후 60초 안에 `이 펌웨어 사용`을 누른다.

5. 최종 상태를 확인한다.

```bash
curl -s http://192.168.4.1/api/status | jq '{fw:.firmware_build_id,state:.ota_pending_state,recovery:.ota_recovery_mode,current:.ota_current_label,fallback:.ota_fallback_label}'
```

최종 성공 기준:
- `ota_pending_state=0`.
- `ota_recovery_mode=false`.
- 일반 대시보드가 표시된다.
- A/B 채널 상태가 정상적으로 갱신된다.

## 8. 실패 시 수집할 자료

문제가 발생하면 아래를 저장한다.

1. 현재 상태 API.

```bash
curl -s http://192.168.4.1/api/status > ota_status_failed.json
```

2. 전체 로그 번들.

```bash
curl -o canmod_logs_failed.txt http://192.168.4.1/api/logs-bundle
```

3. 업로드 명령 로그.

```bash
curl -v \
  --connect-timeout 10 \
  --max-time 300 \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @.pio/build/lilygo_t2can/firmware.bin \
  http://192.168.4.1/api/ota 2>&1 | tee ota_upload_failed.log
```

4. 웹 화면 스크린샷.

5. 보드가 재부팅했는지, Wi-Fi AP가 사라졌다가 다시 떴는지 메모한다.

## 9. 빠른 판정표

| 증상 | 우선 의심 | 다음 액션 |
|---|---|---|
| 웹 UI가 `업로드 중...` 에서 멈춤 | 구 UI의 multipart 업로드 | `curl --data-binary` raw 업로드 사용 |
| 서버 응답이 `multipart/form-data` 오류 | 캐시된 구 UI 또는 잘못된 업로드 방식 | 새로고침, raw curl 사용 |
| 업로드 직후 AP가 사라짐 | 재부팅 중일 수 있음 | 30~60초 후 AP 재접속 |
| 새 펌웨어 배너가 안 뜸 | USB 플래시 또는 `ota_pending` 미설정 | `/api/status`에서 `ota_pending_state` 확인 |
| 60초 후 자동 롤백 안 됨 | fallback 펌웨어가 구버전이거나 watchdog 미동작 | 양쪽 슬롯에 최신 펌웨어를 한 번씩 올린 뒤 재검증 |
| 복구모드 진입 안 됨 | fallback 슬롯에 복구모드 로직 없음 | 최신 펌웨어를 양쪽 OTA 슬롯에 확보 후 재검증 |