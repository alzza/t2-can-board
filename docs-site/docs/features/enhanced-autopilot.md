---
sidebar_position: 4
---

# 이전 Enhanced Autopilot 설정 안내

현재 LILYGO T2-CAN HW3 펌웨어는 `ENHANCED_AUTOPILOT`을 사용하지 않습니다. 이전 EAP 설정, 런타임 변수, NVS 키는 독립된 **조건부 Summon Unlock (HW3)**으로 대체되었습니다.

[HW3 조건부 Summon Unlock](/docs/features/smart-summon) 안내를 사용하십시오. 현재 구현은 `SUMMON_UNLOCK` 빌드 플래그, `/api/summon-unlock` API, `summon_unlock` NVS 키를 사용합니다.

:::note
이전 `enh_autopilot` NVS 값은 의도적으로 이전하지 않습니다. 펌웨어 업데이트 뒤에는 새 Summon 토글과 CAN-A TX 마스터를 직접 확인한 후 송신을 활성화하십시오.
:::
