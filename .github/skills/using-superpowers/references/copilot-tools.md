# VS Code Copilot Tool Mapping

Superpowers 문서에서 자주 보이는 도구명을 이 저장소 환경 도구로 대응한 표다.

| Superpowers 표현 | 이 저장소에서 사용 |
|---|---|
| Read | read_file |
| Write | create_file |
| Edit | apply_patch |
| Bash | run_in_terminal |
| Grep | grep_search |
| Glob | file_search |
| WebFetch | fetch_webpage |
| Task (subagent) | runSubagent |
| TodoWrite | manage_todo_list |

## 참고

- 병렬 가능한 읽기 작업은 `multi_tool_use.parallel` 사용을 우선한다.
- 테스트 실행은 가능한 경우 `runTests`를 우선한다.
- Python 스니펫 실행은 가능한 경우 `mcp_pylance_mcp_s_pylanceRunCodeSnippet`를 우선한다.
