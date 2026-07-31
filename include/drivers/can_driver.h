// CAN 드라이버 추상 인터페이스
// MCP2515(SPI), TWAI(내장 CAN), MockDriver(테스트) 등이 이 인터페이스를 구현
#pragma once

#include "../can_frame_types.h"

enum class CanTxResult : uint8_t
{
    Queued = 0,
    Busy,
    ArbitrationLost,
    Aborted,
    ControllerError,
    InvalidFrame
};

inline bool canTxQueued(CanTxResult result)
{
    return result == CanTxResult::Queued;
}

struct CanDriver
{
    virtual bool init() = 0;                                    // 드라이버 초기화 (500kbps 설정 등)
    virtual void setFilters(const uint32_t *ids, uint8_t count) = 0;  // 수신 필터 설정 (필요한 ID만 수신)
    virtual bool enableInterrupt(void (*onReady)()) = 0;        // 수신 인터럽트 콜백 등록
    virtual bool read(CanFrame &frame) = 0;                     // 프레임 1개 수신 (없으면 false)
    virtual void send(const CanFrame &frame) = 0;               // 프레임 1개 전송
    // 프레임 1개 전송 + 결과 반환 (true=성공, false=실패).
    // 결과 확인을 구현하지 않은 드라이버의 기본값은 send() 호출 후 true다.
    virtual bool sendCheck(const CanFrame &frame) { send(frame); return true; }
    // 큐 포화와 실제 CAN 오류를 구분하는 상세 송신 결과.
    virtual CanTxResult sendDetailed(const CanFrame &frame)
    {
        return sendCheck(frame) ? CanTxResult::Queued : CanTxResult::ControllerError;
    }
    // 비동기 TX 완료 상태를 회수한다. MCP2515처럼 TXREQ가 나중에 해제되는
    // 드라이버가 완료/중재 손실/중단을 진단 카운터에 반영할 때 사용한다.
    virtual void pollTransmitResults() {}
    // OTA 플래시 쓰기 전에 하드웨어 송신 경로를 물리적으로 정지한다.
    // 성공 뒤에는 재부팅 전까지 송신을 재개하지 않는다.
    virtual bool quiesceTransmit() { return true; }
    // 에러 플래그 레지스터 (EFLG) 반환 — MCP2515 구현 시 실제 값, 나머지는 0
    // 비트 의미: bit5=TXBO(BUS-OFF), bit4=TXEP(TX에러패시브), bit2=TXWAR(TEC≥96)
    //            bit7=RX1OVR, bit6=RX0OVR (RX 버퍼 오버플로)
    virtual uint8_t getErrorFlags() { return 0; }
    // TX/RX 에러 카운터 읽기 — 0~255 범위. 기본 구현은 0/0
    virtual void getErrorCounters(uint8_t &tec, uint8_t &rec) { tec = 0; rec = 0; }
    // CANINTF.MERRF (메시지 에러: ACK/Bit/Stuff) 비트가 set이면 클리어 후 1, 아니면 0
    // 호출 시점부터 다음 호출까지 한 번이라도 발생했는지 감지.
    virtual uint8_t readAndClearMerrf() { return 0; }
    // EFLG.RX0OVR/RX1OVR sticky 비트 클리어 (호스트가 수동 클리어해야만 다음 OVR 검출 가능)
    virtual void clearRxOverrun() {}
    // BUS-OFF 상태에서 드라이버별 재초기화. MCP2515는 하드 리셋/재설정, 기본은 미지원.
    virtual bool recoverBusOff() { return false; }
    // 웹 UI/NVS 기반 런타임 설정 적용 훅. 기본 드라이버는 동작 없음.
    virtual void applyRuntimeSettings() {}
    virtual ~CanDriver() = default;
};
