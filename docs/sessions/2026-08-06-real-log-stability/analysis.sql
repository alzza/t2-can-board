-- DuckDB 기준 시계열 핵심 지표 재현 쿼리.
-- 로그 CSV를 이 파일과 같은 작업 디렉터리에서 실행한다.
WITH
ts10 AS (
  SELECT * FROM read_csv_auto('can_timeseries_ab 10.csv', header = true)
),
ts11 AS (
  SELECT * FROM read_csv_auto('can_timeseries_ab 11.csv', header = true)
),
periods AS (
  SELECT
    '08-05 17:27~17:47' AS date,
    max(b_busoff) AS b_busoff,
    max(b_bus_error) AS b_bus_error,
    max(b_tec) AS b_tec_max,
    max(b_rec) AS b_rec_max,
    max(a_rx_overrun) - min(a_rx_overrun) AS a_overrun_delta,
    max(a_rx_queue_drops) - min(a_rx_queue_drops) AS a_queue_drops,
    max(a_tx_hard_error) - min(a_tx_hard_error) AS a_tx_hard_delta
  FROM ts10
  UNION ALL
  SELECT
    '08-06 07:19~07:39' AS date,
    max(b_busoff) AS b_busoff,
    max(b_bus_error) AS b_bus_error,
    max(b_tec) AS b_tec_max,
    max(b_rec) AS b_rec_max,
    max(a_rx_overrun) - min(a_rx_overrun) AS a_overrun_delta,
    max(a_rx_queue_drops) - min(a_rx_queue_drops) AS a_queue_drops,
    max(a_tx_hard_error) - min(a_tx_hard_error) AS a_tx_hard_delta
  FROM ts11
)
SELECT * FROM periods ORDER BY date;

