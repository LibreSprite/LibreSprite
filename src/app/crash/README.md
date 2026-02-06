# Crash Recovery

*Last updated: 2025-12-30*

Session backup and crash recovery system.

## Purpose

Automatically saves document state periodically so work can be recovered after a crash or unexpected shutdown.

## Key Classes

| Class | Purpose |
|-------|---------|
| `BackupObserver` | Monitors documents and triggers periodic backups |
| `DataRecovery` | Manages recovery of crashed sessions |
| `Session` | Represents a backup session with saved documents |

## Files

| File | Purpose |
|------|---------|
| `backup_observer.cpp/h` | Watches for document changes, schedules backups |
| `data_recovery.cpp/h` | Scans for and restores previous sessions |
| `session.cpp/h` | Session management and serialization |
| `read_document.cpp/h` | Deserialize backed-up documents |
| `write_document.cpp/h` | Serialize documents for backup |
| `raw_images_as.h` | Raw image data serialization |

## Backup Flow

1. `BackupObserver` detects document changes
2. Periodically writes session data via `write_document`
3. On next launch, `DataRecovery` checks for unfinished sessions
4. User prompted to recover or discard
