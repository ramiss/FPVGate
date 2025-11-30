# Flash Tool Workflow Trigger Options

## Current Setup

The `test-flash-tool.yml` workflow currently triggers on:

1. **Push to main/master/develop** - BUT only when `flash_tool/**` files change
2. **Pull Requests** - When PRs touch `flash_tool/**` files
3. **Releases/Tags** - When you create/publish a release (same as `build-flasher.yml`)
4. **Manual dispatch** - You can trigger it manually from GitHub Actions UI

## Trigger Options Explained

### Option 1: Current Setup (Path Filter) ✅ **RECOMMENDED**
**Triggers:** Only when `flash_tool/**` changes

```yaml
push:
  branches: [ main, master, develop ]
  paths:
    - 'flash_tool/**'
```

**Pros:**
- ✅ Saves runner resources
- ✅ Only runs when flash tool actually changes
- ✅ Won't conflict with other workflows

**Cons:**
- ❌ Won't test if firmware changes affect flash tool compatibility

**Best for:** Development workflow - test flash tool when you modify it

---

### Option 2: Every Push (Like multi-board-tests.yml)
**Triggers:** On every push to main/develop

```yaml
push:
  branches: [ main, master, develop ]
  # Remove 'paths' filter
```

**Pros:**
- ✅ Always tests flash tool
- ✅ Catches compatibility issues early
- ✅ Matches your multi-board-test pattern

**Cons:**
- ❌ Uses more runner resources
- ❌ Might run unnecessarily

**Best for:** Critical validation - ensure flash tool always works

---

### Option 3: Release Only (Like build-flasher.yml)
**Triggers:** Only on releases/tags

```yaml
on:
  release:
    types: [ published, created ]
  workflow_dispatch:
  # Remove push/pull_request triggers
```

**Pros:**
- ✅ Only tests before release (most efficient)
- ✅ Ensures release quality
- ✅ Matches your build-flasher pattern

**Cons:**
- ❌ Won't catch issues during development
- ❌ Less frequent feedback

**Best for:** Release validation only

---

### Option 4: Scheduled (Nightly)
**Triggers:** On a schedule (e.g., daily at 2 AM)

```yaml
schedule:
  - cron: '0 2 * * *'  # 2 AM UTC daily
```

**Pros:**
- ✅ Regular validation
- ✅ Catches hardware issues over time
- ✅ Doesn't block development

**Cons:**
- ❌ Delayed feedback
- ❌ Might miss immediate issues

**Best for:** Continuous validation alongside other triggers

---

### Option 5: Hybrid (Recommended for Production)
Combine multiple triggers:

```yaml
on:
  # Test before release
  release:
    types: [ published, created ]
  # Test when flash tool changes
  push:
    branches: [ main, master, develop ]
    paths:
      - 'flash_tool/**'
  # Nightly validation
  schedule:
    - cron: '0 2 * * *'
  # Manual trigger
  workflow_dispatch:
```

**Best for:** Balanced approach - test when needed + regular validation

---

## Comparison with Your Other Workflows

| Workflow | Triggers | Pattern |
|----------|----------|---------|
| `build-flasher.yml` | Release only | Release validation |
| `multi-board-tests.yml` | Every push | Continuous testing |
| `test-flash-tool.yml` (current) | Push (path-filtered) + Release | Hybrid |

## Recommended Setting

For a homelab with hardware testing, I recommend **Option 1 (current)** or **Option 5 (hybrid)**:

- **Option 1** if you want efficiency - only test when flash tool changes
- **Option 5** if you want comprehensive testing - test on changes + releases + nightly

## How to Change

Edit `.github/workflows/test-flash-tool.yml` and modify the `on:` section.

Current setting:
```yaml
push:
  branches: [ main, master, develop ]
  paths:
    - 'flash_tool/**'  # Only runs when flash_tool changes
```

To run on every push:
```yaml
push:
  branches: [ main, master, develop ]
  # Remove paths filter
```

To run only on releases:
```yaml
on:
  release:
    types: [ published, created ]
  workflow_dispatch:
  # Remove push/pull_request
```

