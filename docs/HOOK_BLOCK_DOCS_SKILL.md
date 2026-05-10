# BLOCK_DOCS_SKILL 훅 정책

Git hooks로 관리되는 `docs/` 디렉토리 및 `SKILL.md` 파일의 커밋을 선택적으로 차단하는 정책입니다.

## 목적

- **기본 정책**: `docs/` 및 `SKILL.md` 파일은 언제든 커밋/푸시 가능 (차단 없음)
- **선택적 차단**: 필요할 때만 환경변수 또는 플래그 파일로 차단 활성화
- **유연한 제어**: 개발자가 원할 때만 docs 관련 변경을 제외할 수 있음

## 기본 동작

**차단 비활성화 상태 (기본값)**
```bash
git commit -m "..."      # ✅ docs/SKILL.md 포함 가능
git push                 # ✅ docs/SKILL.md 포함 가능
```

## 차단 활성화 방법

### 1. 환경변수 방식

한 번의 커밋/푸시에만 차단을 적용합니다.

```bash
# pre-commit 훅 활성화
BLOCK_DOCS_SKILL=1 git commit -m "..."

# pre-push 훅 활성화  
BLOCK_DOCS_SKILL=1 git push

# 둘 다 활성화
BLOCK_DOCS_SKILL=1 git commit -m "..." && BLOCK_DOCS_SKILL=1 git push
```

### 2. 플래그 파일 방식

별도의 플래그 파일을 생성하면 지속적으로 차단됩니다.

```bash
# 차단 활성화 — 플래그 파일 생성
touch /Users/akanus/tesla-open-can-mod/.git/BLOCK_DOCS_SKILL

# 이후 모든 커밋/푸시가 docs/SKILL.md 차단
git commit -m "..."  # ❌ docs/ 또는 SKILL.md 포함 시 실패
git push             # ❌ docs/ 또는 SKILL.md 포함 시 실패

# 차단 해제 — 플래그 파일 삭제
rm /Users/akanus/tesla-open-can-mod/.git/BLOCK_DOCS_SKILL
```

## 차단 로직

`.git/hooks/pre-commit`, `.git/hooks/pre-push` 훅에서 아래 순서로 처리됩니다.

1. `BLOCK_DOCS_SKILL` 환경변수 확인 (기본값: 0)
2. `BLOCK_DOCS_SKILL=1` 또는 플래그 파일 존재 시:
   - Staged files / Push 대상 커밋에서 `docs/` 또는 `SKILL.md` 패턴 검색
   - 일치하는 파일 발견 시 커밋/푸시 **실패**
3. 조건 미충족 시: **성공** (차단 없음)

## 사용 시나리오

### 시나리오 1: 일반적인 개발

```bash
# docs 변경을 포함한 커밋 (기본 허용)
$ git add docs/ src/
$ git commit -m "docs: add feature guide & refactor handler"
✅ OK (docs 파일 포함 가능)
```

### 시나리오 2: docs 제외 강제

어떤 이유로든 **이번 세션에서만** docs 변경을 막고 싶다면:

```bash
# 환경변수로 일회성 활성화
$ BLOCK_DOCS_SKILL=1 git commit -m "..."
❌ Error: pre-commit 차단: docs/ 또는 SKILL.md 파일은 커밋할 수 없습니다.

# 또는 플래그 파일로 지속적 활성화
$ touch .git/BLOCK_DOCS_SKILL
$ git commit -m "..."
❌ Error: pre-commit 차단

# docs 제외 후 재커밋
$ git reset HEAD docs/
$ git commit -m "..."
✅ OK (docs 파일 제외)
```

### 시나리오 3: 훅 우회 필요시

Git hooks를 강제로 우회하려면 `--no-verify` 옵션 사용:

```bash
# 차단을 무시하고 강제 커밋
$ git commit --no-verify -m "..."
✅ OK (hooks 스킵)

# 차단을 무시하고 강제 푸시
$ git push --no-verify
✅ OK (hooks 스킵)
```

## 훅 파일 위치

```
/Users/akanus/tesla-open-can-mod/.git/hooks/
├── pre-commit (BLOCK_DOCS_SKILL 토글 포함)
└── pre-push   (BLOCK_DOCS_SKILL 토글 포함)
```

## FAQ

**Q: docs/SKILL.md를 항상 커밋하고 싶어요.**  
A: 기본값이 차단 비활성화이므로 아무 조치 필요 없습니다. 그냥 커밋하면 됩니다.

**Q: 실수로 docs를 포함한 커밋을 했어요. 어떻게 하나요?**  
A: `BLOCK_DOCS_SKILL=1 git commit --amend` 로 재커밋하거나, docs 파일을 제외 후 `git reset HEAD docs/` 후 다시 커밋하세요.

**Q: 플래그 파일이 `.git` 내부에 있으면 Git tracking이 안 되나요?**  
A: 맞습니다. `.git/BLOCK_DOCS_SKILL`은 worktree 로컬 설정이므로 Git에 의해 추적되지 않습니다. (의도된 동작)

**Q: 훅을 완전히 비활성화하고 싶어요.**  
A: `.git/hooks/pre-commit`, `.git/hooks/pre-push`를 삭제하거나 executable 권한을 제거하면 됩니다.
