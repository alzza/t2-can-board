# D안 sign hold 컨텍스트 노트

- 결정: C안을 수정하지 않고 D안을 새 profile `4`로 추가한다.
- D안은 C안과 같은 `state2DelayMs=600`, `strongDelayMs=400`, `state2MildMaxRawDelta=170`, strong max `2.10Nm`을 유지한다.
- 바꾸는 것은 방향 결정뿐이다. AP gate, real hands-on bypass, checksum, counter, torque cap은 바꾸지 않는다.
- D안 방향 hold 후보값은 hold `1500ms`, zero deadband `0.6deg`, switch threshold `1.2deg`로 시작한다.
- `SCCM_steeringAngle` 근거: ID `297`, bytes `2..3`, little-endian 14bit, formula `raw * 0.1 - 819.2`.
- 실차 확인 포인트: USER_MARK를 경고 첫 표시와 해제 순간에 누르고, D안에서 직선 `modeBLastNm` 부호 전환 빈도와 `dSkipHO` spike를 비교한다.